#include "features/annotations.h"

#include <algorithm>
#include <cmath>
#include <string>

#include "core/document_model.h"
#include "core/settings.h"
#include "core/text_select.h"
#include "ui/dialogs.h"
#include "view/page_view.h"
#include "view/tool_registry.h"

namespace efpdf {
namespace {

bool inside(const PopplerRectangle &r, double x, double y) {
    return x >= std::min(r.x1, r.x2) && x <= std::max(r.x1, r.x2) &&
           y >= std::min(r.y1, r.y2) && y <= std::max(r.y1, r.y2);
}

// How many annotations Poppler now holds on the page (for feedback and logs).
guint annotation_count(PopplerPage *page) {
    GList *mappings = poppler_page_get_annot_mapping(page);
    const guint n = g_list_length(mappings);
    poppler_page_free_annot_mapping(mappings);
    return n;
}

void report_added(PageView &view, const char *what, PopplerPage *page) {
    const guint n = annotation_count(page);
    g_message("efpdf: %s added; the page now holds %u annotation(s)", what, n);
    view.signal_message.emit(Glib::ustring(what) + " added. Save (Ctrl+S) to keep it. " +
                             "Right-click an annotation to delete it.");
}

// Colour chosen in Preferences (stored as "#rrggbb"), or `fallback`.
PopplerColor annotation_color(Settings &settings, const char *key, const char *fallback) {
    Gdk::RGBA rgba;
    if (!rgba.set(settings.get_string("annotations", key, fallback))) rgba.set(fallback);
    PopplerColor color;
    color.red = static_cast<guint16>(rgba.get_red() * 65535.0 + 0.5);
    color.green = static_cast<guint16>(rgba.get_green() * 65535.0 + 0.5);
    color.blue = static_cast<guint16>(rgba.get_blue() * 65535.0 + 0.5);
    return color;
}

}  // namespace

// ---------------------------------------------------------------------------
// MarkupTool
// ---------------------------------------------------------------------------

MarkupTool::MarkupTool(AppContext &ctx, bool underline)
    : ctx_(ctx), underline_(underline) {}

bool MarkupTool::on_press(PageView &, const PointerEvent &event) {
    dragging_ = true;
    x0_ = x1_ = event.x;
    y0_ = y1_ = event.y;
    return true;
}

bool MarkupTool::on_motion(PageView &view, const PointerEvent &event) {
    if (!dragging_) return false;
    x1_ = event.x;
    y1_ = event.y;
    view.area().queue_draw();
    return true;
}

bool MarkupTool::on_release(PageView &view, const PointerEvent &event) {
    if (!dragging_ || event.button != 1) return false;
    dragging_ = false;
    x1_ = event.x;
    y1_ = event.y;
    // A drag along one line of text is nearly horizontal, so only its length counts.
    if (std::hypot(x1_ - x0_, y1_ - y0_) > 3) {
        add_annotation(view);
    } else {
        view.signal_message.emit("Nothing marked: press, drag across the text, then release.");
        view.area().queue_draw();
    }
    return true;
}

void MarkupTool::deactivate(PageView &) { dragging_ = false; }

void MarkupTool::paint(PageView &, const Cairo::RefPtr<Cairo::Context> &cr) {
    if (!dragging_) return;
    cr->rectangle(std::min(x0_, x1_), std::min(y0_, y1_), std::abs(x1_ - x0_),
                  std::abs(y1_ - y0_));
    cr->set_source_rgba(0.2, 0.5, 1.0, 0.25);
    cr->fill_preserve();
    cr->set_source_rgba(0.2, 0.5, 1.0, 0.9);
    cr->set_line_width(1.0);
    cr->stroke();
}

void MarkupTool::add_annotation(PageView &view) {
    PagePtr page = ctx_.model.current_page_object();
    if (!page) return;

    // Follow the text: one quadrilateral per selected line, so the highlight
    // hugs the words. On a page with no text under the drag (a scan, a picture)
    // fall back to the dragged rectangle itself.
    double page_w = 0, page_h = 0;
    poppler_page_get_size(page.get(), &page_w, &page_h);
    const double s = view.scale();
    const text_select::Selection snapped =
        text_select::select(page.get(), x0_ / s, y0_ / s, x1_ / s, y1_ / s);

    GArray *quads = g_array_new(FALSE, TRUE, sizeof(PopplerQuadrilateral));
    PopplerRectangle rect{0, 0, 0, 0};
    auto add_quad = [&](double left, double bottom, double right, double top) {
        PopplerQuadrilateral quad;
        quad.p1 = PopplerPoint{left, top};      // top-left
        quad.p2 = PopplerPoint{right, top};     // top-right
        quad.p3 = PopplerPoint{left, bottom};   // bottom-left
        quad.p4 = PopplerPoint{right, bottom};  // bottom-right
        g_array_append_val(quads, quad);
        if (quads->len == 1) {
            rect = PopplerRectangle{left, bottom, right, top};
        } else {
            rect.x1 = std::min(rect.x1, left);
            rect.y1 = std::min(rect.y1, bottom);
            rect.x2 = std::max(rect.x2, right);
            rect.y2 = std::max(rect.y2, top);
        }
    };
    if (!snapped.empty()) {
        // The selection is measured from the top of the page; annotations from the bottom.
        for (const PopplerRectangle &line : snapped.lines) {
            add_quad(line.x1, page_h - line.y2, line.x2, page_h - line.y1);
        }
    } else {
        double ax, ay, bx, by;
        view.to_pdf(x0_, y0_, ax, ay);
        view.to_pdf(x1_, y1_, bx, by);
        if (std::abs(ax - bx) < 2 || std::abs(ay - by) < 2) {
            g_array_free(quads, TRUE);
            view.signal_message.emit("No text there: drag across some text, or drag a larger area.");
            return;
        }
        add_quad(std::min(ax, bx), std::min(ay, by), std::max(ax, bx), std::max(ay, by));
    }

    PopplerAnnot *annot =
        underline_
            ? poppler_annot_text_markup_new_underline(ctx_.model.poppler(), &rect, quads)
            : poppler_annot_text_markup_new_highlight(ctx_.model.poppler(), &rect, quads);
    g_array_free(quads, TRUE);
    if (!annot) {
        g_warning("efpdf: poppler could not create the markup annotation");
        ui::show_error(ctx_.window, "Could not create the annotation.");
        return;
    }

    PopplerColor color = underline_ ? annotation_color(ctx_.settings, "underline_color", "#ff0000")
                                    : annotation_color(ctx_.settings, "highlight_color", "#ffff00");
    poppler_annot_set_color(annot, &color);
    poppler_page_add_annot(page.get(), annot);
    g_object_unref(annot);
    report_added(view, underline_ ? "Underline" : "Highlight", page.get());
    ctx_.model.notify_modified();
}

// ---------------------------------------------------------------------------
// NoteTool
// ---------------------------------------------------------------------------

bool NoteTool::on_press(PageView &view, const PointerEvent &event) {
    PagePtr page = ctx_.model.current_page_object();
    if (!page) return false;

    std::string text;
    if (!ui::prompt_text(ctx_.window, "Add note", "Note text:", text, true) ||
        text.empty()) {
        return true;
    }

    PopplerRectangle rect{event.pdf_x, event.pdf_y - 24.0, event.pdf_x + 24.0,
                          event.pdf_y};
    PopplerAnnot *annot = poppler_annot_text_new(ctx_.model.poppler(), &rect);
    if (!annot) {
        g_warning("efpdf: poppler could not create the note annotation");
        ui::show_error(ctx_.window, "Could not create the note.");
        return true;
    }
    poppler_annot_set_contents(annot, text.c_str());
    PopplerColor color = annotation_color(ctx_.settings, "note_color", "#ffff00");
    poppler_annot_set_color(annot, &color);
    poppler_page_add_annot(page.get(), annot);
    g_object_unref(annot);
    report_added(view, "Note", page.get());
    ctx_.model.notify_modified();
    return true;
}

// ---------------------------------------------------------------------------
// Annotations feature
// ---------------------------------------------------------------------------

Annotations::Annotations(AppContext &ctx) : ctx_(ctx) {
    ctx.tools.emplace<MarkupTool>(ctx, false);
    ctx.tools.emplace<MarkupTool>(ctx, true);
    ctx.tools.emplace<NoteTool>(ctx);
    ctx.view.signal_secondary_click.connect(
        sigc::mem_fun(*this, &Annotations::delete_at));
}

// Right-click removes a markup or note annotation under the pointer.
void Annotations::delete_at(const PointerEvent &event) {
    PagePtr page = ctx_.model.current_page_object();
    if (!page) return;

    GList *mappings = poppler_page_get_annot_mapping(page.get());
    bool changed = false;
    for (GList *l = mappings; l; l = l->next) {
        auto *m = static_cast<PopplerAnnotMapping *>(l->data);
        const PopplerAnnotType type = poppler_annot_get_annot_type(m->annot);
        if (type == POPPLER_ANNOT_WIDGET || type == POPPLER_ANNOT_LINK) continue;
        if (!inside(m->area, event.pdf_x, event.pdf_y)) continue;

        if (ui::ask_yes_no(ctx_.window, "Delete this annotation?")) {
            poppler_page_remove_annot(page.get(), m->annot);
            changed = true;
        }
        break;
    }
    poppler_page_free_annot_mapping(mappings);
    if (changed) ctx_.model.notify_modified();
}

}  // namespace efpdf
