#include "features/forms.h"

#include <algorithm>
#include <string>

#include "core/document_model.h"
#include "core/history.h"
#include "ui/text_undo.h"
#include "view/page_view.h"
#include "view/tool_registry.h"

namespace efpdf {
namespace {

bool inside(const PopplerRectangle &r, double x, double y) {
    return x >= std::min(r.x1, r.x2) && x <= std::max(r.x1, r.x2) &&
           y >= std::min(r.y1, r.y2) && y <= std::max(r.y1, r.y2);
}

// One reversible change to a form field. Fields are found again by page and id
// so the step still works after the page object has been released.
struct FieldChange {
    enum Kind { Text, Choice, Check, Radio } kind = Text;
    int page = 0;
    int id = 0;
    std::string old_text, new_text;
    int old_choice = -1, new_choice = -1;
    bool old_state = false, new_state = false;
    int old_radio_id = -1;  // the radio button that was selected before
};

PopplerFormField *find_field(GList *mappings, int id) {
    for (GList *l = mappings; l; l = l->next) {
        auto *m = static_cast<PopplerFormFieldMapping *>(l->data);
        if (poppler_form_field_get_id(m->field) == id) return m->field;
    }
    return nullptr;
}

// Applies the "before" values (undo) or "after" values (redo) of a change.
void apply_change(AppContext &ctx, const FieldChange &c, bool undo) {
    PagePtr page = ctx.model.page(c.page);
    if (!page) return;
    ctx.model.set_current_page(c.page);

    GList *mappings = poppler_page_get_form_field_mapping(page.get());
    PopplerFormField *field = find_field(mappings, c.id);
    if (field) {
        switch (c.kind) {
            case FieldChange::Text:
                poppler_form_field_text_set_text(field,
                                                 (undo ? c.old_text : c.new_text).c_str());
                break;
            case FieldChange::Choice: {
                const int item = undo ? c.old_choice : c.new_choice;
                poppler_form_field_choice_unselect_all(field);
                if (item >= 0) poppler_form_field_choice_select_item(field, item);
                break;
            }
            case FieldChange::Check:
                poppler_form_field_button_set_state(field, undo ? c.old_state : c.new_state);
                break;
            case FieldChange::Radio:
                if (!undo) {
                    poppler_form_field_button_set_state(field, TRUE);
                } else if (PopplerFormField *prev =
                               c.old_radio_id >= 0 ? find_field(mappings, c.old_radio_id)
                                                   : nullptr) {
                    poppler_form_field_button_set_state(prev, TRUE);  // unchecks `field`
                } else {
                    poppler_form_field_button_set_state(field, FALSE);
                }
                break;
        }
    }
    poppler_page_free_form_field_mapping(mappings);
    ctx.model.notify_modified();
}

void record_change(AppContext &ctx, const char *label, const FieldChange &change) {
    AppContext *c = &ctx;
    ctx.history.push({label, [c, change] { apply_change(*c, change, true); },
                      [c, change] { apply_change(*c, change, false); }});
}

}  // namespace

FormTool::FormTool(AppContext &ctx) : ctx_(ctx) {
    // An editor is only valid for the page and zoom it was opened on.
    auto cancel = [this] { finish(false); };
    ctx.model.signal_page_changed.connect(cancel);
    ctx.model.signal_loaded.connect(cancel);
    ctx.model.signal_closed.connect(cancel);
    ctx.view.signal_scale_changed.connect(cancel);
}

void FormTool::deactivate(PageView &) { finish(true); }

bool FormTool::on_press(PageView &view, const PointerEvent &event) {
    // Clicking elsewhere keeps whatever was being typed.
    finish(true);

    PagePtr page = ctx_.model.current_page_object();
    if (!page) return false;

    GList *mappings = poppler_page_get_form_field_mapping(page.get());
    bool handled = false;
    bool editing = false;

    for (GList *l = mappings; l; l = l->next) {
        auto *m = static_cast<PopplerFormFieldMapping *>(l->data);
        if (!inside(m->area, event.pdf_x, event.pdf_y)) continue;
        handled = true;
        PopplerFormField *field = m->field;
        if (poppler_form_field_is_read_only(field)) break;

        switch (poppler_form_field_get_field_type(field)) {
            case POPPLER_FORM_FIELD_TEXT:
                mappings_ = mappings;  // keep the field alive until finish()
                field_ = field;
                begin_text_edit(field, m->area);
                editing = true;
                break;
            case POPPLER_FORM_FIELD_CHOICE:
                mappings_ = mappings;
                field_ = field;
                begin_choice_edit(field, m->area);
                editing = true;
                break;
            case POPPLER_FORM_FIELD_BUTTON: {
                const PopplerFormButtonType type =
                    poppler_form_field_button_get_button_type(field);
                const gboolean state = poppler_form_field_button_get_state(field);
                FieldChange change;
                change.page = ctx_.model.current_page();
                change.id = poppler_form_field_get_id(field);
                if (type == POPPLER_FORM_BUTTON_CHECK) {
                    change.kind = FieldChange::Check;
                    change.old_state = state;
                    change.new_state = !state;
                    poppler_form_field_button_set_state(field, !state);
                    ctx_.model.notify_modified();
                    record_change(ctx_, "Toggle check box", change);
                } else if (type == POPPLER_FORM_BUTTON_RADIO && !state) {
                    change.kind = FieldChange::Radio;
                    // Remember which button of the same group was selected.
                    const gchar *group = poppler_form_field_get_name(field);
                    for (GList *o = mappings; group && o; o = o->next) {
                        auto *other = static_cast<PopplerFormFieldMapping *>(o->data)->field;
                        if (other == field ||
                            poppler_form_field_get_field_type(other) != POPPLER_FORM_FIELD_BUTTON ||
                            poppler_form_field_button_get_button_type(other) !=
                                POPPLER_FORM_BUTTON_RADIO ||
                            !poppler_form_field_button_get_state(other)) {
                            continue;
                        }
                        const gchar *name = poppler_form_field_get_name(other);
                        if (name && g_strcmp0(name, group) == 0) {
                            change.old_radio_id = poppler_form_field_get_id(other);
                            break;
                        }
                    }
                    poppler_form_field_button_set_state(field, TRUE);
                    ctx_.model.notify_modified();
                    record_change(ctx_, "Select option", change);
                }
                break;
            }
            default:
                break;
        }
        break;
    }

    if (!editing) poppler_page_free_form_field_mapping(mappings);
    (void)view;
    return handled;
}

void FormTool::begin_text_edit(PopplerFormField *field, const PopplerRectangle &area) {
    gchar *current = poppler_form_field_text_get_text(field);
    const std::string value = current ? current : "";
    g_free(current);
    orig_text_ = value;
    page_index_ = ctx_.model.current_page();

    const bool multiline = poppler_form_field_text_get_text_type(field) ==
                           POPPLER_FORM_TEXT_MULTILINE;

    if (multiline) {
        auto *scroller = Gtk::manage(new Gtk::ScrolledWindow());
        text_view_ = Gtk::manage(new Gtk::TextView());
        text_view_->get_buffer()->set_text(value);
        TextUndo::attach(*text_view_);
        text_view_->set_wrap_mode(Gtk::WRAP_WORD);
        scroller->add(*text_view_);
        scroller->set_shadow_type(Gtk::SHADOW_IN);
        editor_ = scroller;

        // Enter inserts a new line here; Ctrl+Enter keeps the value.
        text_view_->signal_key_press_event().connect(
            [this](GdkEventKey *e) {
                if (e->keyval == GDK_KEY_Escape) {
                    finish(false);
                    return true;
                }
                if ((e->keyval == GDK_KEY_Return || e->keyval == GDK_KEY_KP_Enter) &&
                    (e->state & GDK_CONTROL_MASK)) {
                    finish(true);
                    return true;
                }
                return false;
            },
            false);
        text_view_->signal_focus_out_event().connect([this](GdkEventFocus *) {
            finish(true);
            return false;
        });
    } else {
        entry_ = Gtk::manage(new Gtk::Entry());
        entry_->set_text(value);
        TextUndo::attach(*entry_);
        editor_ = entry_;

        entry_->signal_activate().connect([this] { finish(true); });
        entry_->signal_key_press_event().connect(
            [this](GdkEventKey *e) {
                if (e->keyval == GDK_KEY_Escape) {
                    finish(false);
                    return true;
                }
                return false;
            },
            false);
        entry_->signal_focus_out_event().connect([this](GdkEventFocus *) {
            finish(true);
            return false;
        });
    }

    ctx_.view.overlay().add_overlay(*editor_);
    ctx_.view.place_over_pdf_rect(*editor_, area.x1, area.y1, area.x2, area.y2);
    editor_->show_all();
    if (entry_) {
        entry_->grab_focus();
        entry_->select_region(0, -1);
    } else {
        text_view_->grab_focus();
    }
}

void FormTool::begin_choice_edit(PopplerFormField *field, const PopplerRectangle &area) {
    combo_ = Gtk::manage(new Gtk::ComboBoxText());
    const int n = poppler_form_field_choice_get_n_items(field);
    int selected = -1;
    for (int i = 0; i < n; ++i) {
        gchar *item = poppler_form_field_choice_get_item(field, i);
        combo_->append(item ? item : "");
        g_free(item);
        if (poppler_form_field_choice_is_item_selected(field, i)) selected = i;
    }
    if (selected >= 0) combo_->set_active(selected);
    orig_choice_ = selected;
    page_index_ = ctx_.model.current_page();
    editor_ = combo_;

    // Picking an item applies it immediately.
    combo_->signal_changed().connect([this] {
        chosen_item_ = combo_ ? combo_->get_active_row_number() : -1;
        finish(true);
    });

    ctx_.view.overlay().add_overlay(*editor_);
    ctx_.view.place_over_pdf_rect(*editor_, area.x1, area.y1, area.x2, area.y2);
    editor_->show_all();
    combo_->popup();
}

void FormTool::finish(bool keep) {
    if (!field_) return;  // nothing open (also stops re-entry from focus-out)

    PopplerFormField *field = field_;
    bool changed = false;

    if (keep) {
        FieldChange change;
        change.page = page_index_;
        change.id = poppler_form_field_get_id(field);
        if (entry_ || text_view_) {
            const std::string text = entry_ ? std::string(entry_->get_text())
                                            : std::string(text_view_->get_buffer()->get_text());
            if (text != orig_text_) {
                poppler_form_field_text_set_text(field, text.c_str());
                change.kind = FieldChange::Text;
                change.old_text = orig_text_;
                change.new_text = text;
                changed = true;
            }
        } else if (combo_ && chosen_item_ >= 0 && chosen_item_ != orig_choice_) {
            poppler_form_field_choice_unselect_all(field);
            poppler_form_field_choice_select_item(field, chosen_item_);
            change.kind = FieldChange::Choice;
            change.old_choice = orig_choice_;
            change.new_choice = chosen_item_;
            changed = true;
        }
        if (changed) {
            // The field must be alive until the step is recorded.
            record_change(ctx_, "Fill in field", change);
        }
    }

    field_ = nullptr;
    release_editor();
    if (changed) ctx_.model.notify_modified();
}

// Removes the floating editor and frees the field mapping. The widget is
// removed from an idle callback because this can run inside one of its own
// signal handlers, and a widget must not be destroyed while it is emitting.
void FormTool::release_editor() {
    Gtk::Widget *editor = editor_;
    editor_ = nullptr;
    entry_ = nullptr;
    text_view_ = nullptr;
    combo_ = nullptr;
    chosen_item_ = -1;

    if (mappings_) {
        poppler_page_free_form_field_mapping(mappings_);
        mappings_ = nullptr;
    }
    if (editor) {
        Gtk::Overlay &overlay = ctx_.view.overlay();
        Glib::signal_idle().connect_once([&overlay, editor] { static_cast<Gtk::Container &>(overlay).remove(*editor); });
    }
}

Forms::Forms(AppContext &ctx) { ctx.tools.emplace<FormTool>(ctx); }

}  // namespace efpdf
