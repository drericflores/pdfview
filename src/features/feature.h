// feature.h - the contract between the window shell and its features.
//
// Every feature is a class derived from Feature. Its constructor receives the
// AppContext and installs whatever it contributes: menu items, toolbar widgets,
// mouse tools, or panels. Features never include each other's headers (the two
// that need to open files take a FileCommands reference explicitly), so any
// feature can be removed by deleting one line in main_window.cpp.
#pragma once

#include <gtkmm.h>

namespace efpdf {

class DocumentModel;
class History;
class MenuBuilder;
class PageView;
class Settings;
class ToolRegistry;

struct AppContext {
    Gtk::Window &window;
    DocumentModel &model;
    PageView &view;
    MenuBuilder &menus;
    Gtk::Box &toolbar;   // row of buttons under the menu bar
    Gtk::Box &top_area;  // strips between toolbar and page (e.g. the search bar)
    Gtk::Box &sidebar;   // left panel (e.g. thumbnails)
    Settings &settings;
    ToolRegistry &tools;
    History &history;     // undo/redo of document edits
};

class Feature {
public:
    virtual ~Feature() = default;
};

}  // namespace efpdf
