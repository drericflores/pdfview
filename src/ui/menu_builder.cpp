#include "ui/menu_builder.h"

namespace efpdf {

MenuBuilder::MenuBuilder(Gtk::MenuBar &bar,
                         const Glib::RefPtr<Gtk::AccelGroup> &accel)
    : bar_(bar), accel_(accel) {}

void MenuBuilder::declare(std::initializer_list<const char *> names) {
    for (const char *name : names) {
        auto *submenu = Gtk::manage(new Gtk::Menu());
        auto *top = Gtk::manage(new Gtk::MenuItem(name, true));
        top->set_submenu(*submenu);
        bar_.append(*top);
        menus_[name] = submenu;
    }
}

Gtk::Menu &MenuBuilder::menu(const std::string &name) {
    auto found = menus_.find(name);
    if (found == menus_.end()) {
        // A feature asked for a menu the window did not declare: create it
        // at the end rather than crash.
        declare({name.c_str()});
        found = menus_.find(name);
    }
    return *found->second;
}

void MenuBuilder::set_accelerator(Gtk::MenuItem &item, guint key,
                                  Gdk::ModifierType mods) {
    if (key != 0) {
        item.add_accelerator("activate", accel_, key, mods, Gtk::ACCEL_VISIBLE);
    }
}

Gtk::MenuItem *MenuBuilder::add_item(const std::string &menu_name, const char *label,
                                     const sigc::slot<void> &slot, guint key,
                                     Gdk::ModifierType mods) {
    auto *item = Gtk::manage(new Gtk::MenuItem(label, true));
    item->signal_activate().connect(slot);
    set_accelerator(*item, key, mods);
    menu(menu_name).append(*item);
    return item;
}

Gtk::CheckMenuItem *MenuBuilder::add_check_item(const std::string &menu_name,
                                                const char *label,
                                                const sigc::slot<void> &on_toggled,
                                                guint key, Gdk::ModifierType mods) {
    auto *item = Gtk::manage(new Gtk::CheckMenuItem(label, true));
    item->signal_toggled().connect(on_toggled);
    set_accelerator(*item, key, mods);
    menu(menu_name).append(*item);
    return item;
}

void MenuBuilder::add_separator(const std::string &menu_name) {
    menu(menu_name).append(*Gtk::manage(new Gtk::SeparatorMenuItem()));
}

Gtk::Menu &MenuBuilder::insert_submenu(const std::string &menu_name,
                                       const char *label, int position) {
    auto *submenu = Gtk::manage(new Gtk::Menu());
    auto *item = Gtk::manage(new Gtk::MenuItem(label, true));
    item->set_submenu(*submenu);
    menu(menu_name).insert(*item, position);
    return *submenu;
}

}  // namespace efpdf
