#include "features/forms.h"

#include <algorithm>
#include <string>

#include "core/document_model.h"
#include "ui/dialogs.h"
#include "view/page_view.h"
#include "view/tool_registry.h"

namespace efpdf {
namespace {

bool inside(const PopplerRectangle &r, double x, double y) {
    return x >= std::min(r.x1, r.x2) && x <= std::max(r.x1, r.x2) &&
           y >= std::min(r.y1, r.y2) && y <= std::max(r.y1, r.y2);
}

// Returns true if the field's value was changed.
bool edit_field(AppContext &ctx, PopplerFormField *field) {
    switch (poppler_form_field_get_field_type(field)) {
        case POPPLER_FORM_FIELD_TEXT: {
            gchar *current = poppler_form_field_text_get_text(field);
            std::string value = current ? current : "";
            g_free(current);
            const bool multiline = poppler_form_field_text_get_text_type(field) ==
                                   POPPLER_FORM_TEXT_MULTILINE;
            if (!ui::prompt_text(ctx.window, "Edit field", "Value:", value, multiline)) {
                return false;
            }
            poppler_form_field_text_set_text(field, value.c_str());
            return true;
        }
        case POPPLER_FORM_FIELD_BUTTON: {
            const PopplerFormButtonType type =
                poppler_form_field_button_get_button_type(field);
            const gboolean state = poppler_form_field_button_get_state(field);
            if (type == POPPLER_FORM_BUTTON_CHECK) {
                poppler_form_field_button_set_state(field, !state);
                return true;
            }
            if (type == POPPLER_FORM_BUTTON_RADIO && !state) {
                poppler_form_field_button_set_state(field, TRUE);
                return true;
            }
            return false;
        }
        case POPPLER_FORM_FIELD_CHOICE: {
            Gtk::Dialog dlg("Choose a value", ctx.window, true);
            dlg.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
            dlg.add_button("_OK", Gtk::RESPONSE_OK);
            Gtk::ComboBoxText combo;
            const int n = poppler_form_field_choice_get_n_items(field);
            for (int i = 0; i < n; ++i) {
                gchar *item = poppler_form_field_choice_get_item(field, i);
                combo.append(item ? item : "");
                g_free(item);
                if (poppler_form_field_choice_is_item_selected(field, i)) {
                    combo.set_active(i);
                }
            }
            dlg.get_content_area()->pack_start(combo, Gtk::PACK_SHRINK, 8);
            dlg.show_all_children();
            if (dlg.run() != Gtk::RESPONSE_OK || combo.get_active_row_number() < 0) {
                return false;
            }
            poppler_form_field_choice_unselect_all(field);
            poppler_form_field_choice_select_item(field, combo.get_active_row_number());
            return true;
        }
        default:
            return false;
    }
}

}  // namespace

bool FormTool::on_press(PageView &, const PointerEvent &event) {
    PagePtr page = ctx_.model.current_page_object();
    if (!page) return false;

    GList *mappings = poppler_page_get_form_field_mapping(page.get());
    bool changed = false;
    bool hit = false;
    for (GList *l = mappings; l; l = l->next) {
        auto *m = static_cast<PopplerFormFieldMapping *>(l->data);
        if (!inside(m->area, event.pdf_x, event.pdf_y)) continue;
        hit = true;
        if (!poppler_form_field_is_read_only(m->field)) {
            changed = edit_field(ctx_, m->field);
        }
        break;
    }
    poppler_page_free_form_field_mapping(mappings);
    if (changed) ctx_.model.notify_modified();
    return hit;
}

Forms::Forms(AppContext &ctx) { ctx.tools.emplace<FormTool>(ctx); }

}  // namespace efpdf
