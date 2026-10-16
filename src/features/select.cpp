#include "features/select.h"

#include <algorithm>
#include <cmath>

#include "core/document_model.h"
#include "view/page_view.h"
#include "view/tool_registry.h"

namespace efpdf {
namespace {

// A drag shorter than this (in pixels) is a click, not a selection.
constexpr double kDragThreshold = 4.0;

bool inside(const PopplerRectangle &r, double x, double y) {
    return x >= std::min(r.x1, r.x2) && x <= std::max(r.x1, r.x2) &&
           y >= std::min(r.y1, r.y2) && y <= std::max(r.y1, r.y2);
}

// A PDF is untrusted input: only web and mail links are handed to the desktop.
// File paths, programs to launch and other schemes are ignored.
bool safe_uri(const std::string &uri) {
    const Glib::ustring lower = Glib::ustring(uri).lowercase();
    return lower.compare(0, 7, "http://") == 0 || lower.compare(0, 8, "https://") == 0 ||
           lower.compare(0, 7, "mailto:") == 0;
}

}  // namespace

// ---------------------------------------------------------------------------
// SelectTool
// ---------------------------------------------------------------------------

SelectTool::SelectTool(AppContext &ctx) : ctx_(ctx) {
    // A selection belongs to one page of one document.
    auto drop = [this] {
        selection_ = text_select::Selection();
        selection_page_ = -1;
        ctx_.view.area().queue_draw();
    };
    ctx.model.signal_page_changed.connect(drop);
    ctx.model.signal_loaded.connect(drop);
    ctx.model.signal_closed.connect(drop);
    ctx.model.signal_modified.connect(drop);
}

void SelectTool::activate(PageView &) { active_ = true; }

void SelectTool::deactivate(PageView &view) {
    active_ = false;
    dragging_ = false;
    clear();
    view.set_cursor(nullptr);
}

void SelectTool::clear() {
    if (selection_page_ < 0 && selection_.empty()) return;
    selection_ = text_select::Selection();
    selection_page_ = -1;
    ctx_.view.area().queue_draw();
}

std::string SelectTool::selected_text() const {
    if (selection_.empty() || selection_page_ != ctx_.model.current_page()) return "";
    PagePtr page = ctx_.model.current_page_object();
    return text_select::text(page.get(), selection_);
}

void SelectTool::select_all() {
    PagePtr page = ctx_.model.current_page_object();
    if (!page) return;
    if (!active_) {
        ctx_.view.signal_message.emit("Pick the Select tool to select text.");
        return;
    }
    double w = 0, h = 0;
    poppler_page_get_size(page.get(), &w, &h);
    select_between(0, 0, w, h);
    if (selection_.empty()) ctx_.view.signal_message.emit("There is no text on this page.");
}

void SelectTool::select_between(double x0, double y0, double x1, double y1) {
    PagePtr page = ctx_.model.current_page_object();
    if (!page) return;
    selection_ = text_select::select(page.get(), x0, y0, x1, y1);
    selection_page_ = selection_.empty() ? -1 : ctx_.model.current_page();
    ctx_.view.area().queue_draw();
}

bool SelectTool::on_press(PageView &view, const PointerEvent &event) {
    // Features with their own clickable items (form fields) get the click first.
    PointerEvent probe = event;
    view.signal_probe_click.emit(probe);
    if (probe.handled) {
        clear();
        return true;
    }

    clear();
    dragging_ = true;
    moved_ = false;
    press_x_ = event.x;
    press_y_ = event.y;
    pressed_link_ = link_at(event.pdf_x, event.pdf_y);
    return true;
}

bool SelectTool::on_motion(PageView &view, const PointerEvent &event) {
    if (!dragging_) return false;
    if (!moved_ && std::hypot(event.x - press_x_, event.y - press_y_) < kDragThreshold) {
        return true;
    }
    moved_ = true;
    const double s = view.scale();
    select_between(press_x_ / s, press_y_ / s, event.x / s, event.y / s);
    return true;
}

bool SelectTool::on_release(PageView &, const PointerEvent &event) {
    if (!dragging_ || event.button != 1) return false;
    dragging_ = false;
    if (!moved_ && pressed_link_.kind != Link::Kind::None) follow(pressed_link_);
    pressed_link_ = Link();
    return true;
}

void SelectTool::on_hover(PageView &view, const PointerEvent &event) {
    if (link_at(event.pdf_x, event.pdf_y).kind != Link::Kind::None) {
        view.set_cursor("pointer");
    } else {
        view.set_cursor("text");
    }
}

void SelectTool::paint(PageView &view, const Cairo::RefPtr<Cairo::Context> &cr) {
    if (selection_.empty() || selection_page_ != ctx_.model.current_page()) return;
    const double s = view.scale();
    cr->set_source_rgba(0.20, 0.50, 1.0, 0.35);
    for (const PopplerRectangle &r : selection_.lines) {
        cr->rectangle(r.x1 * s, r.y1 * s, (r.x2 - r.x1) * s, (r.y2 - r.y1) * s);
    }
    cr->fill();
}

SelectTool::Link SelectTool::link_at(double pdf_x, double pdf_y) {
    Link link;
    PagePtr page = ctx_.model.current_page_object();
    if (!page) return link;

    GList *mappings = poppler_page_get_link_mapping(page.get());
    for (GList *l = mappings; l; l = l->next) {
        auto *m = static_cast<PopplerLinkMapping *>(l->data);
        if (!m->action || !inside(m->area, pdf_x, pdf_y)) continue;

        if (m->action->type == POPPLER_ACTION_GOTO_DEST) {
            PopplerDest *dest = m->action->goto_dest.dest;
            PopplerDest *resolved = nullptr;
            if (dest && dest->type == POPPLER_DEST_NAMED && dest->named_dest) {
                resolved = poppler_document_find_dest(ctx_.model.poppler(), dest->named_dest);
                dest = resolved;
            }
            if (dest && dest->page_num >= 1 && dest->page_num <= ctx_.model.page_count()) {
                link.kind = Link::Kind::Page;
                link.page = dest->page_num - 1;
                if (dest->change_top && (dest->type == POPPLER_DEST_XYZ ||
                                         dest->type == POPPLER_DEST_FITH ||
                                         dest->type == POPPLER_DEST_FITBH)) {
                    link.top = dest->top;
                }
            }
            if (resolved) poppler_dest_free(resolved);
        } else if (m->action->type == POPPLER_ACTION_URI && m->action->uri.uri &&
                   safe_uri(m->action->uri.uri)) {
            link.kind = Link::Kind::Uri;
            link.uri = m->action->uri.uri;
        }
        if (link.kind != Link::Kind::None) break;
    }
    poppler_page_free_link_mapping(mappings);
    return link;
}

void SelectTool::follow(const Link &link) {
    if (link.kind == Link::Kind::Page) {
        ctx_.model.set_current_page(link.page);
        PagePtr page = ctx_.model.current_page_object();
        double w = 0, h = 0;
        if (page) poppler_page_get_size(page.get(), &w, &h);
        const double top = link.top >= 0 ? link.top : h;
        // Wait for the new page to be laid out before scrolling to it.
        Glib::signal_idle().connect_once(
            [this, top] { ctx_.view.scroll_to_pdf_point(0, top); });
    } else if (link.kind == Link::Kind::Uri) {
        try {
            Gio::AppInfo::launch_default_for_uri(link.uri);
        } catch (const Glib::Error &e) {
            ctx_.view.signal_message.emit("Could not open the link: " + e.what());
        }
    }
}

// ---------------------------------------------------------------------------
// Select feature
// ---------------------------------------------------------------------------

Select::Select(AppContext &ctx) : tool_(ctx.tools.emplace<SelectTool>(ctx)) {
    ctx.view.signal_collect_selection.connect(
        [this](std::string &text) { text = tool_.selected_text(); });
    ctx.view.signal_select_all.connect([this] { tool_.select_all(); });
}

}  // namespace efpdf
