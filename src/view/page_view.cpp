#include "view/page_view.h"

#include <algorithm>
#include <cmath>

namespace efpdf {

PageView::PageView(DocumentModel &model) : model_(model) {
    area_.set_halign(Gtk::ALIGN_CENTER);
    area_.set_valign(Gtk::ALIGN_CENTER);
    area_.set_can_focus(true);
    area_.add_events(Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK |
                     Gdk::BUTTON1_MOTION_MASK | Gdk::SCROLL_MASK);
    area_.signal_draw().connect(sigc::mem_fun(*this, &PageView::on_area_draw));
    area_.signal_button_press_event().connect(
        sigc::mem_fun(*this, &PageView::on_area_press));
    area_.signal_motion_notify_event().connect(
        sigc::mem_fun(*this, &PageView::on_area_motion));
    area_.signal_button_release_event().connect(
        sigc::mem_fun(*this, &PageView::on_area_release));
    area_.signal_scroll_event().connect(
        sigc::mem_fun(*this, &PageView::on_area_scroll));
    add(area_);

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

    surface_ = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, w, h);
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
        // cairomm's operator enum lacks the blend modes, so use the C API.
        cairo_set_operator(cr->cobj(), CAIRO_OPERATOR_DIFFERENCE);
        cr->set_source_rgb(1.0, 1.0, 1.0);
        cr->paint();
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
    return tool_->on_motion(*this, make_event(event->x, event->y, 1));
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
