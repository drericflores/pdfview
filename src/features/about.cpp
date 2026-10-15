#include "features/about.h"

#include "core/version.h"
#include "ui/app_icon.h"
#include "ui/menu_builder.h"

namespace efpdf {

About::About(AppContext &ctx) : ctx_(ctx) {
    ctx.menus.add_item("_Help", "_About", sigc::mem_fun(*this, &About::show));
}

void About::show() {
    Gtk::AboutDialog dlg;
    dlg.set_transient_for(ctx_.window);
    dlg.set_program_name("efpdf (Easy & Flexible PDF)");
    dlg.set_comments(
        "A lightweight PDF viewer and editor built with GTKmm 3, "
        "Poppler-GLib and QPDF.");
    dlg.set_version(efpdf::kRelease);
    dlg.set_authors({"Dr. Eric Oliver Flores Toro"});
    dlg.set_copyright(
        "By: Dr. Eric Oliver Flores Toro\n"
        "Anatolica Consulting, LLC\n"
        "October 2026");
    dlg.set_license_type(Gtk::LICENSE_MIT_X11);
    dlg.set_website("https://github.com/drericflores/efpdf");
    dlg.run();
}

}  // namespace efpdf
