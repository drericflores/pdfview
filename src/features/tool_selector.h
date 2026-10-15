// tool_selector.h - toolbar buttons that choose the active mouse tool, and a
// status line that explains how to use it.
//
// Construct it after every feature that registers a tool, so the list is full.
#pragma once

#include <vector>

#include "features/feature.h"

namespace efpdf {

class ToolSelector : public Feature {
public:
    explicit ToolSelector(AppContext &ctx);

private:
    void select(size_t index);

    AppContext &ctx_;
    Gtk::Box buttons_{Gtk::ORIENTATION_HORIZONTAL, 0};
    Gtk::Label caption_{"Tool:"};
    Gtk::Label hint_;
    std::vector<Gtk::RadioButton *> radios_;
};

}  // namespace efpdf
