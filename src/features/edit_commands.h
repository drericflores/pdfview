// edit_commands.h - Edit > Undo, Redo, Cut, Copy, Paste.
//
// While the cursor is in a text editor (a form field being filled, the search
// box) these act on that editor's text. Otherwise Undo and Redo step through
// the document's edit history (core/history.h).
#pragma once

#include "features/feature.h"

namespace efpdf {

class EditCommands : public Feature {
public:
    explicit EditCommands(AppContext &ctx);

private:
    void undo();
    void redo();
    void cut();
    void copy();
    void paste();
    void select_all();
    void say(const Glib::ustring &message);

    AppContext &ctx_;
};

}  // namespace efpdf
