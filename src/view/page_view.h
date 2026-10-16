// page_view.h - draws the current page and turns mouse input into tool events.
//
// Responsibilities: rendering (with zoom and dark mode), coordinate conversion,
// and forwarding pointer events. It deliberately knows nothing about
// annotations, forms, search or any other feature; those plug in through the
// Tool interface and the overlay/secondary-click signals.
#pragma once

#include <gtkmm.h>
#include <sigc++/sigc++.h>

#include <string>

#include "core/document_model.h"
#include "view/tool.h"

namespace efpdf {

class PageView : public Gtk::ScrolledWindow {
public:
    static constexpr double kMinScale = 0.1;
    static constexpr double kMaxScale = 8.0;
    static constexpr double kZoomStep = 1.2;

    explicit PageView(DocumentModel &model);

    double scale() const { return scale_; }
    void set_scale(double scale);
    void fit_width();
    void fit_page();

    void set_dark(bool dark);
    bool dark() const { return dark_; }

    // Re-renders the current page (after edits, zoom, theme change...).
    void refresh();

    void set_tool(Tool *tool);

    // Conversions between widget pixels and PDF points (origin bottom-left).
    void to_pdf(double sx, double sy, double &px, double &py) const;
    void from_pdf(double px, double py, double &sx, double &sy) const;

    // Scrolls so the given PDF point is comfortably in view.
    void scroll_to_pdf_point(double px, double py);

    Gtk::DrawingArea &area() { return area_; }

    // Sets the mouse cursor over the page by name ("pointer", "text", ...);
    // nullptr restores the default arrow.
    void set_cursor(const char *name);

    // Widgets added with overlay().add_overlay() float above the page. Use
    // place_over_pdf_rect() to line one up with a rectangle in PDF points
    // (for example, a form field being edited in place).
    Gtk::Overlay &overlay() { return overlay_; }
    void place_over_pdf_rect(Gtk::Widget &widget, double x1, double y1, double x2,
                             double y2);

    sigc::signal<void> signal_scale_changed;
    // Short status messages from tools ("Highlight added"), shown to the user.
    sigc::signal<void, const Glib::ustring &> signal_message;
    // Right mouse button; used by the annotation eraser.
    sigc::signal<void, const PointerEvent &> signal_secondary_click;
    // Emitted by the Select tool before it starts selecting text, so a feature
    // that owns something clickable (form fields) can take the click first by
    // setting event.handled.
    sigc::signal<void, PointerEvent &> signal_probe_click;

    // Copy: a feature that has a text selection appends it to the string.
    sigc::signal<void, std::string &> signal_collect_selection;

    // Edit > Select All, when no text box has the keyboard focus.
    sigc::signal<void> signal_select_all;

    // Lets features draw over the page (search highlights, for example).
    sigc::signal<void, const Cairo::RefPtr<Cairo::Context> &> signal_paint_overlay;

private:
    bool on_area_draw(const Cairo::RefPtr<Cairo::Context> &cr);
    bool on_area_press(GdkEventButton *event);
    bool on_area_motion(GdkEventMotion *event);
    bool on_area_release(GdkEventButton *event);
    bool on_area_scroll(GdkEventScroll *event);
    bool on_area_leave(GdkEventCrossing *event);
    PointerEvent make_event(double x, double y, guint button) const;
    bool measure();

    DocumentModel &model_;
    Gtk::Overlay overlay_;
    Gtk::DrawingArea area_;
    Cairo::RefPtr<Cairo::ImageSurface> surface_;
    Tool *tool_ = nullptr;
    double scale_ = 1.0;
    double page_w_ = 0, page_h_ = 0;  // current page size in points
    bool dark_ = false;
    int surface_w_ = 0, surface_h_ = 0;  // page size in logical pixels
    std::string cursor_name_;
};

}  // namespace efpdf
