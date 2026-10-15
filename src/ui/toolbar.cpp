#include "ui/toolbar.h"

namespace efpdf::ui {

Gtk::Button *add_toolbar_button(Gtk::Box &toolbar, const char *label,
                                const char *icon_name, const char *tooltip,
                                const sigc::slot<void> &on_click) {
    auto *button = Gtk::manage(new Gtk::Button(label, true));
    if (icon_name && *icon_name) {
        auto *image = Gtk::manage(new Gtk::Image());
        image->set_from_icon_name(icon_name, Gtk::ICON_SIZE_SMALL_TOOLBAR);
        button->set_image(*image);
        button->set_always_show_image(true);
    }
    if (tooltip && *tooltip) button->set_tooltip_text(tooltip);
    button->signal_clicked().connect(on_click);
    toolbar.pack_start(*button, Gtk::PACK_SHRINK);
    return button;
}

void add_toolbar_separator(Gtk::Box &toolbar) {
    auto *separator = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_VERTICAL));
    toolbar.pack_start(*separator, Gtk::PACK_SHRINK);
}

}  // namespace efpdf::ui
