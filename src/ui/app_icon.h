// app_icon.h - the application icon (resources/efpdf2.png).
//
// The icon is found next to the program when run from the source folder
// (./resources/efpdf2.png), in the installed location
// (<prefix>/share/efpdf/efpdf2.png), or by name ("efpdf") in the icon theme
// once `make install` has put it there.
#pragma once

#include <gtkmm.h>

namespace efpdf::ui {

// Makes every window use the application icon. Call once at start-up.
void install_app_icon();

// The icon as a picture (for the About dialog), or null if it was not found.
Glib::RefPtr<Gdk::Pixbuf> app_icon_pixbuf(int size);

}  // namespace efpdf::ui
