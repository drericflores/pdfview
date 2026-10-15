#include "core/history.h"

#include <utility>

namespace efpdf {

void History::push(Command command) {
    undo_.push_back(std::move(command));
    redo_.clear();
    signal_changed.emit();
}

bool History::undo() {
    if (undo_.empty()) return false;
    Command command = std::move(undo_.back());
    undo_.pop_back();
    command.undo();
    redo_.push_back(std::move(command));
    signal_changed.emit();
    return true;
}

bool History::redo() {
    if (redo_.empty()) return false;
    Command command = std::move(redo_.back());
    redo_.pop_back();
    command.redo();
    undo_.push_back(std::move(command));
    signal_changed.emit();
    return true;
}

void History::clear() {
    if (undo_.empty() && redo_.empty()) return;
    undo_.clear();
    redo_.clear();
    signal_changed.emit();
}

}  // namespace efpdf
