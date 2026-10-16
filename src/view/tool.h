// tool.h - the interface every mouse tool implements.
//
// The page view knows nothing about annotations or forms; it just forwards
// pointer events to whichever Tool is active. A feature adds a tool by
// registering one with the ToolRegistry.
#pragma once

#include <gtkmm.h>

namespace efpdf {

class PageView;

struct PointerEvent {
    double x = 0, y = 0;          // widget pixels (origin top-left of the page)
    double pdf_x = 0, pdf_y = 0;  // PDF points (origin bottom-left of the page)
    guint button = 0;
    // Only used by PageView::signal_probe_click: a feature sets this when it
    // has taken the click (for example, a click that landed on a form field).
    bool handled = false;
};

class Tool {
public:
    virtual ~Tool() = default;

    // Text shown in the tool selector.
    virtual Glib::ustring label() const = 0;

    // One-line instruction shown to the user while the tool is active.
    virtual Glib::ustring hint() const { return ""; }

    // Called when this tool becomes the active one.
    virtual void activate(PageView &) {}

    virtual bool on_press(PageView &, const PointerEvent &) { return false; }
    virtual bool on_motion(PageView &, const PointerEvent &) { return false; }
    virtual bool on_release(PageView &, const PointerEvent &) { return false; }

    // The pointer moved with no button held (used to change the cursor).
    virtual void on_hover(PageView &, const PointerEvent &) {}

    // Draw transient feedback (such as a rubber band) over the page.
    virtual void paint(PageView &, const Cairo::RefPtr<Cairo::Context> &) {}

    // Called when another tool is selected; drop any half-finished gesture.
    virtual void deactivate(PageView &) {}
};

}  // namespace efpdf
