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
};

class Tool {
public:
    virtual ~Tool() = default;

    // Text shown in the tool selector.
    virtual Glib::ustring label() const = 0;

    virtual bool on_press(PageView &, const PointerEvent &) { return false; }
    virtual bool on_motion(PageView &, const PointerEvent &) { return false; }
    virtual bool on_release(PageView &, const PointerEvent &) { return false; }

    // Draw transient feedback (such as a rubber band) over the page.
    virtual void paint(PageView &, const Cairo::RefPtr<Cairo::Context> &) {}

    // Called when another tool is selected; drop any half-finished gesture.
    virtual void deactivate(PageView &) {}
};

}  // namespace efpdf
