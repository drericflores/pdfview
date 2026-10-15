#include "features/zoom.h"

#include "core/document_model.h"
#include "core/settings.h"
#include "ui/menu_builder.h"
#include "ui/toolbar.h"
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

    // Preferences > "Zoom when opening a file".
    ctx.model.signal_loaded.connect(sigc::mem_fun(*this, &Zoom::apply_open_zoom));

    ui::add_toolbar_button(ctx.toolbar, "Zoom Out", "zoom-out-symbolic", "Zoom out (Ctrl+-)",
                           zoom_out);
    ui::add_toolbar_button(ctx.toolbar, "Zoom In", "zoom-in-symbolic", "Zoom in (Ctrl++)",
                           zoom_in);
    ui::add_toolbar_button(ctx.toolbar, "Fit Width", "zoom-fit-best-symbolic",
                           "Fit the page to the window width (Ctrl+1)",
                           [&view] { view.fit_width(); });
}

// Applies the chosen opening zoom the first time a given file is shown (not
// after each save, which also reloads the document).
void Zoom::apply_open_zoom() {
    if (!ctx_.model.is_open() || ctx_.model.path() == last_path_) return;
    last_path_ = ctx_.model.path();

    const std::string mode = ctx_.settings.get_string("view", "open_zoom", "remember");
    if (mode == "remember") return;
    // Wait briefly so the window has its real size when fitting.
    Glib::signal_timeout().connect_once(
        [this, mode] {
            if (!ctx_.model.is_open()) return;
            if (mode == "fit_width") ctx_.view.fit_width();
            else if (mode == "fit_page") ctx_.view.fit_page();
            else if (mode == "actual") ctx_.view.set_scale(1.0);
        },
        120);
}

}  // namespace efpdf
