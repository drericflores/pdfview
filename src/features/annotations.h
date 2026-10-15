// annotations.h - highlight, underline and note tools, plus right-click delete.
#pragma once

#include "features/feature.h"
#include "view/tool.h"

namespace efpdf {

// Drag a rectangle to highlight or underline text.
class MarkupTool : public Tool {
public:
    MarkupTool(AppContext &ctx, bool underline);

    Glib::ustring label() const override { return underline_ ? "Underline" : "Highlight"; }
    Glib::ustring hint() const override {
        return underline_ ? "Press and drag over the text to underline it, then release."
                          : "Press and drag over the text to highlight it, then release.";
    }
    bool on_press(PageView &view, const PointerEvent &event) override;
    bool on_motion(PageView &view, const PointerEvent &event) override;
    bool on_release(PageView &view, const PointerEvent &event) override;
    void paint(PageView &view, const Cairo::RefPtr<Cairo::Context> &cr) override;
    void deactivate(PageView &view) override;

private:
    void add_annotation(PageView &view);

    AppContext &ctx_;
    bool underline_;
    bool dragging_ = false;
    double x0_ = 0, y0_ = 0, x1_ = 0, y1_ = 0;  // widget pixels
};

// Click to drop a sticky note.
class NoteTool : public Tool {
public:
    explicit NoteTool(AppContext &ctx) : ctx_(ctx) {}

    Glib::ustring label() const override { return "Note"; }
    Glib::ustring hint() const override {
        return "Click where you want a sticky note, then type its text.";
    }
    bool on_press(PageView &view, const PointerEvent &event) override;

private:
    AppContext &ctx_;
};

class Annotations : public Feature {
public:
    explicit Annotations(AppContext &ctx);

private:
    void delete_at(const PointerEvent &event);

    AppContext &ctx_;
};

}  // namespace efpdf
