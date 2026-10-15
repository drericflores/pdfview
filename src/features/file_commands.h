// file_commands.h - Open, Save, Save As, Close, Quit and drag-and-drop.
#pragma once

#include <string>

#include "features/feature.h"

namespace efpdf {

class FileCommands : public Feature {
public:
    explicit FileCommands(AppContext &ctx);

    // Asks about unsaved edits, then opens `path` (prompting for a password if
    // the PDF is encrypted). Returns true if the file is now open.
    bool open_path(const std::string &path);

    // If there are unsaved edits, asks whether to save them. Returns false if
    // the user cancelled and the current action should be abandoned.
    bool confirm_discard();

    bool save();
    bool save_as();
    void close_document();
    void quit();

    // Emitted after a file was opened successfully.
    sigc::signal<void, const std::string &> signal_file_opened;

private:
    void on_open_dialog();
    void on_drag_data(const Glib::RefPtr<Gdk::DragContext> &context, int x, int y,
                      const Gtk::SelectionData &data, guint info, guint time);

    AppContext &ctx_;
};

}  // namespace efpdf
