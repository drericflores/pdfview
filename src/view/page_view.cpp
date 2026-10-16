#include "view/page_view.h"

#include <algorithm>
#include <cmath>

namespace efpdf {

PageView::PageView(DocumentModel &model) : model_(model) {
    // The overlay is centred in the viewport and is exactly the page's size,
    // so overlay coordinates equal drawing-area coordinates.
    overlay_.set_halign(Gtk::ALIGN_CENTER);
    overlay_.set_valign(Gtk::ALIGN_CENTER);
    // Breathing room around the page. fit_width() and fit_page() subtract
    // twice this margin (2 x 12 = the 24 px they allow for).
    overlay_.set_margin_start(12);
    overlay_.set_margin_end(12);
    overlay_.set_margin_top(12);
    overlay_.set_margin_bottom(12);
    area_.set_can_focus(true);
    area_.add_events(Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK |
                     Gdk::POINTER_MOTION_MASK | Gdk::LEAVE_NOTIFY_MASK |
                     Gdk::SCROLL_MASK);
    area_.signal_draw().connect(sigc::mem_fun(*this, &PageView::on_area_draw));
    area_.signal_button_press_event().connect(
        sigc::mem_fun(*this, &PageView::on_area_press));
    area_.signal_motion_notify_event().connect(
        sigc::mem_fun(*this, &PageView::on_area_motion));
    area_.signal_button_release_event().connect(
        sigc::mem_fun(*this, &PageView::on_area_release));
    area_.signal_scroll_event().connect(
        sigc::mem_fun(*this, &PageView::on_area_scroll));
    area_.signal_leave_notify_event().connect(
        sigc::mem_fun(*this, &PageView::on_area_leave));
    // Moving the window to a screen with a different scale factor (HiDPI)
    // needs the page re-rendered at the new resolution.
    area_.property_scale_factor().signal_changed().connect(
        sigc::mem_fun(*this, &PageView::refresh));
    overlay_.add(area_);
    add(overlay_);

    // Taking keyboard focus must not scroll the page: GTK would otherwise
    // scroll the focused child into view, which shifts the page up by the
    // margin around it the first time it is clicked. Point the viewport's
    // "scroll the focus into view" adjustments at throwaway objects instead.
    if (auto *viewport = dynamic_cast<Gtk::Viewport *>(get_child())) {
        viewport->set_focus_hadjustment(Gtk::Adjustment::create(0, 0, 0, 0, 0, 0));
        viewport->set_focus_vadjustment(Gtk::Adjustment::create(0, 0, 0, 0, 0, 0));
    }

    model_.signal_loaded.connect(sigc::mem_fun(*this, &PageView::refresh));
    model_.signal_closed.connect(sigc::mem_fun(*this, &PageView::refresh));
    model_.signal_page_changed.connect(sigc::mem_fun(*this, &PageView::refresh));
    model_.signal_modified.connect(sigc::mem_fun(*this, &PageView::refresh));
}

bool PageView::measure() {
    PagePtr page = model_.current_page_object();
    if (!page) {
        page_w_ = page_h_ = 0;
        return false;
    }
    poppler_page_get_size(page.get(), &page_w_, &page_h_);
    return true;
}

void PageView::set_scale(double scale) {
    scale_ = std::clamp(scale, kMinScale, kMaxScale);
    refresh();
    signal_scale_changed.emit();
}

void PageView::fit_width() {
    if (!measure() || page_w_ <= 0) return;
    const int available = std::max(1, get_allocation().get_width() - 24);
    set_scale(available / page_w_);
}

void PageView::fit_page() {
    if (!measure() || page_w_ <= 0 || page_h_ <= 0) return;
    const int w = std::max(1, get_allocation().get_width() - 24);
    const int h = std::max(1, get_allocation().get_height() - 24);
    set_scale(std::min(w / page_w_, h / page_h_));
}

void PageView::set_dark(bool dark) {
    if (dark_ == dark) return;
    dark_ = dark;
    refresh();
}

void PageView::set_tool(Tool *tool) {
    if (tool_ && tool_ != tool) tool_->deactivate(*this);
    tool_ = tool;
    set_cursor(nullptr);
    if (tool_) tool_->activate(*this);
    area_.queue_draw();
}

void PageView::refresh() {
    // Assigning an empty RefPtr works with both old and new cairomm.
    surface_ = Cairo::RefPtr<Cairo::ImageSurface>();
    PagePtr page = model_.current_page_object();
    if (!page) {
        page_w_ = page_h_ = 0;
        area_.set_size_request(0, 0);
        area_.queue_draw();
        return;
    }

    poppler_page_get_size(page.get(), &page_w_, &page_h_);
    const int w = std::max(1, static_cast<int>(std::ceil(page_w_ * scale_)));
    const int h = std::max(1, static_cast<int>(std::ceil(page_h_ * scale_)));

    // Render at the screen's real resolution so text stays sharp on HiDPI
    // displays: the surface has dpr x more pixels, and its device scale tells
    // cairo that one drawing unit is still one logical pixel.
    const int dpr = std::max(1, area_.get_scale_factor());
    const int pw = w * dpr, ph = h * dpr;
    surface_w_ = w;
    surface_h_ = h;
    surface_ = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, pw, ph);
    cairo_surface_set_device_scale(surface_->cobj(), dpr, dpr);
    auto cr = Cairo::Context::create(surface_);

    // Poppler draws onto a transparent canvas, so lay down paper first.
    cr->set_source_rgb(1.0, 1.0, 1.0);
    cr->paint();
    cr->save();
    cr->scale(scale_, scale_);
    poppler_page_render(page.get(), cr->cobj());
    cr->restore();

