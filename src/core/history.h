// history.h - undo/redo stack for document edits.
//
// A feature that changes the document in memory pushes a Command describing
// how to reverse and repeat the change. The Edit menu (features/edit_commands)
// drives the stack. The history is cleared whenever a document is loaded or
// closed, because commands refer to positions in one particular document.
#pragma once

#include <sigc++/sigc++.h>

#include <functional>
#include <string>
#include <vector>

namespace efpdf {

struct Command {
    std::string label;             // shown to the user, e.g. "Fill in field"
    std::function<void()> undo;
    std::function<void()> redo;
};

class History {
public:
    // Records a change that has already been applied. Drops any redo steps.
    void push(Command command);

    bool can_undo() const { return !undo_.empty(); }
    bool can_redo() const { return !redo_.empty(); }
    const std::string &undo_label() const { return undo_.back().label; }
    const std::string &redo_label() const { return redo_.back().label; }

    // Each returns false when there is nothing to do.
    bool undo();
    bool redo();
    void clear();

    sigc::signal<void> signal_changed;

private:
    std::vector<Command> undo_;
    std::vector<Command> redo_;
};

}  // namespace efpdf
