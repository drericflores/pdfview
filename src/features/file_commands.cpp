#include "features/file_commands.h"

#include "core/document_model.h"
#include "core/util.h"
#include "ui/dialogs.h"
#include "ui/menu_builder.h"

namespace efpdf {

FileCommands::FileCommands(AppContext &ctx) : ctx_(ctx) {
    const Gdk::ModifierType ctrl = Gdk::CONTROL_MASK;
    const Gdk::ModifierType ctrl_shift = Gdk::CONTROL_MASK | Gdk::SHIFT_MASK;

    auto &menus = ctx.menus;
    menus.add_item("_File", "_Open...", sigc::mem_fun(*this, &FileCommands::on_open_dialog),
                   GDK_KEY_o, ctrl);
    menus.add_item("_File", "_Save", [this] { save(); }, GDK_KEY_s, ctrl);
    menus.add_item("_File", "Save _As...", [this] { save_as(); }, GDK_KEY_s, ctrl_shift);
    menus.add_item("_File", "_Close", [this] { close_document(); }, GDK_KEY_w, ctrl);
    menus.add_separator("_File");
    menus.add_item("_File", "_Quit", [this] { quit(); }, GDK_KEY_q, ctrl);

    auto add_button = [&ctx](const char *label, const sigc::slot<void> &slot) {
        auto *button = Gtk::manage(new Gtk::Button(label, true));
        button->signal_clicked().connect(slot);
        ctx.toolbar.pack_start(*button, Gtk::PACK_SHRINK);
    };
    add_button("_Open", sigc::mem_fun(*this, &FileCommands::on_open_dialog));
    add_button("_Save", [this] { save(); });

    // Dropping a PDF onto the window opens it.
    ctx.window.drag_dest_set({Gtk::TargetEntry("text/uri-list")}, Gtk::DEST_DEFAULT_ALL,
                             Gdk::ACTION_COPY);
    ctx.window.signal_drag_data_received().connect(
        sigc::mem_fun(*this, &FileCommands::on_drag_data));
}

void FileCommands::on_open_dialog() {
    std::string path;
    if (ui::choose_open_pdf(ctx_.window, "Open PDF", path)) open_path(path);
}

bool FileCommands::open_path(const std::string &path) {
    if (!confirm_discard()) return false;

    std::string password;
    std::string err;
    for (;;) {
        const LoadResult result = ctx_.model.open(path, password, err);
        if (result == LoadResult::Ok) break;
        if (result == LoadResult::Failed) {
            ui::show_error(ctx_.window, "Could not open the PDF: " + err);
            return false;
        }
        if (!ui::prompt_text(ctx_.window, "Password required",
                             "This PDF is encrypted. Enter its password:", password,
                             false, true)) {
            return false;
        }
    }
    signal_file_opened.emit(path);
    return true;
}

bool FileCommands::confirm_discard() {
    if (!ctx_.model.dirty()) return true;
    switch (ui::ask_save_changes(ctx_.window, util::base_name(ctx_.model.path()))) {
        case ui::SaveChoice::Save: return save();
        case ui::SaveChoice::Discard: return true;
        default: return false;
    }
}

bool FileCommands::save() {
    if (!ctx_.model.is_open()) return false;
    std::string err;
    if (!ctx_.model.save(err)) {
        ui::show_error(ctx_.window, "Could not save: " + err);
        return false;
    }
    return true;
}

bool FileCommands::save_as() {
    if (!ctx_.model.is_open()) return false;
    std::string dest;
    if (!ui::choose_save_pdf(ctx_.window, "Save PDF As", dest,
                             util::base_name(ctx_.model.path()))) {
        return false;
    }
    std::string err;
    if (!ctx_.model.save_as(dest, err)) {
        ui::show_error(ctx_.window, "Could not save: " + err);
        return false;
    }
    return true;
}

void FileCommands::close_document() {
    if (confirm_discard()) ctx_.model.close();
}

void FileCommands::quit() {
    if (confirm_discard()) ctx_.window.hide();
}

void FileCommands::on_drag_data(const Glib::RefPtr<Gdk::DragContext> &context, int,
                                int, const Gtk::SelectionData &data, guint,
                                guint time) {
    bool handled = false;
    try {
        const auto uris = data.get_uris();
        if (!uris.empty()) {
            handled = open_path(Glib::filename_from_uri(uris.front()));
        }
    } catch (const Glib::Error &) {
    }
    context->drag_finish(handled, false, time);
}

}  // namespace efpdf
