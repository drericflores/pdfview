#include "features/appearance.h"

#include <cstdlib>
#include <string>

#include "view/page_view.h"

namespace efpdf {
namespace {

// --- palettes ---------------------------------------------------------------
// Named colors (@efpdf_*) are the only place colors are chosen; the rules
// below just use the names. Change a value here to re-color the whole program.

const char *kLightPalette = R"CSS(
@define-color efpdf_accent        #2563eb;
@define-color efpdf_accent_dark   #1d4ed8;
@define-color efpdf_accent_soft   #dfe9ff;
@define-color efpdf_menubar_top    #ffffff;
@define-color efpdf_menubar_bottom #eef2f8;
@define-color efpdf_menu_text      #000000;
@define-color efpdf_menu_hover     rgba(0, 0, 0, 0.08);
@define-color efpdf_bar           #ffffff;
@define-color efpdf_bar_alt       #f3f6fb;
@define-color efpdf_line          #dbe1ea;
@define-color efpdf_hover         #e9effb;
@define-color efpdf_text          #1f2937;
@define-color efpdf_muted         #64748b;
@define-color efpdf_canvas        #d9dfe9;
@define-color efpdf_sidebar       #eef2f8;
@define-color efpdf_hint_bg       #eaf1ff;
)CSS";

const char *kDarkPalette = R"CSS(
@define-color efpdf_accent        #6b96ff;
@define-color efpdf_accent_dark   #4a78eb;
@define-color efpdf_accent_soft   #243459;
@define-color efpdf_menubar_top    #0b111f;
@define-color efpdf_menubar_bottom #16203a;
@define-color efpdf_menu_text      #ffffff;
@define-color efpdf_menu_hover     rgba(255, 255, 255, 0.18);
@define-color efpdf_bar           #1a202b;
@define-color efpdf_bar_alt       #212937;
@define-color efpdf_line          #2c3544;
@define-color efpdf_hover         #27324a;
@define-color efpdf_text          #e5e9f0;
@define-color efpdf_muted         #9aa7ba;
@define-color efpdf_canvas        #0d1016;
@define-color efpdf_sidebar       #151a23;
@define-color efpdf_hint_bg       #1b2538;
)CSS";

// --- dark base ----------------------------------------------------------------
// Darkens everything, including dialogs and menus, even on themes that ignore
// GTK's "prefer dark theme" setting. Only used when dark mode is on; the
// specific rules further down (which have class selectors) win over these.

const char *kDarkBase = R"CSS(
window, dialog, menubar, menu, menuitem, toolbar, paned, viewport, scrolledwindow,
list, row, label, button, entry, checkbutton, radiobutton, combobox, box {
    background-color: @efpdf_bar;
    background-image: none;
    color: @efpdf_text;
}
button, entry, combobox {
    border: 1px solid @efpdf_line;
}
entry {
    background-color: @efpdf_bar_alt;
}
menuitem:hover, button:hover {
    background-color: @efpdf_hover;
}
row:selected {
    background-color: @efpdf_accent_soft;
}
)CSS";

// --- the look -----------------------------------------------------------------

const char *kStyle = R"CSS(
/* Menu bar: black text on a light bar (light mode), white text on a dark bar
   (dark mode). A thin accent line underneath keeps the brand color. */
menubar {
    background-color: @efpdf_menubar_bottom;
    background-image: linear-gradient(to bottom, @efpdf_menubar_top, @efpdf_menubar_bottom);
    border-bottom: 2px solid @efpdf_accent;
    padding: 0 6px;
}
menubar > menuitem {
    background-color: transparent;
    background-image: none;
    color: @efpdf_menu_text;
    padding: 4px 11px;
    margin: 3px 1px;
    border-radius: 6px;
}
menubar > menuitem label {
    background-color: transparent;
    color: @efpdf_menu_text;
}
menubar > menuitem:hover,
menubar > menuitem:selected {
    background-color: @efpdf_menu_hover;
}

