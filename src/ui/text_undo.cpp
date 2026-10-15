#include "ui/text_undo.h"

namespace efpdf {
namespace {
// Typing within this time is merged into one undo step.
constexpr int64_t kMergeMicros = 700 * 1000;
const char *kKey = "efpdf-text-undo";
}  // namespace

void TextUndo::attach_to(Gtk::Widget &widget, TextUndo *undo) {
    widget.set_data(Glib::Quark(kKey), undo, &TextUndo::destroy);
}

void TextUndo::destroy(void *self) { delete static_cast<TextUndo *>(self); }

TextUndo *TextUndo::from(Gtk::Widget *widget) {
    if (!widget) return nullptr;
    return static_cast<TextUndo *>(widget->get_data(Glib::Quark(kKey)));
}

void TextUndo::attach(Gtk::Entry &entry) {
    auto *undo = new TextUndo();
    Gtk::Entry *e = &entry;
    undo->get_ = [e] { return std::string(e->get_text()); };
    undo->set_ = [e](const std::string &text) {
        e->set_text(text);
        e->set_position(-1);
    };
    undo->states_.push_back(entry.get_text());
    entry.signal_changed().connect([undo, e] { undo->record(e->get_text()); });
    attach_to(entry, undo);
}

void TextUndo::attach(Gtk::TextView &view) {
    auto *undo = new TextUndo();
    Glib::RefPtr<Gtk::TextBuffer> buffer = view.get_buffer();
    Gtk::TextBuffer *b = buffer.operator->();
    undo->get_ = [b] { return std::string(b->get_text()); };
    undo->set_ = [b](const std::string &text) { b->set_text(text); };
    undo->states_.push_back(buffer->get_text());
    buffer->signal_changed().connect([undo, b] { undo->record(b->get_text()); });
    attach_to(view, undo);
}

void TextUndo::record(const std::string &text) {
    if (applying_ || text == states_[pos_]) return;
    states_.resize(pos_ + 1);  // a new edit drops the redo steps

    const int64_t now = g_get_monotonic_time();
    if (pos_ > 0 && now - last_time_ < kMergeMicros) {
        states_[pos_] = text;  // still the same burst of typing
    } else {
        states_.push_back(text);
        ++pos_;
    }
    last_time_ = now;
}

void TextUndo::apply(const std::string &text) {
    applying_ = true;
    set_(text);
    applying_ = false;
    last_time_ = 0;  // the next keystroke starts a new step
}

bool TextUndo::undo() {
    if (pos_ == 0) return false;
    --pos_;
    apply(states_[pos_]);
    return true;
}

bool TextUndo::redo() {
    if (pos_ + 1 >= states_.size()) return false;
    ++pos_;
    apply(states_[pos_]);
    return true;
}

}  // namespace efpdf
