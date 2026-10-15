// text_undo.h - undo/redo for the text inside one editor widget.
//
// GTK 3's Gtk::Entry and Gtk::TextView have no undo of their own. Attach a
// TextUndo to an editor and the Edit menu can undo typing inside it. The object
// is owned by the widget and is destroyed with it.
#pragma once

#include <gtkmm.h>

#include <cstdint>
#include <string>
#include <vector>

namespace efpdf {

class TextUndo {
public:
    static void attach(Gtk::Entry &entry);
    static void attach(Gtk::TextView &view);

    // The TextUndo attached to `widget`, or nullptr.
    static TextUndo *from(Gtk::Widget *widget);

    bool undo();  // false if nothing to undo
    bool redo();

private:
    TextUndo() = default;
    static void attach_to(Gtk::Widget &widget, TextUndo *undo);
    static void destroy(void *self);
    void record(const std::string &text);
    void apply(const std::string &text);

    sigc::slot<std::string> get_;
    sigc::slot<void, const std::string &> set_;
    std::vector<std::string> states_;
    size_t pos_ = 0;
    bool applying_ = false;
    int64_t last_time_ = 0;
};

}  // namespace efpdf