/* Drop-down menus: same text color rule, on the toolbar's background. */
menu {
    background-color: @efpdf_bar;
    background-image: none;
    border: 1px solid @efpdf_line;
    border-radius: 8px;
    padding: 4px 0;
}
menu menuitem {
    background-color: transparent;
    background-image: none;
    color: @efpdf_menu_text;
    padding: 6px 14px;
}
menu menuitem label {
    background-color: transparent;
    color: @efpdf_menu_text;
}
menu menuitem:hover {
    background-color: @efpdf_accent_soft;
}
menu menuitem:disabled label {
    opacity: 0.45;
}
menu separator {
    background-color: @efpdf_line;
    min-height: 1px;
    margin: 4px 0;
}

/* Toolbar */
.efpdf-toolbar {
    background-color: @efpdf_bar;
    background-image: none;
    border-bottom: 1px solid @efpdf_line;
    padding: 7px 10px;
}
.efpdf-toolbar > label {
    background-color: transparent;
    color: @efpdf_muted;
}
.efpdf-toolbar separator {
    background-color: @efpdf_line;
    min-width: 1px;
    margin: 4px 8px;
}

/* Buttons in the toolbar and in the strip under it (search bar) */
.efpdf-toolbar button,
.efpdf-topstrip button {
    background-color: transparent;
    background-image: none;
    border: 1px solid transparent;
    border-radius: 7px;
    box-shadow: none;
    text-shadow: none;
    color: @efpdf_text;
    padding: 5px 11px;
    transition: background-color 120ms ease, border-color 120ms ease;
}
.efpdf-toolbar button:hover,
.efpdf-topstrip button:hover {
    background-color: @efpdf_hover;
    border-color: @efpdf_line;
}
.efpdf-toolbar button:active,
.efpdf-toolbar button:checked,
.efpdf-topstrip button:active,
.efpdf-topstrip button:checked {
    background-color: @efpdf_accent_soft;
    border-color: @efpdf_accent;
    color: @efpdf_accent;
}
.efpdf-toolbar button:disabled,
.efpdf-topstrip button:disabled {
    opacity: 0.45;
}

/* Text inside any of these buttons follows the button's own color. */
.efpdf-toolbar button label,
.efpdf-topstrip button label,
.efpdf-tool label {
    background-color: transparent;
    color: inherit;
}

/* The main action (Save) in the accent color */
.efpdf-toolbar button.efpdf-primary {
    background-color: @efpdf_accent;
    border-color: @efpdf_accent;
    color: #ffffff;
}
.efpdf-toolbar button.efpdf-primary:hover {
    background-color: @efpdf_accent_dark;
    border-color: @efpdf_accent_dark;
}

/* Tool picker (Fill forms / Highlight / Underline / Note): a segmented control */
.efpdf-tool {
    background-color: @efpdf_bar_alt;
    background-image: none;
    border: 1px solid @efpdf_line;
    border-radius: 0;
    box-shadow: none;
    text-shadow: none;
    color: @efpdf_text;
    padding: 5px 13px;
}
.efpdf-tool:first-child {
    border-radius: 7px 0 0 7px;
}
.efpdf-tool:last-child {
    border-radius: 0 7px 7px 0;
}
.efpdf-tool:hover {
    background-color: @efpdf_hover;
}
.efpdf-tool:checked {
    background-color: @efpdf_accent;
    border-color: @efpdf_accent;
    color: #ffffff;
}

/* Entries (page number box, search box) */
.efpdf-toolbar entry,
.efpdf-topstrip entry {
    background-color: @efpdf_bar_alt;
    background-image: none;
    color: @efpdf_text;
    border: 1px solid @efpdf_line;
    border-radius: 7px;
    box-shadow: none;
    padding: 4px 8px;
    min-height: 22px;
}
.efpdf-toolbar entry:focus,
.efpdf-topstrip entry:focus {
    border-color: @efpdf_accent;
}

