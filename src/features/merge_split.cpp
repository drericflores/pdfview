#include "features/merge_split.h"

#include <string>
#include <vector>

#include "core/document_model.h"
#include "core/pdf_ops.h"
#include "core/util.h"
#include "features/file_commands.h"
#include "ui/dialogs.h"
#include "ui/menu_builder.h"

namespace efpdf {
namespace {

bool ask_page_range(Gtk::Window &parent, int page_count, int current, int &first,
                    int &last) {
    Gtk::Dialog dlg("Extract pages", parent, true);
    dlg.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    dlg.add_button("_Extract", Gtk::RESPONSE_OK);

    Gtk::Grid grid;
    grid.set_row_spacing(6);
    grid.set_column_spacing(8);
    Gtk::Label first_label("First page:"), last_label("Last page:");
    Gtk::SpinButton first_spin(Gtk::Adjustment::create(current + 1, 1, page_count, 1));
    Gtk::SpinButton last_spin(Gtk::Adjustment::create(current + 1, 1, page_count, 1));
    grid.attach(first_label, 0, 0);
    grid.attach(first_spin, 1, 0);
    grid.attach(last_label, 0, 1);
    grid.attach(last_spin, 1, 1);
    dlg.get_content_area()->pack_start(grid, Gtk::PACK_SHRINK, 8);
    dlg.show_all_children();

    if (dlg.run() != Gtk::RESPONSE_OK) return false;
    first = first_spin.get_value_as_int() - 1;
    last = last_spin.get_value_as_int() - 1;
    return true;
}

}  // namespace

MergeSplit::MergeSplit(AppContext &ctx, FileCommands &files)
    : ctx_(ctx), files_(files) {
    ctx.menus.add_item("_Edit", "_Extract Pages...",
                       sigc::mem_fun(*this, &MergeSplit::extract_pages));
    ctx.menus.add_item("_Edit", "_Merge PDFs...",
                       sigc::mem_fun(*this, &MergeSplit::merge));
}

void MergeSplit::extract_pages() {
    DocumentModel &model = ctx_.model;
    if (!model.is_open()) return;

    int first = 0, last = 0;
    if (!ask_page_range(ctx_.window, model.page_count(), model.current_page(), first,
                        last)) {
        return;
    }

    std::string dest;
    if (!ui::choose_save_pdf(ctx_.window, "Save extracted pages", dest, "extract.pdf")) {
        return;
    }

    std::string in, err;
    if (!model.snapshot(in, err)) {
        ui::show_error(ctx_.window, "Could not prepare the document: " + err);
        return;
    }
    const bool ok = pdf_ops::extract_range(in, dest, first, last, model.password(), err);
    util::remove_file(in);
    if (!ok) ui::show_error(ctx_.window, "Could not extract pages: " + err);
}

void MergeSplit::merge() {
    std::vector<std::string> inputs;
    if (!ui::choose_open_pdfs(ctx_.window, "Select PDFs to merge (merged in listed order)",
                              inputs)) {
        return;
    }
    std::string dest;
    if (!ui::choose_save_pdf(ctx_.window, "Save merged PDF", dest, "merged.pdf")) return;

    std::string err;
    if (!pdf_ops::merge(inputs, dest, err)) {
        ui::show_error(ctx_.window, "Could not merge: " + err);
        return;
    }
    if (ui::ask_yes_no(ctx_.window, "Merged PDF saved. Open it now?")) {
        files_.open_path(dest);
    }
}

}  // namespace efpdf
