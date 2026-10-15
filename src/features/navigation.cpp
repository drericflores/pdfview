#include "features/navigation.h"

#include <string>

#include "core/document_model.h"
#include "ui/menu_builder.h"
#include "ui/toolbar.h"
#include "view/page_view.h"

namespace efpdf {

Navigation::Navigation(AppContext &ctx) : ctx_(ctx) {
    auto &model = ctx.model;

    ui::add_toolbar_button(ctx.toolbar, "_Previous", "go-previous-symbolic",
                           "Previous page (Page Up)",
                           [&model] { model.set_current_page(model.current_page() - 1); });
    ui::add_toolbar_button(ctx.toolbar, "_Next", "go-next-symbolic", "Next page (Page Down)",
                           [&model] { model.set_current_page(model.current_page() + 1); });

    entry_.set_width_chars(4);
    entry_.set_max_width_chars(6);
    entry_.set_alignment(1.0f);
    entry_.set_tooltip_text("Type a page number and press Enter");
    entry_.signal_activate().connect(sigc::mem_fun(*this, &Navigation::jump_from_entry));
    ctx.toolbar.pack_start(entry_, Gtk::PACK_SHRINK);
    ctx.toolbar.pack_start(total_label_, Gtk::PACK_SHRINK);
    ui::add_toolbar_separator(ctx.toolbar);

    const Gdk::ModifierType ctrl = Gdk::CONTROL_MASK;
    auto &menus = ctx.menus;
    menus.add_item("_View", "_Previous Page",
                   [&model] { model.set_current_page(model.current_page() - 1); },
                   GDK_KEY_Page_Up);
    menus.add_item("_View", "_Next Page",
                   [&model] { model.set_current_page(model.current_page() + 1); },
                   GDK_KEY_Page_Down);
    menus.add_item("_View", "_First Page", [&model] { model.set_current_page(0); },
                   GDK_KEY_Home, ctrl);
    menus.add_item("_View", "_Last Page",
                   [&model] { model.set_current_page(model.page_count() - 1); },
                   GDK_KEY_End, ctrl);
    menus.add_item("_View", "_Go to Page...", sigc::mem_fun(*this, &Navigation::focus_entry),
                   GDK_KEY_g, ctrl);
    menus.add_separator("_View");

    model.signal_loaded.connect(sigc::mem_fun(*this, &Navigation::update_display));
    model.signal_closed.connect(sigc::mem_fun(*this, &Navigation::update_display));
    model.signal_page_changed.connect(sigc::mem_fun(*this, &Navigation::update_display));
    update_display();
}

void Navigation::update_display() {
    if (!ctx_.model.is_open()) {
        entry_.set_text("");
        total_label_.set_text("");
        return;
    }
    entry_.set_text(std::to_string(ctx_.model.current_page() + 1));
    total_label_.set_text(" / " + std::to_string(ctx_.model.page_count()));
}

void Navigation::jump_from_entry() {
    if (!ctx_.model.is_open()) return;
    try {
        ctx_.model.set_current_page(std::stoi(entry_.get_text()) - 1);
    } catch (const std::exception &) {
        // Not a number: fall through and restore the real page number.
    }
    update_display();
    ctx_.view.area().grab_focus();
}

void Navigation::focus_entry() {
    if (!ctx_.model.is_open()) return;
    entry_.grab_focus();
    entry_.select_region(0, -1);
}

}  // namespace efpdf
