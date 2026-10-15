#include "features/dark_mode.h"

#include "core/settings.h"
#include "ui/menu_builder.h"
#include "view/page_view.h"

namespace efpdf {
namespace {

// An explicit dark stylesheet, so the window goes dark even on themes that
// ignore GTK's "prefer dark theme" setting.
const char *kDarkCss = R"CSS(
window, dialog, menubar, menu, menuitem, toolbar, paned, viewport, scrolledwindow,
list, row, label, button, entry, checkbutton, combobox, box {
    background-color: #2b2b2b;
    background-image: none;
    color: #e8e8e8;
}
button, entry, combobox {
    border: 1px solid #555555;
}
entry {
    background-color: #3a3a3a;
}
menuitem:hover, button:hover {
    background-color: #454545;
}
row:selected {
    background-color: #3d6fb4;
}
)CSS";

}  // namespace

DarkMode::DarkMode(AppContext &ctx) : ctx_(ctx) {
    item_ = ctx.menus.add_check_item("_View", "_Dark Mode",
                                     sigc::mem_fun(*this, &DarkMode::apply), GDK_KEY_d,
                                     Gdk::CONTROL_MASK);
    // Restoring the saved choice toggles the item, which calls apply().
    if (ctx.settings.get_bool("view", "dark", false)) item_->set_active(true);
}

DarkMode::~DarkMode() {
    if (css_installed_) {
        Gtk::StyleContext::remove_provider_for_screen(Gdk::Screen::get_default(), css_);
    }
}

void DarkMode::apply() {
    const bool dark = item_->get_active();

    // 1. The page itself.
    ctx_.view.set_dark(dark);

    // 2. The window: GTK's own dark variant, plus our explicit stylesheet.
    Gtk::Settings::get_default()->property_gtk_application_prefer_dark_theme() = dark;
    if (dark && !css_installed_) {
        try {
            if (!css_) {
                css_ = Gtk::CssProvider::create();
                css_->load_from_data(kDarkCss);
            }
            Gtk::StyleContext::add_provider_for_screen(
                Gdk::Screen::get_default(), css_, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
            css_installed_ = true;
        } catch (const Glib::Error &) {
            // Keep going: the inverted page and theme preference still apply.
        }
    } else if (!dark && css_installed_) {
        Gtk::StyleContext::remove_provider_for_screen(Gdk::Screen::get_default(), css_);
        css_installed_ = false;
    }

    ctx_.settings.set_bool("view", "dark", dark);
}

}  // namespace efpdf
