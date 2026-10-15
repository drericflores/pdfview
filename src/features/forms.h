// forms.h - the "Select / Fill forms" tool: click a form field to edit it.
#pragma once

#include "features/feature.h"
#include "view/tool.h"

namespace efpdf {

class FormTool : public Tool {
public:
    explicit FormTool(AppContext &ctx) : ctx_(ctx) {}

    Glib::ustring label() const override { return "Select / Fill forms"; }
    bool on_press(PageView &view, const PointerEvent &event) override;

private:
    AppContext &ctx_;
};

class Forms : public Feature {
public:
    explicit Forms(AppContext &ctx);
};

}  // namespace efpdf
