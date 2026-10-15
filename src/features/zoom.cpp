#include "features/zoom.h"

#include "core/settings.h"
#include "ui/menu_builder.h"
#include "view/page_view.h"

namespace efpdf {

Zoom::Zoom(AppContext &ctx) : ctx_(ctx) {
    PageView &view = ctx.view;
    const Gdk::ModifierType ctrl = Gdk::CONTROL_MASK;

    view.set_scale(ctx.settings.get_double("view", "scale", 1.0));
    view.signal_scale_changed.connect(
        [this] { ctx_.settings.set_double("view", "scale", ctx_.view.scale()); });

    auto zoom_in = [&view] { view.set_scale(view.scale() * PageView::kZoomStep); };
    auto zoom_out = [&view] { view.set_scale(view.scale() / PageView::kZoomStep); };

    auto &menus = ctx.menus;
    menus.add_item("_View", "Zoom _In", zoom_in, GDK_KEY_plus, ctrl);
    menus.add_item("_View", "Zoom In (Ctrl+=)", zoom_in, GDK_KEY_equal, ctrl);
    menus.add_item("_View", "Zoom _Out", zoom_out, GDK_KEY_minus, ctrl);
    menus.add_item("_View", "_Actual Size", [&view] { view.set_scale(1.0); }, GDK_KEY_0, ctrl);
    menus.add_item("_View", "Fit _Width", [&view] { view.fit_width(); }, GDK_KEY_1, ctrl);
    menus.add_item("_View", "Fit Pa_ge", [&view] { view.fit_page(); }, GDK_KEY_2, ctrl);
    menus.add_separator("_View");

    auto add_button = [&ctx](const char *label, const sigc::slot<void> &slot) {
        auto *button = Gtk::manage(new Gtk::Button(label, true));
        button->signal_clicked().connect(slot);
        ctx.toolbar.pack_start(*button, Gtk::PACK_SHRINK);
    };
    add_button("Zoom _-", zoom_out);
    add_button("Zoom _+", zoom_in);
    add_button("Fit _Width", [&view] { view.fit_width(); });
}

}  // namespace efpdf
