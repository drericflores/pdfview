// pdfviewer.h - EFPDF (Easy & Flexible PDF) main window (GTKmm 3 + Poppler-GLib).
#pragma once

#include <gtkmm.h>
#include <poppler.h>

#include <functional>
#include <memory>
#include <string>

// Releases any GObject-derived Poppler object when the owner goes away.
struct GObjectDeleter {
    void operator()(gpointer p) const {
        if (p) g_object_unref(p);
    }
};
using DocumentPtr = std::unique_ptr<PopplerDocument, GObjectDeleter>;
using PagePtr = std::unique_ptr<PopplerPage, GObjectDeleter>;

class PdfViewer : public Gtk::Window {
public:
    PdfViewer();
    ~PdfViewer() override;

    // Opens `path`, asking to save unsaved edits first. Returns false on failure.
    bool open_file(const std::string &path);

protected:
    bool on_delete_event(GdkEventAny *event) override;

private:
    enum class Tool { Select, Highlight, Underline, Note };

    // --- construction -----------------------------------------------------
    void build_menus();
    void build_toolbar();
    void add_menu_item(Gtk::Menu &menu, const char *label,
                       const sigc::slot<void> &slot, guint key = 0,
                       Gdk::ModifierType mods = Gdk::ModifierType(0));

    // --- document lifecycle -----------------------------------------------
    bool load_document(const std::string &path, int page);
    bool confirm_discard();
    void close_document();
    void discard_work_file();
    bool save_to(const std::string &dest);
    bool commit(const std::string &dest);
    bool snapshot(std::string &tmp);
    std::string make_temp_path();

    // --- file actions -----------------------------------------------------
    void on_open();
    bool on_save();
    bool on_save_as();
    void on_close();
    void on_quit();
    void on_about();

    // --- navigation and view ----------------------------------------------
    void go_to_page(int page);
    void on_prev_page();
    void on_next_page();
    void set_scale(double scale);
    void on_dark_toggled();
    void on_tool_changed();

    // --- editing: structure (libqpdf) -------------------------------------
    using PageOp = std::function<bool(const std::string &in,
                                      const std::string &out, std::string &err)>;
    bool apply_page_op(const PageOp &op, int new_page);
    void on_rotate(int degrees);
    void on_delete_page();
    void on_move_page(int delta);
    void on_extract_pages();
    void on_merge();

    // --- editing: annotations and forms (Poppler) -------------------------
    void add_markup(double x0, double y0, double x1, double y1, bool underline);
    void add_note(double x, double y);
    void delete_annotation_at(double px, double py);
    void edit_form_field_at(double px, double py);
    bool prompt_text(const Glib::ustring &title, const Glib::ustring &label,
                     std::string &value, bool multiline);

    // --- rendering and input ----------------------------------------------
    void rebuild_page_surface();
    void rebuild_thumbnails();
    bool add_next_thumbnail();
    bool on_area_draw(const Cairo::RefPtr<Cairo::Context> &cr);
    bool on_area_press(GdkEventButton *event);
    bool on_area_motion(GdkEventMotion *event);
    bool on_area_release(GdkEventButton *event);
    void on_thumbnail_activated(Gtk::ListBoxRow *row);
    void update_title();
    void mark_dirty();
    void show_error(const Glib::ustring &message);
    int page_count() const;
    PagePtr current_page_object() const;

    // Converts a widget point to PDF user space (origin bottom-left, points).
    void to_pdf(double sx, double sy, double &px, double &py) const;

    // --- state ------------------------------------------------------------
    DocumentPtr doc_;
    std::string path_;       // file the user opened (target of Save)
    std::string work_path_;  // file Poppler currently reads (temp after page ops)
    int current_page_ = 0;
    double scale_ = 1.0;
    double page_w_ = 0, page_h_ = 0;  // current page size in points
    bool dark_ = false;
    bool dirty_ = false;
    Tool tool_ = Tool::Select;

    bool dragging_ = false;
    double drag_x0_ = 0, drag_y0_ = 0, drag_x1_ = 0, drag_y1_ = 0;

    Cairo::RefPtr<Cairo::ImageSurface> page_surface_;
    sigc::connection thumb_idle_;
    int next_thumb_ = 0;

    // --- widgets ----------------------------------------------------------
    Gtk::Box root_{Gtk::ORIENTATION_VERTICAL};
    Gtk::MenuBar menubar_;
    Gtk::Box toolbar_{Gtk::ORIENTATION_HORIZONTAL, 4};
    Gtk::Paned paned_{Gtk::ORIENTATION_HORIZONTAL};
    Gtk::ScrolledWindow thumb_scroll_;
    Gtk::ListBox thumb_list_;
    Gtk::ScrolledWindow page_scroll_;
    Gtk::DrawingArea area_;
    Gtk::ComboBoxText tool_combo_;
    Gtk::Label page_label_;
    Gtk::CheckMenuItem *dark_item_ = nullptr;
    Glib::RefPtr<Gtk::AccelGroup> accel_;
};
