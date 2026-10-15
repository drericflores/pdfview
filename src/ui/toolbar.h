// toolbar.h - helpers so every feature builds toolbar buttons the same way
// (icon + label + tooltip) and the toolbar can be styled consistently.
#pragma once

#include <gtkmm.h>

namespace efpdf::ui {

// Adds a button to the toolbar. `icon_name` is a themed icon such as
// "document-open-symbolic" (empty for text only); `label` may contain an
// underscore mnemonic. Returns the button so the caller can style it further.
Gtk::Button *add_toolbar_button(Gtk::Box &toolbar, const char *label,
                                const char *icon_name, const char *tooltip,
                                const sigc::slot<void> &on_click);

// A thin vertical line separating groups of toolbar buttons.
void add_toolbar_separator(Gtk::Box &toolbar);

}  // namespace efpdf::ui
