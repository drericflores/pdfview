// appearance.h - the look of the program: colors, spacing and styling.
//
// Loads one application-wide GTK stylesheet and tags the main areas of the
// window (toolbar, sidebar, page area, strip under the toolbar) with style
// classes the stylesheet targets. The palette follows GTK's "prefer dark
// theme" setting, which View > Dark Mode changes, so the two stay in step.
//
// Start the program with EFPDF_PLAIN=1 in the environment to skip all of this
// and get the plain system theme (useful for comparing, or if a theme clashes).
#pragma once

#include "features/feature.h"

namespace efpdf {

class Appearance : public Feature {
public:
    explicit Appearance(AppContext &ctx);
    ~Appearance() override;

private:
    void apply();

    AppContext &ctx_;
    Glib::RefPtr<Gtk::CssProvider> css_;
    sigc::connection dark_changed_;
    bool installed_ = false;
};

}  // namespace efpdf
