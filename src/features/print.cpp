#include "features/print.h"

#include <poppler.h>

#include <algorithm>

#include "core/document_model.h"
#include "core/util.h"
#include "ui/dialogs.h"
#include "ui/menu_builder.h"

namespace efpdf {

Print::Print(AppContext &ctx) : ctx_(ctx) {
    // Position 4 puts it after "Save As...", before "Close".
    ctx.menus.insert_item("_File", 4, "_Print...", [this] { print(); }, GDK_KEY_p,
                          Gdk::CONTROL_MASK);
}

void Print::print() {
    DocumentModel &model = ctx_.model;
    if (!model.is_open()) return;

    auto op = Gtk::PrintOperation::create();
    op->set_job_name(util::base_name(model.path()));
    op->set_n_pages(model.page_count());
    if (settings_) op->set_print_settings(settings_);

    op->signal_draw_page().connect(
        [this](const Glib::RefPtr<Gtk::PrintContext> &context, int page_nr) {
            PagePtr page = ctx_.model.page(page_nr);
            if (!page) return;
            double w = 0, h = 0;
            poppler_page_get_size(page.get(), &w, &h);
            if (w <= 0 || h <= 0) return;

            // Fit the page inside the printable area, keeping its proportions.
            auto cr = context->get_cairo_context();
            const double s = std::min(context->get_width() / w, context->get_height() / h);
            cr->scale(s, s);
            poppler_page_render_for_printing(page.get(), cr->cobj());
        });

    try {
        const Gtk::PrintOperationResult result =
            op->run(Gtk::PRINT_OPERATION_ACTION_PRINT_DIALOG, ctx_.window);
        if (result == Gtk::PRINT_OPERATION_RESULT_APPLY) {
            settings_ = op->get_print_settings();
        }
    } catch (const Glib::Error &e) {
        ui::show_error(ctx_.window, "Printing failed: " + e.what());
    }
}

}  // namespace efpdf