/* Strip under the toolbar: search bar and the one-line instructions */
.efpdf-topstrip > box {
    background-color: @efpdf_bar_alt;
    border-bottom: 1px solid @efpdf_line;
    padding: 6px 10px;
}
.efpdf-topstrip > box > label {
    background-color: transparent;
    color: @efpdf_muted;
}
.efpdf-hint {
    background-color: @efpdf_hint_bg;
    border-bottom: 1px solid @efpdf_line;
    color: @efpdf_muted;
    padding: 6px 14px;
    font-size: 92%;
}

/* Thumbnail sidebar */
.efpdf-sidebar {
    background-color: @efpdf_sidebar;
    background-image: none;
    border-right: 1px solid @efpdf_line;
}
.efpdf-sidebar viewport,
.efpdf-sidebar list {
    background-color: transparent;
    background-image: none;
}
.efpdf-sidebar row {
    background-color: transparent;
    background-image: none;
    border: 2px solid transparent;
    border-radius: 9px;
    margin: 3px 10px;
    padding: 6px 4px;
    transition: background-color 120ms ease, border-color 120ms ease;
}
.efpdf-sidebar row:hover {
    background-color: @efpdf_hover;
}
.efpdf-sidebar row:selected {
    background-color: @efpdf_accent_soft;
    border-color: @efpdf_accent;
}
.efpdf-sidebar row:drop(active) {
    background-color: @efpdf_accent_soft;
    border-color: @efpdf_accent;
}
.efpdf-sidebar image {
    box-shadow: 0 1px 4px rgba(0, 0, 0, 0.30);
}
.efpdf-sidebar label {
    background-color: transparent;
    color: @efpdf_muted;
    font-size: 90%;
    font-weight: bold;
}
.efpdf-sidebar row:selected label {
    color: @efpdf_accent;
}

/* Page area: a calm backdrop so the white page stands out */
.efpdf-canvas,
.efpdf-canvas viewport {
    background-color: @efpdf_canvas;
    background-image: none;
}
)CSS";

std::string build_css(bool dark) {
    std::string css = dark ? kDarkPalette : kLightPalette;
    if (dark) css += kDarkBase;
    css += kStyle;
    return css;
}

void tag(Gtk::Widget &widget, const char *style_class) {
    widget.get_style_context()->add_class(style_class);
}

}  // namespace

Appearance::Appearance(AppContext &ctx) : ctx_(ctx) {
    // EFPDF_PLAIN=1 leaves the system theme completely untouched.
    const char *plain = std::getenv("EFPDF_PLAIN");
    if (plain && *plain && std::string(plain) != "0") return;

    tag(ctx.toolbar, "efpdf-toolbar");
    tag(ctx.top_area, "efpdf-topstrip");
    tag(ctx.sidebar, "efpdf-sidebar");
    tag(ctx.view, "efpdf-canvas");

    css_ = Gtk::CssProvider::create();
    Gtk::StyleContext::add_provider_for_screen(
        Gdk::Screen::get_default(), css_, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    installed_ = true;

    // View > Dark Mode flips this setting; follow it.
    dark_changed_ = Gtk::Settings::get_default()
                        ->property_gtk_application_prefer_dark_theme()
                        .signal_changed()
                        .connect(sigc::mem_fun(*this, &Appearance::apply));
    apply();
}

Appearance::~Appearance() {
    dark_changed_.disconnect();
    if (installed_) {
        Gtk::StyleContext::remove_provider_for_screen(Gdk::Screen::get_default(), css_);
    }
}

void Appearance::apply() {
    const bool dark =
        Gtk::Settings::get_default()->property_gtk_application_prefer_dark_theme().get_value();
    try {
        css_->load_from_data(build_css(dark));
    } catch (const Glib::Error &error) {
        // A bad stylesheet must never stop the program; it just looks plain.
        g_warning("efpdf: could not load the theme: %s", error.what().c_str());
    }
    ctx_.view.area().queue_draw();  // the page border follows the palette
}

}  // namespace efpdf
