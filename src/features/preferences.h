// preferences.h - Edit > Preferences...: lets the user configure the program.
//
// Everything is stored in ~/.config/efpdf/settings.ini through Settings, and
// changes take effect immediately (other features listen to
// Settings::signal_changed or read the value when they need it).
#pragma once

#include "features/feature.h"

namespace efpdf {

class Preferences : public Feature {
public:
    explicit Preferences(AppContext &ctx);
    ~Preferences() override;

private:
    void show_dialog();

    AppContext &ctx_;
};

}  // namespace efpdf
