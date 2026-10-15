#include "features/tool_selector.h"

#include "view/page_view.h"
#include "view/tool_registry.h"

namespace efpdf {

ToolSelector::ToolSelector(AppContext &ctx) : ctx_(ctx) {
    for (const auto &tool : ctx.tools.all()) combo_.append(tool->label());
    combo_.signal_changed().connect(sigc::mem_fun(*this, &ToolSelector::on_changed));
    ctx.toolbar.pack_end(combo_, Gtk::PACK_SHRINK);
    if (!ctx.tools.all().empty()) combo_.set_active(0);
}

void ToolSelector::on_changed() {
    const int index = combo_.get_active_row_number();
    const auto &tools = ctx_.tools.all();
    if (index >= 0 && static_cast<size_t>(index) < tools.size()) {
        ctx_.view.set_tool(tools[index].get());
    }
}

}  // namespace efpdf
