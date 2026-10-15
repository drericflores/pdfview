// forms.h - the "Fill forms" tool: edit form fields directly on the page.
//
// Click a text field and an editor appears right on top of it; type, then
// press Enter (or click elsewhere) to keep the value, or Escape to cancel.
// Check boxes and radio buttons toggle on click; drop-downs open in place.
#pragma once

#include <poppler.h>

#include "features/feature.h"
#include "view/tool.h"

namespace efpdf {

class FormTool : public Tool {
public:
    explicit FormTool(AppContext &ctx);

    Glib::ustring label() const override { return "Fill forms"; }
    Glib::ustring hint() const override {
        return "Click a form field to fill it in. Enter keeps the value, Esc cancels.";
    }
    bool on_press(PageView &view, const PointerEvent &event) override;
    void deactivate(PageView &view) override;

private:
    void begin_text_edit(PopplerFormField *field, const PopplerRectangle &area);
    void begin_choice_edit(PopplerFormField *field, const PopplerRectangle &area);
    // Applies (or discards) the open editor and removes it.
    void finish(bool keep);
    void release_editor();

    AppContext &ctx_;
    GList *mappings_ = nullptr;        // keeps the field alive while editing
    PopplerFormField *field_ = nullptr;
    Gtk::Widget *editor_ = nullptr;    // the widget floating over the page
    Gtk::Entry *entry_ = nullptr;
    Gtk::TextView *text_view_ = nullptr;
    Gtk::ComboBoxText *combo_ = nullptr;
    int chosen_item_ = -1;
};

class Forms : public Feature {
public:
    explicit Forms(AppContext &ctx);
};

}  // namespace efpdf
