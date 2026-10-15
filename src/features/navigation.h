// navigation.h - previous/next buttons, the page-jump box and page shortcuts.
#pragma once

#include "features/feature.h"

namespace efpdf {

class Navigation : public Feature {
public:
    explicit Navigation(AppContext &ctx);

private:
    void update_display();
    void jump_from_entry();
    void focus_entry();

    AppContext &ctx_;
    Gtk::Entry entry_;
    Gtk::Label total_label_;
};

}  // namespace efpdf
