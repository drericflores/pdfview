// dark_mode.h - View > Dark Mode: inverts the page and uses GTK's dark theme.
#pragma once

#include "features/feature.h"

namespace efpdf {

class DarkMode : public Feature {
public:
    explicit DarkMode(AppContext &ctx);

private:
    void apply();

    AppContext &ctx_;
    Gtk::CheckMenuItem *item_ = nullptr;
};

}  // namespace efpdf
