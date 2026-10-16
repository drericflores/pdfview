// menu_builder.h - lets each feature add its own menu entries.
//
// The window declares the top-level menus (and their order) once; features
// then append items to them by name, so no feature needs to know about any
// other feature's menu code.
#pragma once

#include <gtkmm.h>

#include <initializer_list>
#include <map>
#include <string>

namespace efpdf {

class MenuBuilder {
public:
    MenuBuilder(Gtk::MenuBar &bar, const Glib::RefPtr<Gtk::AccelGroup> &accel);

    // Creates the top-level menus in the given order, e.g. {"_File", "_Edit"}.
    void declare(std::initializer_list<const char *> names);

    Gtk::MenuItem *add_item(const std::string &menu, const char *label,
                            const sigc::slot<void> &slot, guint key = 0,
                            Gdk::ModifierType mods = Gdk::ModifierType(0));
    Gtk::CheckMenuItem *add_check_item(const std::string &menu, const char *label,
                                       const sigc::slot<void> &on_toggled,
                                       guint key = 0,
                                       Gdk::ModifierType mods = Gdk::ModifierType(0));
    void add_separator(const std::string &menu);

    // Like add_item(), but inserts at `position` (0 = first) instead of appending.
    Gtk::MenuItem *insert_item(const std::string &menu, int position, const char *label,
                               const sigc::slot<void> &slot, guint key = 0,
                               Gdk::ModifierType mods = Gdk::ModifierType(0));

    // Inserts a submenu entry at `position` (0 = first) and returns the submenu.
    Gtk::Menu &insert_submenu(const std::string &menu, const char *label,
                              int position);

private:
    Gtk::Menu &menu(const std::string &name);
    void set_accelerator(Gtk::MenuItem &item, guint key, Gdk::ModifierType mods);

    Gtk::MenuBar &bar_;
    Glib::RefPtr<Gtk::AccelGroup> accel_;
    std::map<std::string, Gtk::Menu *> menus_;
};

}  // namespace efpdf
