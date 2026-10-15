// tool_selector.h - the toolbar drop-down that chooses the active mouse tool.
//
// Construct it after every feature that registers a tool, so the list is full.
#pragma once

#include "features/feature.h"

namespace efpdf {

class ToolSelector : public Feature {
public:
    explicit ToolSelector(AppContext &ctx);

private:
    void on_changed();

    AppContext &ctx_;
    Gtk::ComboBoxText combo_;
};

}  // namespace efpdf
