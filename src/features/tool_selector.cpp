#include "features/tool_selector.h"

#include "view/page_view.h"
#include "view/tool_registry.h"

namespace efpdf {

ToolSelector::ToolSelector(AppContext &ctx) : ctx_(ctx) {
    const auto &tools = ctx.tools.all();

    Gtk::RadioButton::Group group;
    for (size_t i = 0; i < tools.size(); ++i) {
        Gtk::RadioButton *radio = nullptr;
        if (i == 0) {
            radio = Gtk::manage(new Gtk::RadioButton(tools[i]->label()));
            group = radio->get_group();
        } else {
            radio = Gtk::manage(new Gtk::RadioButton(group, tools[i]->label()));
        }
        radio->set_mode(false);  // draw as a push button, not a round radio
        radio->set_tooltip_text(tools[i]->hint());
        radio->signal_toggled().connect([this, i, radio] {
            if (radio->get_active()) select(i);
        });
        buttons_.pack_start(*radio, Gtk::PACK_SHRINK);
        radios_.push_back(radio);
    }
    buttons_.get_style_context()->add_class("linked");

    ctx.toolbar.pack_end(buttons_, Gtk::PACK_SHRINK);
    ctx.toolbar.pack_end(caption_, Gtk::PACK_SHRINK);

    // A status line under the toolbar: how to use the active tool, plus short
    // messages from tools ("Highlight added...").
    hint_.set_halign(Gtk::ALIGN_START);
    hint_.set_margin_start(8);
    hint_.set_margin_bottom(4);
    ctx.top_area.pack_start(hint_, Gtk::PACK_SHRINK);
    ctx.view.signal_message.connect([this](const Glib::ustring &message) {
        hint_.set_text(message);
    });

    if (!tools.empty()) select(0);
}

void ToolSelector::select(size_t index) {
    const auto &tools = ctx_.tools.all();
    if (index >= tools.size()) return;
    g_message("efpdf: tool selected: %s", tools[index]->label().c_str());
    ctx_.view.set_tool(tools[index].get());
    hint_.set_text(tools[index]->hint());
}

}  // namespace efpdf
