// dialogs.h - reusable modal dialogs. Features call these instead of building
// their own, so every dialog looks and behaves the same.
#pragma once

#include <gtkmm.h>

#include <functional>
#include <string>
#include <vector>

namespace efpdf::ui {

// Lets Preferences decide which folder the file choosers start in. `start`
// returns a folder (or "" for GTK's default); `used` is told the folder of
// every file the user picks.
struct FolderHooks {
    std::function<std::string()> start;
    std::function<void(const std::string &)> used;
};
void set_folder_hooks(FolderHooks hooks);

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

// Asks for an existing folder (the dialog can also create one).
bool choose_folder(Gtk::Window &parent, const Glib::ustring &title,
                   std::string &result);

// Asks for a line (or, with `multiline`, a paragraph) of text. With `secret`
// the entry hides what is typed, for passwords.
bool prompt_text(Gtk::Window &parent, const Glib::ustring &title,
                 const Glib::ustring &label, std::string &value,
                 bool multiline = false, bool secret = false);

}  // namespace efpdf::ui
