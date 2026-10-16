// select.h - the "Select" tool: select and copy text, follow links.
//
// Drag across text to select it, then press Ctrl+C to copy. Clicking a link
// follows it (a page jump, or a web / mail address in the desktop's own
// program). A click on a form field is passed to the forms feature first, so
// this default tool can fill forms too.
#pragma once

#include <poppler.h>

#include <string>

#include "core/text_select.h"
#include "features/feature.h"
#include "view/tool.h"

namespace efpdf {

class SelectTool : public Tool {
public:
    explicit SelectTool(AppContext &ctx);

    Glib::ustring label() const override { return "Select"; }
    Glib::ustring hint() const override {
        return "Drag across text to select it, then Ctrl+C to copy. Click a link to follow it.";
    }

    void activate(PageView &view) override;
    bool on_press(PageView &view, const PointerEvent &event) override;
    bool on_motion(PageView &view, const PointerEvent &event) override;
    bool on_release(PageView &view, const PointerEvent &event) override;
    void on_hover(PageView &view, const PointerEvent &event) override;
    void paint(PageView &view, const Cairo::RefPtr<Cairo::Context> &cr) override;
    void deactivate(PageView &view) override;

    std::string selected_text() const;
    void select_all();
    void clear();

private:
    // Where a link on the page leads.
    struct Link {
        enum class Kind { None, Page, Uri } kind = Kind::None;
        int page = 0;          // Kind::Page: 0-based target page
        double top = -1;       // Kind::Page: PDF y to scroll to, or -1 for the page top
        std::string uri;       // Kind::Uri
    };

    Link link_at(double pdf_x, double pdf_y);
    void follow(const Link &link);
    void select_between(double x0, double y0, double x1, double y1);

    AppContext &ctx_;
    bool active_ = false;
    bool dragging_ = false;
    bool moved_ = false;
    double press_x_ = 0, press_y_ = 0;   // widget pixels
    Link pressed_link_;
    int selection_page_ = -1;
    text_select::Selection selection_;
};

class Select : public Feature {
public:
    explicit Select(AppContext &ctx);

private:
    SelectTool &tool_;
};

}  // namespace efpdf