    if (dark_) {
        // Invert the finished page: white paper becomes dark, black text light.
        // The page is fully opaque (it was painted on white), so flipping the
        // colour bytes directly is exact and does not depend on blend modes.
        surface_->flush();
        unsigned char *data = surface_->get_data();
        const int stride = surface_->get_stride();
        for (int y = 0; y < ph; ++y) {
            unsigned char *row = data + static_cast<size_t>(y) * stride;
            for (int x = 0; x < pw; ++x) {
                row[x * 4 + 0] = 255 - row[x * 4 + 0];  // blue
                row[x * 4 + 1] = 255 - row[x * 4 + 1];  // green
                row[x * 4 + 2] = 255 - row[x * 4 + 2];  // red
            }
        }
        surface_->mark_dirty();
    }

    area_.set_size_request(w, h);
    area_.queue_draw();
}

void PageView::to_pdf(double sx, double sy, double &px, double &py) const {
    px = sx / scale_;
    py = page_h_ - sy / scale_;
}

void PageView::from_pdf(double px, double py, double &sx, double &sy) const {
    sx = px * scale_;
    sy = (page_h_ - py) * scale_;
}

void PageView::place_over_pdf_rect(Gtk::Widget &widget, double x1, double y1,
                                   double x2, double y2) {
    double sx0, sy0, sx1, sy1;
    from_pdf(std::min(x1, x2), std::max(y1, y2), sx0, sy0);  // top-left
    from_pdf(std::max(x1, x2), std::min(y1, y2), sx1, sy1);  // bottom-right
    widget.set_halign(Gtk::ALIGN_START);
    widget.set_valign(Gtk::ALIGN_START);
    widget.set_margin_start(static_cast<int>(sx0));
    widget.set_margin_top(static_cast<int>(sy0));
    widget.set_size_request(std::max(40, static_cast<int>(sx1 - sx0)),
                            std::max(24, static_cast<int>(sy1 - sy0)));
}

void PageView::scroll_to_pdf_point(double px, double py) {
    double sx = 0, sy = 0;
    from_pdf(px, py, sx, sy);
    auto adjustment = get_vadjustment();
    if (!adjustment) return;
    const double max_value =
        std::max(adjustment->get_lower(),
                 adjustment->get_upper() - adjustment->get_page_size());
    adjustment->set_value(
        std::clamp(sy - adjustment->get_page_size() / 3.0, adjustment->get_lower(),
                   max_value));
}

PointerEvent PageView::make_event(double x, double y, guint button) const {
    PointerEvent event;
    event.x = x;
    event.y = y;
    event.button = button;
    to_pdf(x, y, event.pdf_x, event.pdf_y);
    return event;
}

bool PageView::on_area_draw(const Cairo::RefPtr<Cairo::Context> &cr) {
    if (!surface_) return true;
    cr->set_source(surface_, 0, 0);
    cr->paint();

    // A hairline frame so the page edge reads clearly against the backdrop.
    cr->set_line_width(1.0);
    if (dark_) {
        cr->set_source_rgba(1.0, 1.0, 1.0, 0.20);
    } else {
        cr->set_source_rgba(0.0, 0.0, 0.0, 0.28);
    }
    cr->rectangle(0.5, 0.5, surface_w_ - 1.0, surface_h_ - 1.0);
    cr->stroke();
    if (tool_) tool_->paint(*this, cr);
    signal_paint_overlay.emit(cr);
    return true;
}

bool PageView::on_area_press(GdkEventButton *event) {
    if (!model_.is_open()) return false;
    area_.grab_focus();
    const PointerEvent pointer = make_event(event->x, event->y, event->button);
    if (event->button == 3) {
        signal_secondary_click.emit(pointer);
        return true;
    }
    return tool_ && event->button == 1 && tool_->on_press(*this, pointer);
}

bool PageView::on_area_motion(GdkEventMotion *event) {
    if (!tool_) return false;
    const PointerEvent pointer = make_event(event->x, event->y, 1);
    if (event->state & GDK_BUTTON1_MASK) return tool_->on_motion(*this, pointer);
    tool_->on_hover(*this, pointer);
    return false;
}

bool PageView::on_area_leave(GdkEventCrossing *) {
    set_cursor(nullptr);
    return false;
}

void PageView::set_cursor(const char *name) {
    const std::string wanted = name ? name : "";
    if (wanted == cursor_name_) return;
    cursor_name_ = wanted;
    auto window = area_.get_window();
    if (!window) return;
    if (name) {
        window->set_cursor(Gdk::Cursor::create(area_.get_display(), name));
    } else {
        window->set_cursor(Glib::RefPtr<Gdk::Cursor>());
    }
}

bool PageView::on_area_release(GdkEventButton *event) {
    if (!tool_) return false;
    return tool_->on_release(*this, make_event(event->x, event->y, event->button));
}

// Ctrl + wheel zooms; a plain wheel falls through to normal scrolling.
bool PageView::on_area_scroll(GdkEventScroll *event) {
    if (!(event->state & GDK_CONTROL_MASK)) return false;

    bool zoom_in = false;
    if (event->direction == GDK_SCROLL_UP) {
        zoom_in = true;
    } else if (event->direction == GDK_SCROLL_DOWN) {
        zoom_in = false;
    } else if (event->direction == GDK_SCROLL_SMOOTH) {
        if (event->delta_y == 0) return false;
        zoom_in = event->delta_y < 0;
    } else {
        return false;
    }
    set_scale(zoom_in ? scale_ * kZoomStep : scale_ / kZoomStep);
    return true;
}

}  // namespace efpdf
