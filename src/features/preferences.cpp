#include "features/preferences.h"

#include <cstdio>
#include <string>

#include "core/settings.h"
#include "ui/dialogs.h"
#include "ui/menu_builder.h"

namespace efpdf {
namespace {

// Defaults for every preference, in one place.
constexpr const char *kDefaultHighlight = "#ffff00";
constexpr const char *kDefaultUnderline = "#ff0000";
constexpr const char *kDefaultNote = "#ffff00";

bool is_folder(const std::string &path) {
    return !path.empty() && Glib::file_test(path, Glib::FILE_TEST_IS_DIR);
}

std::string to_hex(const Gdk::RGBA &c) {
    char buf[16];
    std::snprintf(buf, sizeof buf, "#%02x%02x%02x", static_cast<int>(c.get_red() * 255 + 0.5),
                  static_cast<int>(c.get_green() * 255 + 0.5),
                  static_cast<int>(c.get_blue() * 255 + 0.5));
    return buf;
}

Gdk::RGBA from_hex(const std::string &hex, const char *fallback) {
    Gdk::RGBA rgba;
    if (!rgba.set(hex)) rgba.set(fallback);
    return rgba;
}

// One titled group of settings.
Gtk::Box *add_section(Gtk::Box &parent, const char *title) {
    auto *frame = Gtk::manage(new Gtk::Frame());
    auto *heading = Gtk::manage(new Gtk::Label());
    heading->set_markup(std::string("<b>") + title + "</b>");
    frame->set_label_widget(*heading);
    frame->set_shadow_type(Gtk::SHADOW_NONE);

    auto *box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 6));
    box->set_margin_start(12);
    box->set_margin_top(6);
    box->set_margin_bottom(8);
    frame->add(*box);
    parent.pack_start(*frame, Gtk::PACK_SHRINK);
    return box;
}

void add_row(Gtk::Box &box, const char *caption, Gtk::Widget &widget) {
    auto *row = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
    auto *label = Gtk::manage(new Gtk::Label(caption));
    label->set_halign(Gtk::ALIGN_START);
    label->set_size_request(190, -1);
    row->pack_start(*label, Gtk::PACK_SHRINK);
    row->pack_start(widget, Gtk::PACK_SHRINK);
    box.pack_start(*row, Gtk::PACK_SHRINK);
}

class PreferencesDialog : public Gtk::Dialog {
public:
    PreferencesDialog(Gtk::Window &parent, Settings &settings)
        : Gtk::Dialog("Preferences", parent, true), s_(settings) {
        set_default_size(520, -1);
        add_button("Restore _Defaults", Gtk::RESPONSE_REJECT);
        add_button("_Close", Gtk::RESPONSE_CLOSE);
        set_default_response(Gtk::RESPONSE_CLOSE);

        Gtk::Box *content = get_content_area();
        content->set_spacing(8);
        content->set_border_width(10);

        // --- Files ---------------------------------------------------------
        Gtk::Box *files = add_section(*content, "Files");
        last_radio_.set_label("Open and save dialogs start in the folder I used last");
        fixed_radio_.set_label("Open and save dialogs always start in this folder:");
        Gtk::RadioButton::Group group = last_radio_.get_group();
        fixed_radio_.set_group(group);
        folder_button_ = Gtk::manage(new Gtk::FileChooserButton(
            "Choose a folder", Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER));
        files->pack_start(last_radio_, Gtk::PACK_SHRINK);
        files->pack_start(fixed_radio_, Gtk::PACK_SHRINK);
        folder_button_->set_margin_start(24);
        files->pack_start(*folder_button_, Gtk::PACK_SHRINK);
        reopen_check_.set_label("Reopen the last file when EFPDF starts");
        files->pack_start(reopen_check_, Gtk::PACK_SHRINK);

        // --- Appearance ----------------------------------------------------
        Gtk::Box *look = add_section(*content, "Appearance");
        dark_check_.set_label("Dark mode");
        look->pack_start(dark_check_, Gtk::PACK_SHRINK);
        zoom_combo_.append("remember", "Keep the last zoom level");
        zoom_combo_.append("fit_width", "Fit width");
        zoom_combo_.append("fit_page", "Fit page");
        zoom_combo_.append("actual", "Actual size (100%)");
        add_row(*look, "Zoom when opening a file:", zoom_combo_);

        // --- Annotations ---------------------------------------------------
        Gtk::Box *notes = add_section(*content, "Annotation colors");
        add_row(*notes, "Highlight:", highlight_button_);
        add_row(*notes, "Underline:", underline_button_);
        add_row(*notes, "Sticky note:", note_button_);
        auto *hint = Gtk::manage(new Gtk::Label(
            "Colors apply to annotations you add from now on."));
        hint->set_halign(Gtk::ALIGN_START);
        notes->pack_start(*hint, Gtk::PACK_SHRINK);

        load();
        connect_signals();
        show_all_children();
    }

private:
    // Fills the widgets from the saved settings.
    void load() {
        loading_ = true;
        const std::string folder = s_.get_string("general", "default_folder", "");
        if (is_folder(folder)) folder_button_->set_filename(folder);
        const bool fixed = s_.get_string("general", "folder_mode", "last") == "fixed";
        (fixed ? fixed_radio_ : last_radio_).set_active(true);
        folder_button_->set_sensitive(fixed);
        reopen_check_.set_active(s_.get_bool("general", "reopen_last", false));

        dark_check_.set_active(s_.get_bool("view", "dark", false));
        zoom_combo_.set_active_id(s_.get_string("view", "open_zoom", "remember"));
        if (zoom_combo_.get_active_row_number() < 0) zoom_combo_.set_active_id("remember");

        highlight_button_.set_rgba(
            from_hex(s_.get_string("annotations", "highlight_color", kDefaultHighlight), kDefaultHighlight));
        underline_button_.set_rgba(
            from_hex(s_.get_string("annotations", "underline_color", kDefaultUnderline), kDefaultUnderline));
        note_button_.set_rgba(
            from_hex(s_.get_string("annotations", "note_color", kDefaultNote), kDefaultNote));
        loading_ = false;
    }

