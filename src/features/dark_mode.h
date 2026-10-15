// dark_mode.h - View > Dark Mode: inverts the page and darkens the window.
#pragma once

#include "features/feature.h"

namespace efpdf {

class DarkMode : public Feature {
public:
    explicit DarkMode(AppContext &ctx);
    ~DarkMode() override;

private:
    void apply();

    AppContext &ctx_;
    Gtk::CheckMenuItem *item_ = nullptr;
};

}  // namespace efpdf
