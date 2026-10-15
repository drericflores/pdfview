#include "ui/dialogs.h"

namespace efpdf::ui {
namespace {

Glib::RefPtr<Gtk::FileFilter> pdf_filter() {
    auto filter = Gtk::FileFilter::create();
    filter->set_name("PDF files");
    filter->add_mime_type("application/pdf");
    filter->add_pattern("*.pdf");
    filter->add_pattern("*.PDF");
    return filter;
}

}  // namespace

void show_error(Gtk::Window &parent, const Glib::ustring &message) {
    Gtk::MessageDialog dlg(parent, message, false, Gtk::MESSAGE_ERROR,
                           Gtk::BUTTONS_OK, true);
    dlg.run();
}

void show_info(Gtk::Window &parent, const Glib::ustring &message) {
    Gtk::MessageDialog dlg(parent, message, false, Gtk::MESSAGE_INFO,
                           Gtk::BUTTONS_OK, true);
    dlg.run();
}

bool ask_yes_no(Gtk::Window &parent, const Glib::ustring &message) {
    Gtk::MessageDialog dlg(parent, message, false, Gtk::MESSAGE_QUESTION,
                           Gtk::BUTTONS_YES_NO, true);
    return dlg.run() == Gtk::RESPONSE_YES;
}

SaveChoice ask_save_changes(Gtk::Window &parent, const std::string &file_name) {
    Gtk::MessageDialog dlg(parent, "Save changes to " + file_name + "?", false,
                           Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_NONE, true);
    dlg.set_secondary_text("Your edits will be lost if you don't save them.");
    dlg.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    dlg.add_button("_Discard", Gtk::RESPONSE_REJECT);
    dlg.add_button("_Save", Gtk::RESPONSE_ACCEPT);
    switch (dlg.run()) {
        case Gtk::RESPONSE_ACCEPT: return SaveChoice::Save;
        case Gtk::RESPONSE_REJECT: return SaveChoice::Discard;
        default: return SaveChoice::Cancel;
    }
}

bool choose_open_pdf(Gtk::Window &parent, const Glib::ustring &title,
                     std::string &result) {
    Gtk::FileChooserDialog dlg(title, Gtk::FILE_CHOOSER_ACTION_OPEN);
    dlg.set_transient_for(parent);
    dlg.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    dlg.add_button("_Open", Gtk::RESPONSE_ACCEPT);
    dlg.add_filter(pdf_filter());
    if (dlg.run() != Gtk::RESPONSE_ACCEPT) return false;
    result = dlg.get_filename();
    return !result.empty();
}

bool choose_open_pdfs(Gtk::Window &parent, const Glib::ustring &title,
                      std::vector<std::string> &result) {
    Gtk::FileChooserDialog dlg(title, Gtk::FILE_CHOOSER_ACTION_OPEN);
    dlg.set_transient_for(parent);
    dlg.set_select_multiple(true);
    dlg.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    dlg.add_button("_Open", Gtk::RESPONSE_ACCEPT);
    dlg.add_filter(pdf_filter());
    if (dlg.run() != Gtk::RESPONSE_ACCEPT) return false;
    result = dlg.get_filenames();
    return !result.empty();
}

bool choose_save_pdf(Gtk::Window &parent, const Glib::ustring &title,
                     std::string &result, const std::string &suggested_name) {
    Gtk::FileChooserDialog dlg(title, Gtk::FILE_CHOOSER_ACTION_SAVE);
    dlg.set_transient_for(parent);
    dlg.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    dlg.add_button("_Save", Gtk::RESPONSE_ACCEPT);
    dlg.add_filter(pdf_filter());
    dlg.set_do_overwrite_confirmation(true);
    if (!suggested_name.empty()) dlg.set_current_name(suggested_name);
    if (dlg.run() != Gtk::RESPONSE_ACCEPT) return false;
    result = dlg.get_filename();
    return !result.empty();
}

bool choose_folder(Gtk::Window &parent, const Glib::ustring &title,
                   std::string &result) {
    Gtk::FileChooserDialog dlg(title, Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
    dlg.set_transient_for(parent);
    dlg.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    dlg.add_button("_Select", Gtk::RESPONSE_ACCEPT);
    if (dlg.run() != Gtk::RESPONSE_ACCEPT) return false;
    result = dlg.get_filename();
    return !result.empty();
}

bool prompt_text(Gtk::Window &parent, const Glib::ustring &title,
                 const Glib::ustring &label, std::string &value, bool multiline,
                 bool secret) {
    Gtk::Dialog dlg(title, parent, true);
    dlg.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    dlg.add_button("_OK", Gtk::RESPONSE_OK);
    dlg.set_default_response(Gtk::RESPONSE_OK);

    Gtk::Box *content = dlg.get_content_area();
    content->set_spacing(6);
    Gtk::Label caption(label);
    caption.set_halign(Gtk::ALIGN_START);
    content->pack_start(caption, Gtk::PACK_SHRINK);

    Gtk::Entry entry;
    Gtk::TextView text_view;
    Gtk::ScrolledWindow scroller;
    if (multiline) {
        text_view.get_buffer()->set_text(value);
        text_view.set_wrap_mode(Gtk::WRAP_WORD);
        scroller.add(text_view);
        scroller.set_size_request(360, 140);
        content->pack_start(scroller, Gtk::PACK_EXPAND_WIDGET);
    } else {
        entry.set_text(value);
        entry.set_activates_default(true);
        entry.set_visibility(!secret);
        content->pack_start(entry, Gtk::PACK_SHRINK);
    }
    dlg.show_all_children();

    if (dlg.run() != Gtk::RESPONSE_OK) return false;
    value = multiline ? std::string(text_view.get_buffer()->get_text())
                      : std::string(entry.get_text());
    return true;
}

}  // namespace efpdf::ui
