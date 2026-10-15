#include "features/annotations.h"

#include <algorithm>
#include <cmath>
#include <string>

#include "core/document_model.h"
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
    if (std::abs(x1_ - x0_) > 3 && std::abs(y1_ - y0_) > 3) {
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

    double ax, ay, bx, by;
    view.to_pdf(x0_, y0_, ax, ay);
    view.to_pdf(x1_, y1_, bx, by);
    PopplerRectangle rect{std::min(ax, bx), std::min(ay, by), std::max(ax, bx),
                          std::max(ay, by)};

    GArray *quads = g_array_new(FALSE, TRUE, sizeof(PopplerQuadrilateral));
    PopplerQuadrilateral quad;
    quad.p1 = PopplerPoint{rect.x1, rect.y2};  // top-left
    quad.p2 = PopplerPoint{rect.x2, rect.y2};  // top-right
    quad.p3 = PopplerPoint{rect.x1, rect.y1};  // bottom-left
    quad.p4 = PopplerPoint{rect.x2, rect.y1};  // bottom-right
    g_array_append_val(quads, quad);

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

    PopplerColor color = underline_ ? PopplerColor{0xffff, 0x0000, 0x0000}
                                    : PopplerColor{0xffff, 0xffff, 0x0000};
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
    PopplerColor color{0xffff, 0xffff, 0x0000};
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
