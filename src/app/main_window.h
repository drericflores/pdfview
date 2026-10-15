// main_window.h - the application shell.
//
// This class owns the shared objects (settings, document model, page view,
// menu builder) and the window layout, then constructs each feature in turn.
// It contains no feature logic of its own: removing a feature means deleting
// its line in main_window.cpp.
#pragma once

#include <gtkmm.h>

#include <memory>
#include <string>
#include <vector>

#include "core/document_model.h"
#include "core/history.h"
#include "core/settings.h"
#include "features/feature.h"
#include "ui/menu_builder.h"
#include "view/page_view.h"
#include "view/tool_registry.h"

namespace efpdf {

class FileCommands;

class MainWindow : public Gtk::Window {
public:
    MainWindow();
    ~MainWindow() override;

    // Opens `path` (as if chosen from File > Open). Returns false on failure.
    bool open_file(const std::string &path);

protected:
    bool on_delete_event(GdkEventAny *event) override;
    void on_hide() override;

private:
    void update_title();

    // Declaration order matters: later members use earlier ones.
    Settings settings_;
    DocumentModel model_;
    History history_;
    Glib::RefPtr<Gtk::AccelGroup> accel_;
    Gtk::Box root_{Gtk::ORIENTATION_VERTICAL};
    Gtk::MenuBar menubar_;
    MenuBuilder menus_;
    Gtk::Box toolbar_{Gtk::ORIENTATION_HORIZONTAL, 4};
    Gtk::Box top_area_{Gtk::ORIENTATION_VERTICAL, 0};
    Gtk::Paned paned_{Gtk::ORIENTATION_HORIZONTAL};
    Gtk::Box sidebar_{Gtk::ORIENTATION_VERTICAL, 0};
    PageView view_;
    ToolRegistry tools_;
    AppContext ctx_;

    std::vector<std::unique_ptr<Feature>> features_;
    FileCommands *files_ = nullptr;
};

}  // namespace efpdf
