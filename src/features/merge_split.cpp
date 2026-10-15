#include "features/merge_split.h"

#include <string>
#include <vector>

#include "core/document_model.h"
#include "core/page_ranges.h"
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

// Asks how to divide the document: every N pages, or the page ranges typed in.
// On success `ranges` holds the pieces (0-based, inclusive).
bool ask_split_options(Gtk::Window &parent, int page_count,
                       std::vector<PageRange> &ranges) {
    Gtk::Dialog dlg("Split into separate files", parent, true);
    dlg.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    dlg.add_button("_Split", Gtk::RESPONSE_OK);
    dlg.set_default_response(Gtk::RESPONSE_OK);

    Gtk::Grid grid;
    grid.set_row_spacing(8);
    grid.set_column_spacing(8);
    grid.set_border_width(4);

    Gtk::Label intro("This document has " + std::to_string(page_count) + " pages.");
    intro.set_halign(Gtk::ALIGN_START);

    Gtk::RadioButton count_radio("Make a file for every");
    Gtk::RadioButton range_radio("Make a file for each page range:");
    Gtk::RadioButton::Group group = count_radio.get_group();
    range_radio.set_group(group);

    Gtk::SpinButton per_file(Gtk::Adjustment::create(1, 1, page_count, 1));
    Gtk::Label pages_label("page(s)");
    Gtk::Entry range_entry;
    range_entry.set_placeholder_text("for example 1-3, 4, 7-9");
    range_entry.set_width_chars(26);
    range_entry.set_activates_default(true);
    range_entry.set_sensitive(false);

    grid.attach(intro, 0, 0, 3, 1);
    grid.attach(count_radio, 0, 1);
    grid.attach(per_file, 1, 1);
    grid.attach(pages_label, 2, 1);
    grid.attach(range_radio, 0, 2);
    grid.attach(range_entry, 1, 2, 2, 1);
    dlg.get_content_area()->pack_start(grid, Gtk::PACK_SHRINK, 8);

    auto sync = [&] {
        per_file.set_sensitive(count_radio.get_active());
        range_entry.set_sensitive(range_radio.get_active());
    };
    count_radio.signal_toggled().connect(sync);
    dlg.show_all_children();

    // Stay in the dialog until the input is usable or the user cancels.
    while (dlg.run() == Gtk::RESPONSE_OK) {
        if (count_radio.get_active()) {
            ranges = chunk_ranges(page_count, per_file.get_value_as_int());
            return true;
        }
        std::string err;
        if (parse_page_ranges(range_entry.get_text(), page_count, ranges, err)) {
            return true;
        }
        ui::show_error(dlg, err);
    }
    return false;
}

}  // namespace

MergeSplit::MergeSplit(AppContext &ctx, FileCommands &files)
    : ctx_(ctx), files_(files) {
    ctx.menus.add_item("_Edit", "_Extract Pages...",
                       sigc::mem_fun(*this, &MergeSplit::extract_pages));
    ctx.menus.add_item("_Edit", "_Split into Files...",
                       sigc::mem_fun(*this, &MergeSplit::split_files));
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

void MergeSplit::split_files() {
    DocumentModel &model = ctx_.model;
    if (!model.is_open()) return;
    const int count = model.page_count();
    if (count < 2) {
        ui::show_info(ctx_.window, "This document has only one page, so there is nothing to split.");
        return;
    }

    std::vector<PageRange> ranges;
    if (!ask_split_options(ctx_.window, count, ranges)) return;

    std::string folder;
    if (!ui::choose_folder(ctx_.window, "Choose the folder for the new files", folder)) {
        return;
    }

    // Name the files after the document: report_p001-003.pdf, report_p004.pdf...
    const std::string stem = util::pdf_stem(model.path());
    std::vector<std::string> outputs;
    size_t existing = 0;
    for (const PageRange &range : ranges) {
        outputs.push_back(
            util::join_path(folder, stem + "_" + range_suffix(range, count) + ".pdf"));
        if (util::file_exists(outputs.back())) ++existing;
    }

    std::string question = "Create " + std::to_string(outputs.size()) + " file(s) in " + folder + "?";
    if (existing > 0) {
        question += "\n\n" + std::to_string(existing) +
                    " file(s) with the same names already exist there and will be replaced.";
    }
    if (!ui::ask_yes_no(ctx_.window, question)) return;

    std::string in, err;
    if (!model.snapshot(in, err)) {
        ui::show_error(ctx_.window, "Could not prepare the document: " + err);
        return;
    }
    const bool ok = pdf_ops::split(in, ranges, outputs, model.password(), err);
    util::remove_file(in);
    if (!ok) {
        ui::show_error(ctx_.window, "Could not split the document: " + err);
        return;
    }
    ui::show_info(ctx_.window,
                  "Created " + std::to_string(outputs.size()) + " file(s) in " + folder + ".");
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
