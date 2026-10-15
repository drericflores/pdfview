#include "features/dark_mode.h"

#include "core/settings.h"
#include "ui/menu_builder.h"
#include "view/page_view.h"

namespace efpdf {

DarkMode::DarkMode(AppContext &ctx) : ctx_(ctx) {
    item_ = ctx.menus.add_check_item("_View", "_Dark Mode",
                                     sigc::mem_fun(*this, &DarkMode::apply), GDK_KEY_d,
                                     Gdk::CONTROL_MASK);
    // Restoring the saved choice toggles the item, which calls apply().
    if (ctx.settings.get_bool("view", "dark", false)) item_->set_active(true);
}

void DarkMode::apply() {
    const bool dark = item_->get_active();
    ctx_.view.set_dark(dark);
    Gtk::Settings::get_default()->property_gtk_application_prefer_dark_theme() = dark;
    ctx_.settings.set_bool("view", "dark", dark);
}

}  // namespace efpdf