    void connect_signals() {
        fixed_radio_.signal_toggled().connect([this] {
            if (loading_) return;
            const bool fixed = fixed_radio_.get_active();
            folder_button_->set_sensitive(fixed);
            s_.set_string("general", "folder_mode", fixed ? "fixed" : "last");
        });
        folder_button_->signal_selection_changed().connect([this] {
            if (loading_) return;
            const std::string folder = folder_button_->get_filename();
            if (is_folder(folder)) s_.set_string("general", "default_folder", folder);
        });
        reopen_check_.signal_toggled().connect([this] {
            if (!loading_) s_.set_bool("general", "reopen_last", reopen_check_.get_active());
        });
        dark_check_.signal_toggled().connect([this] {
            if (!loading_) s_.set_bool("view", "dark", dark_check_.get_active());
        });
        zoom_combo_.signal_changed().connect([this] {
            if (!loading_ && zoom_combo_.get_active_row_number() >= 0) {
                s_.set_string("view", "open_zoom", zoom_combo_.get_active_id());
            }
        });
        auto connect_color = [this](Gtk::ColorButton &button, const char *key) {
            button.signal_color_set().connect([this, &button, key] {
                if (!loading_) s_.set_string("annotations", key, to_hex(button.get_rgba()));
            });
        };
        connect_color(highlight_button_, "highlight_color");
        connect_color(underline_button_, "underline_color");
        connect_color(note_button_, "note_color");

        signal_response().connect([this](int response) {
            if (response != Gtk::RESPONSE_REJECT) return;
            if (!ui::ask_yes_no(*this, "Restore all preferences to their defaults?")) return;
            s_.set_string("general", "folder_mode", "last");
            s_.set_string("general", "default_folder", "");
            s_.set_bool("general", "reopen_last", false);
            s_.set_bool("view", "dark", false);
            s_.set_string("view", "open_zoom", "remember");
            s_.set_string("annotations", "highlight_color", kDefaultHighlight);
            s_.set_string("annotations", "underline_color", kDefaultUnderline);
            s_.set_string("annotations", "note_color", kDefaultNote);
            load();
            // Keep the dialog open after restoring.
            run_again_ = true;
        });
    }

public:
    // Runs until the user chooses Close ("Restore Defaults" keeps it open).
    void run_until_closed() {
        for (;;) {
            run_again_ = false;
            run();
            if (!run_again_) break;
        }
    }

private:
    Settings &s_;
    bool loading_ = false;
    bool run_again_ = false;

    Gtk::RadioButton last_radio_;
    Gtk::RadioButton fixed_radio_;
    Gtk::FileChooserButton *folder_button_ = nullptr;
    Gtk::CheckButton reopen_check_;
    Gtk::CheckButton dark_check_;
    Gtk::ComboBoxText zoom_combo_;
    Gtk::ColorButton highlight_button_;
    Gtk::ColorButton underline_button_;
    Gtk::ColorButton note_button_;
};

}  // namespace

Preferences::Preferences(AppContext &ctx) : ctx_(ctx) {
    ctx.menus.add_separator("_Edit");
    ctx.menus.add_item("_Edit", "_Preferences...", [this] { show_dialog(); }, GDK_KEY_comma,
                       Gdk::CONTROL_MASK);

    // File choosers consult the folder preferences each time they open.
    Settings *settings = &ctx.settings;
    ui::set_folder_hooks(
        {[settings]() -> std::string {
             if (settings->get_string("general", "folder_mode", "last") == "fixed") {
                 const std::string fixed = settings->get_string("general", "default_folder", "");
                 if (is_folder(fixed)) return fixed;
             }
             const std::string last = settings->get_string("general", "last_folder", "");
             return is_folder(last) ? last : std::string();
         },
         [settings](const std::string &folder) {
             settings->set_string("general", "last_folder", folder);
             settings->save();
         }});
}

Preferences::~Preferences() { ui::set_folder_hooks({}); }

void Preferences::show_dialog() {
    PreferencesDialog dialog(ctx_.window, ctx_.settings);
    dialog.run_until_closed();
    ctx_.settings.save();
}

}  // namespace efpdf
