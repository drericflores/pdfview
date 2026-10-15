#include "features/edit_commands.h"

#include "core/document_model.h"
#include "core/history.h"
#include "ui/menu_builder.h"
#include "ui/text_undo.h"
#include "view/page_view.h"

namespace efpdf {

EditCommands::EditCommands(AppContext &ctx) : ctx_(ctx) {
    const Gdk::ModifierType ctrl = Gdk::CONTROL_MASK;
    const Gdk::ModifierType ctrl_shift = Gdk::CONTROL_MASK | Gdk::SHIFT_MASK;
    auto &menus = ctx.menus;

    menus.add_item("_Edit", "_Undo", [this] { undo(); }, GDK_KEY_z, ctrl);
    menus.add_item("_Edit", "_Redo", [this] { redo(); }, GDK_KEY_z, ctrl_shift);
    menus.add_separator("_Edit");
    menus.add_item("_Edit", "Cu_t", [this] { cut(); }, GDK_KEY_x, ctrl);
    menus.add_item("_Edit", "_Copy", [this] { copy(); }, GDK_KEY_c, ctrl);
    menus.add_item("_Edit", "_Paste", [this] { paste(); }, GDK_KEY_v, ctrl);
    menus.add_separator("_Edit");

    // Ctrl+Y is the other common spelling of Redo.
    ctx.window.signal_key_press_event().connect(
        [this](GdkEventKey *e) {
            const guint mods = e->state & gtk_accelerator_get_default_mod_mask();
            if (mods == GDK_CONTROL_MASK && (e->keyval == GDK_KEY_y || e->keyval == GDK_KEY_Y)) {
                redo();
                return true;
            }
            return false;
        },
        false);

    // Edits belong to one document; a different file starts a fresh history.
    ctx.model.signal_loaded.connect([&ctx] { ctx.history.clear(); });
    ctx.model.signal_closed.connect([&ctx] { ctx.history.clear(); });
}

void EditCommands::say(const Glib::ustring &message) { ctx_.view.signal_message.emit(message); }

void EditCommands::undo() {
    if (TextUndo *text = TextUndo::from(ctx_.window.get_focus())) {
        if (!text->undo()) say("Nothing to undo in this field.");
        return;
    }
    if (!ctx_.history.can_undo()) {
        say("Nothing to undo.");
        return;
    }
    const std::string label = ctx_.history.undo_label();
    ctx_.history.undo();
    say("Undid: " + label);
}

void EditCommands::redo() {
    if (TextUndo *text = TextUndo::from(ctx_.window.get_focus())) {
        if (!text->redo()) say("Nothing to redo in this field.");
        return;
    }
    if (!ctx_.history.can_redo()) {
        say("Nothing to redo.");
        return;
    }
    const std::string label = ctx_.history.redo_label();
    ctx_.history.redo();
    say("Redid: " + label);
}

void EditCommands::cut() {
    Gtk::Widget *focus = ctx_.window.get_focus();
    if (auto *editable = dynamic_cast<Gtk::Editable *>(focus)) {
        editable->cut_clipboard();
    } else if (auto *view = dynamic_cast<Gtk::TextView *>(focus)) {
        view->get_buffer()->cut_clipboard(Gtk::Clipboard::get(), view->get_editable());
    } else {
        say("Cut works inside a text field: click a form field with the Fill forms tool first.");
    }
}

void EditCommands::copy() {
    Gtk::Widget *focus = ctx_.window.get_focus();
    if (auto *editable = dynamic_cast<Gtk::Editable *>(focus)) {
        editable->copy_clipboard();
    } else if (auto *view = dynamic_cast<Gtk::TextView *>(focus)) {
        view->get_buffer()->copy_clipboard(Gtk::Clipboard::get());
    } else {
        say("Copy works inside a text field: click a form field with the Fill forms tool first.");
    }
}

void EditCommands::paste() {
    Gtk::Widget *focus = ctx_.window.get_focus();
    if (auto *editable = dynamic_cast<Gtk::Editable *>(focus)) {
        editable->paste_clipboard();
    } else if (auto *view = dynamic_cast<Gtk::TextView *>(focus)) {
        view->get_buffer()->paste_clipboard(Gtk::Clipboard::get(), view->get_editable());
    } else {
        say("Paste works inside a text field: click a form field with the Fill forms tool first.");
    }
}

}  // namespace efpdf
