// dialogs.h - reusable modal dialogs. Features call these instead of building
// their own, so every dialog looks and behaves the same.
#pragma once

#include <gtkmm.h>

#include <string>
#include <vector>

namespace efpdf::ui {

void show_error(Gtk::Window &parent, const Glib::ustring &message);
void show_info(Gtk::Window &parent, const Glib::ustring &message);
bool ask_yes_no(Gtk::Window &parent, const Glib::ustring &message);

enum class SaveChoice { Save, Discard, Cancel };
SaveChoice ask_save_changes(Gtk::Window &parent, const std::string &file_name);

bool choose_open_pdf(Gtk::Window &parent, const Glib::ustring &title,
                     std::string &result);
bool choose_open_pdfs(Gtk::Window &parent, const Glib::ustring &title,
                      std::vector<std::string> &result);
bool choose_save_pdf(Gtk::Window &parent, const Glib::ustring &title,
                     std::string &result, const std::string &suggested_name);

// Asks for a line (or, with `multiline`, a paragraph) of text. With `secret`
// the entry hides what is typed, for passwords.
bool prompt_text(Gtk::Window &parent, const Glib::ustring &title,
                 const Glib::ustring &label, std::string &value,
                 bool multiline = false, bool secret = false);

}  // namespace efpdf::ui
