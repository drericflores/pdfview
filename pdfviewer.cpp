#include "pdfviewer.h"

#include "pdfedit.h"

#include <glib/gstdio.h>

#include <algorithm>
#include <cmath>
#include <vector>

namespace {

constexpr double kMinScale = 0.25;
constexpr double kMaxScale = 5.0;
constexpr double kZoomStep = 1.2;
constexpr double kThumbHeight = 80.0;

// Poppler wants absolute file:// URIs.
std::string to_uri(const std::string &path) {
    gchar *abs = g_canonicalize_filename(path.c_str(), nullptr);
    gchar *uri = g_filename_to_uri(abs, nullptr, nullptr);
    std::string result = uri ? uri : "";
    g_free(abs);
    g_free(uri);
    return result;
}

bool inside(const PopplerRectangle &r, double x, double y) {
    return x >= std::min(r.x1, r.x2) && x <= std::max(r.x1, r.x2) &&
           y >= std::min(r.y1, r.y2) && y <= std::max(r.y1, r.y2);
}

Glib::RefPtr<Gtk::FileFilter> pdf_filter() {
    auto filter = Gtk::FileFilter::create();
    filter->set_name("PDF files");
    filter->add_mime_type("application/pdf");
    filter->add_pattern("*.pdf");
    filter->add_pattern("*.PDF");
    return filter;
}

bool choose_pdf(Gtk::Window &parent, const Glib::ustring &title,
                Gtk::FileChooserAction action, std::string &result,
                const std::string &suggested = "") {
    Gtk::FileChooserDialog dlg(title, action);
    dlg.set_transient_for(parent);
    dlg.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    const bool saving = action == Gtk::FILE_CHOOSER_ACTION_SAVE;
    dlg.add_button(saving ? "_Save" : "_Open", Gtk::RESPONSE_ACCEPT);
    dlg.add_filter(pdf_filter());
    if (saving) {
        dlg.set_do_overwrite_confirmation(true);
        if (!suggested.empty()) dlg.set_current_name(suggested);
    }
    if (dlg.run() != Gtk::RESPONSE_ACCEPT) return false;
    result = dlg.get_filename();
    return !result.empty();
}

bool choose_pdfs(Gtk::Window &parent, const Glib::ustring &title,
                 std::vector<std::string> &result) {
    Gtk::FileChooserDialog dlg(title, Gtk::FILE_CHOOSER_ACTION_OPEN);
    dlg.set_transient_for(parent);
    dlg.set_select_multiple(true);
    dlg.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    dlg.add_button("_Open", Gtk::RESPONSE_ACCEPT);
    dlg.add_filter(pdf_filter());
    if (dlg.run() != Gtk::RESPONSE_ACCEPT) return false;
    result = dlg.get_filenames();
    return !result.empty();
}

}  // namespace

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

PdfViewer::PdfViewer() {
    set_default_size(1000, 750);

    accel_ = Gtk::AccelGroup::create();
    add_accel_group(accel_);

    add(root_);
    build_menus();
    build_toolbar();

    thumb_scroll_.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    thumb_scroll_.set_size_request(130, -1);
    thumb_scroll_.add(thumb_list_);
    thumb_list_.signal_row_activated().connect(
        sigc::mem_fun(*this, &PdfViewer::on_thumbnail_activated));

    area_.set_halign(Gtk::ALIGN_CENTER);
    area_.set_valign(Gtk::ALIGN_CENTER);
    area_.add_events(Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK |
                     Gdk::BUTTON1_MOTION_MASK);
    area_.signal_draw().connect(sigc::mem_fun(*this, &PdfViewer::on_area_draw));
    area_.signal_button_press_event().connect(
        sigc::mem_fun(*this, &PdfViewer::on_area_press));
    area_.signal_motion_notify_event().connect(
        sigc::mem_fun(*this, &PdfViewer::on_area_motion));
    area_.signal_button_release_event().connect(
        sigc::mem_fun(*this, &PdfViewer::on_area_release));
    page_scroll_.add(area_);

    paned_.pack1(thumb_scroll_, false, false);
    paned_.pack2(page_scroll_, true, true);

    root_.pack_start(menubar_, Gtk::PACK_SHRINK);
    root_.pack_start(toolbar_, Gtk::PACK_SHRINK);
    root_.pack_start(paned_, Gtk::PACK_EXPAND_WIDGET);

    show_all_children();
    update_title();
}

PdfViewer::~PdfViewer() {
    thumb_idle_.disconnect();
    discard_work_file();
}

void PdfViewer::add_menu_item(Gtk::Menu &menu, const char *label,
                              const sigc::slot<void> &slot, guint key,
                              Gdk::ModifierType mods) {
    auto *item = Gtk::manage(new Gtk::MenuItem(label, true));
    item->signal_activate().connect(slot);
    if (key != 0) {
        item->add_accelerator("activate", accel_, key, mods, Gtk::ACCEL_VISIBLE);
    }
    menu.append(*item);
}

void PdfViewer::build_menus() {
    const Gdk::ModifierType ctrl = Gdk::CONTROL_MASK;
    const Gdk::ModifierType ctrl_shift = Gdk::CONTROL_MASK | Gdk::SHIFT_MASK;
    const Gdk::ModifierType none = Gdk::ModifierType(0);

    auto attach = [this](const char *title, Gtk::Menu *menu) {
        auto *top = Gtk::manage(new Gtk::MenuItem(title, true));
        top->set_submenu(*menu);
        menubar_.append(*top);
    };

    // File
    auto *file = Gtk::manage(new Gtk::Menu());
    add_menu_item(*file, "_Open...", sigc::mem_fun(*this, &PdfViewer::on_open),
                  GDK_KEY_o, ctrl);
    add_menu_item(*file, "_Save", [this] { on_save(); }, GDK_KEY_s, ctrl);
    add_menu_item(*file, "Save _As...", [this] { on_save_as(); }, GDK_KEY_s,
                  ctrl_shift);
    add_menu_item(*file, "_Close", sigc::mem_fun(*this, &PdfViewer::on_close),
                  GDK_KEY_w, ctrl);
    file->append(*Gtk::manage(new Gtk::SeparatorMenuItem()));
    add_menu_item(*file, "_Quit", sigc::mem_fun(*this, &PdfViewer::on_quit),
                  GDK_KEY_q, ctrl);
    attach("_File", file);

    // Edit
    auto *edit = Gtk::manage(new Gtk::Menu());
    add_menu_item(*edit, "Rotate Page _Left", [this] { on_rotate(-90); },
                  GDK_KEY_l, ctrl);
    add_menu_item(*edit, "Rotate Page _Right", [this] { on_rotate(90); },
                  GDK_KEY_r, ctrl);
    add_menu_item(*edit, "_Delete Page", sigc::mem_fun(*this, &PdfViewer::on_delete_page));
    add_menu_item(*edit, "Move Page _Up", [this] { on_move_page(-1); },
                  GDK_KEY_Up, Gdk::CONTROL_MASK | Gdk::MOD1_MASK);
    add_menu_item(*edit, "Move Page Do_wn", [this] { on_move_page(1); },
                  GDK_KEY_Down, Gdk::CONTROL_MASK | Gdk::MOD1_MASK);
    edit->append(*Gtk::manage(new Gtk::SeparatorMenuItem()));
    add_menu_item(*edit, "_Extract Pages...", sigc::mem_fun(*this, &PdfViewer::on_extract_pages));
    add_menu_item(*edit, "_Merge PDFs...", sigc::mem_fun(*this, &PdfViewer::on_merge));
    attach("_Edit", edit);

    // View
    auto *view = Gtk::manage(new Gtk::Menu());
    add_menu_item(*view, "Zoom _In", [this] { set_scale(scale_ * kZoomStep); },
                  GDK_KEY_plus, ctrl);
    add_menu_item(*view, "Zoom In (=)", [this] { set_scale(scale_ * kZoomStep); },
                  GDK_KEY_equal, ctrl);
    add_menu_item(*view, "Zoom _Out", [this] { set_scale(scale_ / kZoomStep); },
                  GDK_KEY_minus, ctrl);
    add_menu_item(*view, "_Actual Size", [this] { set_scale(1.0); }, GDK_KEY_0, ctrl);
    view->append(*Gtk::manage(new Gtk::SeparatorMenuItem()));
    add_menu_item(*view, "_Previous Page", sigc::mem_fun(*this, &PdfViewer::on_prev_page),
                  GDK_KEY_Page_Up, none);
    add_menu_item(*view, "_Next Page", sigc::mem_fun(*this, &PdfViewer::on_next_page),
                  GDK_KEY_Page_Down, none);
    view->append(*Gtk::manage(new Gtk::SeparatorMenuItem()));
    dark_item_ = Gtk::manage(new Gtk::CheckMenuItem("_Dark Mode", true));
    dark_item_->signal_toggled().connect(sigc::mem_fun(*this, &PdfViewer::on_dark_toggled));
    dark_item_->add_accelerator("activate", accel_, GDK_KEY_d, ctrl, Gtk::ACCEL_VISIBLE);
    view->append(*dark_item_);
    attach("_View", view);

    // Help
    auto *help = Gtk::manage(new Gtk::Menu());
    add_menu_item(*help, "_About", sigc::mem_fun(*this, &PdfViewer::on_about));
    attach("_Help", help);
}

void PdfViewer::build_toolbar() {
    toolbar_.set_border_width(4);

    auto add_button = [this](const char *label, const sigc::slot<void> &slot) {
        auto *button = Gtk::manage(new Gtk::Button(label, true));
        button->signal_clicked().connect(slot);
        toolbar_.pack_start(*button, Gtk::PACK_SHRINK);
    };

    add_button("_Open", sigc::mem_fun(*this, &PdfViewer::on_open));
    add_button("_Save", [this] { on_save(); });
    add_button("_Previous", sigc::mem_fun(*this, &PdfViewer::on_prev_page));
    add_button("_Next", sigc::mem_fun(*this, &PdfViewer::on_next_page));
    toolbar_.pack_start(page_label_, Gtk::PACK_SHRINK);
    add_button("Zoom _-", [this] { set_scale(scale_ / kZoomStep); });
    add_button("Zoom _+", [this] { set_scale(scale_ * kZoomStep); });

    tool_combo_.append("Select / Fill forms");
    tool_combo_.append("Highlight");
    tool_combo_.append("Underline");
    tool_combo_.append("Add note");
    tool_combo_.set_active(0);
    tool_combo_.signal_changed().connect(sigc::mem_fun(*this, &PdfViewer::on_tool_changed));
    toolbar_.pack_end(tool_combo_, Gtk::PACK_SHRINK);
}

// ---------------------------------------------------------------------------
// Document lifecycle
// ---------------------------------------------------------------------------

int PdfViewer::page_count() const {
    return doc_ ? poppler_document_get_n_pages(doc_.get()) : 0;
}

PagePtr PdfViewer::current_page_object() const {
    if (!doc_) return PagePtr();
    return PagePtr(poppler_document_get_page(doc_.get(), current_page_));
}

bool PdfViewer::load_document(const std::string &path, int page) {
    const std::string uri = to_uri(path);
    if (uri.empty()) {
        show_error("Could not resolve the file path.");
        return false;
    }

    std::string password;
    DocumentPtr loaded;
    for (;;) {
        GError *error = nullptr;
        PopplerDocument *d = poppler_document_new_from_file(
            uri.c_str(), password.empty() ? nullptr : password.c_str(), &error);
        if (d) {
            loaded.reset(d);
            break;
        }
        const bool encrypted = error && error->code == POPPLER_ERROR_ENCRYPTED;
        const std::string message = error ? error->message : "unknown error";
        if (error) g_error_free(error);
        if (!encrypted) {
            show_error("Could not open the PDF: " + message);
            return false;
        }
        if (!prompt_text("Password required",
                         "This PDF is encrypted. Enter its password:", password,
                         false)) {
            return false;
        }
    }

    // Stop thumbnail rendering before the old document goes away.
    thumb_idle_.disconnect();
    doc_ = std::move(loaded);
    current_page_ = std::clamp(page, 0, std::max(0, page_count() - 1));
    rebuild_page_surface();
    rebuild_thumbnails();
    update_title();
    return true;
}

bool PdfViewer::open_file(const std::string &path) {
    if (!confirm_discard()) return false;
    if (!load_document(path, 0)) return false;
    discard_work_file();
    path_ = path;
    work_path_ = path;
    dirty_ = false;
    update_title();
    return true;
}

bool PdfViewer::confirm_discard() {
    if (!dirty_) return true;
    Gtk::MessageDialog dlg(*this, "Save changes before closing?", false,
                           Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_NONE, true);
    dlg.set_secondary_text("Your edits will be lost if you don't save them.");
    dlg.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    dlg.add_button("_Discard", Gtk::RESPONSE_REJECT);
    dlg.add_button("_Save", Gtk::RESPONSE_ACCEPT);
    switch (dlg.run()) {
        case Gtk::RESPONSE_ACCEPT: return on_save();
        case Gtk::RESPONSE_REJECT: return true;
        default: return false;
    }
}

void PdfViewer::close_document() {
    thumb_idle_.disconnect();
    for (auto *child : thumb_list_.get_children()) thumb_list_.remove(*child);
    doc_.reset();
    page_surface_.reset();
    discard_work_file();
    path_.clear();
    dirty_ = false;
    current_page_ = 0;
    area_.set_size_request(0, 0);
    area_.queue_draw();
    update_title();
}

// Removes the temporary file created by page operations, if one is in use.
void PdfViewer::discard_work_file() {
    if (!work_path_.empty() && work_path_ != path_) g_remove(work_path_.c_str());
    work_path_.clear();
}

std::string PdfViewer::make_temp_path() {
    gchar *name = nullptr;
    GError *error = nullptr;
    int fd = g_file_open_tmp("efpdf-XXXXXX.pdf", &name, &error);
    if (fd < 0) {
        if (error) g_error_free(error);
        return "";
    }
    g_close(fd, nullptr);
    std::string path = name;
    g_free(name);
    return path;
}

// Writes the current in-memory state (including annotations and form values)
// to a temporary file.
bool PdfViewer::snapshot(std::string &tmp) {
    tmp = make_temp_path();
    if (tmp.empty()) {
        show_error("Could not create a temporary file.");
        return false;
    }
    GError *error = nullptr;
    if (!poppler_document_save_a_copy(doc_.get(), to_uri(tmp).c_str(), &error)) {
        show_error(std::string("Could not prepare the document: ") +
                   (error ? error->message : "unknown error"));
        if (error) g_error_free(error);
        g_remove(tmp.c_str());
        return false;
    }
    return true;
}

// Saves to a sibling temp file and renames it over `dest`, so a failed write
// can never destroy the existing file.
bool PdfViewer::save_to(const std::string &dest) {
    const std::string tmp = dest + ".efpdf-tmp";
    GError *error = nullptr;
    if (!poppler_document_save_a_copy(doc_.get(), to_uri(tmp).c_str(), &error)) {
        show_error(std::string("Could not save: ") +
                   (error ? error->message : "unknown error"));
        if (error) g_error_free(error);
        g_remove(tmp.c_str());
        return false;
    }

    GStatBuf st;
    if (g_stat(dest.c_str(), &st) == 0) g_chmod(tmp.c_str(), st.st_mode & 07777);

    if (g_rename(tmp.c_str(), dest.c_str()) != 0) {
        show_error("Could not replace " + dest);
        g_remove(tmp.c_str());
        return false;
    }
    return true;
}

// Saves to `dest`, then reloads from it so Poppler's state matches the file.
bool PdfViewer::commit(const std::string &dest) {
    if (!doc_) return false;
    if (!save_to(dest)) return false;

    const std::string old_work = work_path_;
    const std::string old_path = path_;
    const bool reloaded = load_document(dest, current_page_);
    if (reloaded) {
        if (!old_work.empty() && old_work != old_path && old_work != dest) {
            g_remove(old_work.c_str());
        }
        work_path_ = dest;
    }
    path_ = dest;
    dirty_ = false;
    update_title();
    return true;
}

// ---------------------------------------------------------------------------
// File actions
// ---------------------------------------------------------------------------

void PdfViewer::on_open() {
    std::string path;
    if (choose_pdf(*this, "Open PDF", Gtk::FILE_CHOOSER_ACTION_OPEN, path)) {
        open_file(path);
    }
}

bool PdfViewer::on_save() {
    if (!doc_ || path_.empty()) return false;
    return commit(path_);
}

bool PdfViewer::on_save_as() {
    if (!doc_) return false;
    std::string dest;
    if (!choose_pdf(*this, "Save PDF As", Gtk::FILE_CHOOSER_ACTION_SAVE, dest,
                    Glib::path_get_basename(path_))) {
        return false;
    }
    return commit(dest);
}

void PdfViewer::on_close() {
    if (confirm_discard()) close_document();
}

void PdfViewer::on_quit() {
    if (confirm_discard()) hide();
}

bool PdfViewer::on_delete_event(GdkEventAny *event) {
    if (!confirm_discard()) return true;  // keep the window open
    return Gtk::Window::on_delete_event(event);
}

void PdfViewer::on_about() {
    Gtk::AboutDialog dlg;
    dlg.set_transient_for(*this);
    dlg.set_program_name("EFPDF");
    dlg.set_comments(
        "EFPDF (Easy & Flexible PDF): a lightweight PDF viewer and editor built with GTKmm 3, "
        "Poppler-GLib and QPDF.");
    dlg.set_authors({"Dr. Eric O. Flores"});
    dlg.set_license_type(Gtk::LICENSE_MIT_X11);
    dlg.set_website("https://github.com/drericflores/pdfview");
    dlg.run();
}

// ---------------------------------------------------------------------------
// Navigation and view
// ---------------------------------------------------------------------------

void PdfViewer::go_to_page(int page) {
    if (!doc_) return;
    page = std::clamp(page, 0, page_count() - 1);
    if (page == current_page_) return;
    current_page_ = page;
    rebuild_page_surface();
    update_title();
    if (auto *row = thumb_list_.get_row_at_index(page)) thumb_list_.select_row(*row);
}

void PdfViewer::on_prev_page() { go_to_page(current_page_ - 1); }
void PdfViewer::on_next_page() { go_to_page(current_page_ + 1); }

void PdfViewer::set_scale(double scale) {
    scale_ = std::clamp(scale, kMinScale, kMaxScale);
    rebuild_page_surface();
}

void PdfViewer::on_dark_toggled() {
    dark_ = dark_item_->get_active();
    Gtk::Settings::get_default()->property_gtk_application_prefer_dark_theme() = dark_;
    rebuild_page_surface();
}

void PdfViewer::on_tool_changed() {
    const int index = tool_combo_.get_active_row_number();
    tool_ = static_cast<Tool>(std::clamp(index, 0, 3));
    dragging_ = false;
}

void PdfViewer::on_thumbnail_activated(Gtk::ListBoxRow *row) {
    if (row) go_to_page(row->get_index());
}

void PdfViewer::to_pdf(double sx, double sy, double &px, double &py) const {
    px = sx / scale_;
    py = page_h_ - sy / scale_;
}

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------

void PdfViewer::rebuild_page_surface() {
    page_surface_.reset();
    PagePtr page = current_page_object();
    if (!page) {
        area_.set_size_request(0, 0);
        area_.queue_draw();
        return;
    }

    poppler_page_get_size(page.get(), &page_w_, &page_h_);
    const int w = std::max(1, static_cast<int>(std::ceil(page_w_ * scale_)));
    const int h = std::max(1, static_cast<int>(std::ceil(page_h_ * scale_)));

    page_surface_ = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, w, h);
    auto cr = Cairo::Context::create(page_surface_);

    // Poppler draws onto a transparent canvas, so lay down paper first.
    cr->set_source_rgb(1.0, 1.0, 1.0);
    cr->paint();
    cr->save();
    cr->scale(scale_, scale_);
    poppler_page_render(page.get(), cr->cobj());
    cr->restore();

    if (dark_) {
        // Invert the finished page (white paper -> black, black text -> white).
        cr->set_operator(Cairo::OPERATOR_DIFFERENCE);
        cr->set_source_rgb(1.0, 1.0, 1.0);
        cr->paint();
    }

    area_.set_size_request(w, h);
    area_.queue_draw();
}

void PdfViewer::rebuild_thumbnails() {
    thumb_idle_.disconnect();
    for (auto *child : thumb_list_.get_children()) thumb_list_.remove(*child);
    next_thumb_ = 0;
    if (!doc_) return;
    thumb_idle_ = Glib::signal_idle().connect(
        sigc::mem_fun(*this, &PdfViewer::add_next_thumbnail));
}

// Renders one thumbnail per idle callback so large documents open instantly.
bool PdfViewer::add_next_thumbnail() {
    if (!doc_ || next_thumb_ >= page_count()) return false;

    auto *box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 2));
    PagePtr page(poppler_document_get_page(doc_.get(), next_thumb_));
    if (page) {
        double w = 0, h = 0;
        poppler_page_get_size(page.get(), &w, &h);
        const double s = h > 0 ? kThumbHeight / h : 1.0;
        const int iw = std::max(1, static_cast<int>(w * s));
        const int ih = std::max(1, static_cast<int>(kThumbHeight));

        auto surface = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, iw, ih);
        auto cr = Cairo::Context::create(surface);
        cr->set_source_rgb(1.0, 1.0, 1.0);
        cr->paint();
        cr->scale(s, s);
        poppler_page_render(page.get(), cr->cobj());

        GdkPixbuf *raw = gdk_pixbuf_get_from_surface(surface->cobj(), 0, 0, iw, ih);
        if (raw) {
            Glib::RefPtr<Gdk::Pixbuf> pixbuf = Glib::wrap(raw);
            box->pack_start(*Gtk::manage(new Gtk::Image(pixbuf)), Gtk::PACK_SHRINK);
        }
    }
    box->pack_start(*Gtk::manage(new Gtk::Label(std::to_string(next_thumb_ + 1))),
                    Gtk::PACK_SHRINK);

    auto *row = Gtk::manage(new Gtk::ListBoxRow());
    row->add(*box);
    thumb_list_.append(*row);
    row->show_all();
    if (next_thumb_ == current_page_) thumb_list_.select_row(*row);

    ++next_thumb_;
    return true;
}

bool PdfViewer::on_area_draw(const Cairo::RefPtr<Cairo::Context> &cr) {
    if (!page_surface_) return true;
    cr->set_source(page_surface_, 0, 0);
    cr->paint();

    if (dragging_) {
        const double x = std::min(drag_x0_, drag_x1_);
        const double y = std::min(drag_y0_, drag_y1_);
        const double w = std::abs(drag_x1_ - drag_x0_);
        const double h = std::abs(drag_y1_ - drag_y0_);
        cr->rectangle(x, y, w, h);
        cr->set_source_rgba(0.2, 0.5, 1.0, 0.25);
        cr->fill_preserve();
        cr->set_source_rgba(0.2, 0.5, 1.0, 0.9);
        cr->set_line_width(1.0);
        cr->stroke();
    }
    return true;
}

void PdfViewer::update_title() {
    if (!doc_) {
        set_title("EFPDF");
        page_label_.set_text("");
        return;
    }
    const std::string name = Glib::path_get_basename(path_);
    const std::string pages = "Page " + std::to_string(current_page_ + 1) + " of " +
                              std::to_string(page_count());
    set_title(std::string(dirty_ ? "*" : "") + name + " - " + pages + " - EFPDF");
    page_label_.set_text("  " + pages + "  ");
}

void PdfViewer::mark_dirty() {
    dirty_ = true;
    rebuild_page_surface();
    update_title();
}

void PdfViewer::show_error(const Glib::ustring &message) {
    Gtk::MessageDialog dlg(*this, message, false, Gtk::MESSAGE_ERROR,
                           Gtk::BUTTONS_OK, true);
    dlg.run();
}

// ---------------------------------------------------------------------------
// Mouse input
// ---------------------------------------------------------------------------

bool PdfViewer::on_area_press(GdkEventButton *event) {
    if (!doc_) return false;

    double px = 0, py = 0;
    to_pdf(event->x, event->y, px, py);

    if (event->button == 3) {
        delete_annotation_at(px, py);
        return true;
    }
    if (event->button != 1) return false;

    switch (tool_) {
        case Tool::Select:
            edit_form_field_at(px, py);
            break;
        case Tool::Note:
            add_note(px, py);
            break;
        case Tool::Highlight:
        case Tool::Underline:
            dragging_ = true;
            drag_x0_ = drag_x1_ = event->x;
            drag_y0_ = drag_y1_ = event->y;
            break;
    }
    return true;
}

bool PdfViewer::on_area_motion(GdkEventMotion *event) {
    if (!dragging_) return false;
    drag_x1_ = event->x;
    drag_y1_ = event->y;
    area_.queue_draw();
    return true;
}

bool PdfViewer::on_area_release(GdkEventButton *event) {
    if (!dragging_ || event->button != 1) return false;
    dragging_ = false;
    drag_x1_ = event->x;
    drag_y1_ = event->y;
    if (std::abs(drag_x1_ - drag_x0_) > 3 && std::abs(drag_y1_ - drag_y0_) > 3) {
        add_markup(drag_x0_, drag_y0_, drag_x1_, drag_y1_, tool_ == Tool::Underline);
    } else {
        area_.queue_draw();
    }
    return true;
}

// ---------------------------------------------------------------------------
// Editing: annotations and forms (Poppler)
// ---------------------------------------------------------------------------

// Takes widget coordinates of the dragged rectangle.
void PdfViewer::add_markup(double x0, double y0, double x1, double y1,
                           bool underline) {
    PagePtr page = current_page_object();
    if (!page) return;

    double ax, ay, bx, by;
    to_pdf(x0, y0, ax, ay);
    to_pdf(x1, y1, bx, by);
    PopplerRectangle rect{std::min(ax, bx), std::min(ay, by), std::max(ax, bx),
                          std::max(ay, by)};

    GArray *quads = g_array_new(FALSE, TRUE, sizeof(PopplerQuadrilateral));
    PopplerQuadrilateral quad;
    quad.p1 = PopplerPoint{rect.x1, rect.y2};  // top-left
    quad.p2 = PopplerPoint{rect.x2, rect.y2};  // top-right
    quad.p3 = PopplerPoint{rect.x1, rect.y1};  // bottom-left
    quad.p4 = PopplerPoint{rect.x2, rect.y1};  // bottom-right
    g_array_append_val(quads, quad);

    PopplerAnnot *annot =
        underline
            ? poppler_annot_text_markup_new_underline(doc_.get(), &rect, quads)
            : poppler_annot_text_markup_new_highlight(doc_.get(), &rect, quads);
    g_array_free(quads, TRUE);
    if (!annot) {
        show_error("Could not create the annotation.");
        return;
    }

    PopplerColor color = underline ? PopplerColor{0xffff, 0x0000, 0x0000}
                                   : PopplerColor{0xffff, 0xffff, 0x0000};
    poppler_annot_set_color(annot, &color);
    poppler_page_add_annot(page.get(), annot);
    g_object_unref(annot);
    mark_dirty();
}

void PdfViewer::add_note(double x, double y) {
    PagePtr page = current_page_object();
    if (!page) return;

    std::string text;
    if (!prompt_text("Add note", "Note text:", text, true) || text.empty()) return;

    PopplerRectangle rect{x, y - 24.0, x + 24.0, y};
    PopplerAnnot *annot = poppler_annot_text_new(doc_.get(), &rect);
    if (!annot) {
        show_error("Could not create the note.");
        return;
    }
    poppler_annot_set_contents(annot, text.c_str());
    PopplerColor color{0xffff, 0xffff, 0x0000};
    poppler_annot_set_color(annot, &color);
    poppler_page_add_annot(page.get(), annot);
    g_object_unref(annot);
    mark_dirty();
}

// Right-click removes a markup or note annotation under the pointer.
void PdfViewer::delete_annotation_at(double px, double py) {
    PagePtr page = current_page_object();
    if (!page) return;

    GList *mappings = poppler_page_get_annot_mapping(page.get());
    bool changed = false;
    for (GList *l = mappings; l; l = l->next) {
        auto *m = static_cast<PopplerAnnotMapping *>(l->data);
        const PopplerAnnotType type = poppler_annot_get_annot_type(m->annot);
        if (type == POPPLER_ANNOT_WIDGET || type == POPPLER_ANNOT_LINK) continue;
        if (!inside(m->area, px, py)) continue;

        Gtk::MessageDialog dlg(*this, "Delete this annotation?", false,
                               Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);
        if (dlg.run() == Gtk::RESPONSE_YES) {
            poppler_page_remove_annot(page.get(), m->annot);
            changed = true;
        }
        break;
    }
    poppler_page_free_annot_mapping(mappings);
    if (changed) mark_dirty();
}

// Fills the form field under the pointer (text, check box, radio, choice).
void PdfViewer::edit_form_field_at(double px, double py) {
    PagePtr page = current_page_object();
    if (!page) return;

    GList *mappings = poppler_page_get_form_field_mapping(page.get());
    bool changed = false;
    for (GList *l = mappings; l; l = l->next) {
        auto *m = static_cast<PopplerFormFieldMapping *>(l->data);
        if (!inside(m->area, px, py)) continue;
        PopplerFormField *field = m->field;
        if (poppler_form_field_is_read_only(field)) break;

        switch (poppler_form_field_get_field_type(field)) {
            case POPPLER_FORM_FIELD_TEXT: {
                gchar *current = poppler_form_field_text_get_text(field);
                std::string value = current ? current : "";
                g_free(current);
                const bool multiline =
                    poppler_form_field_text_get_text_type(field) ==
                    POPPLER_FORM_TEXT_MULTILINE;
                if (prompt_text("Edit field", "Value:", value, multiline)) {
                    poppler_form_field_text_set_text(field, value.c_str());
                    changed = true;
                }
                break;
            }
            case POPPLER_FORM_FIELD_BUTTON: {
                const PopplerFormButtonType type =
                    poppler_form_field_button_get_button_type(field);
                const gboolean state = poppler_form_field_button_get_state(field);
                if (type == POPPLER_FORM_BUTTON_CHECK) {
                    poppler_form_field_button_set_state(field, !state);
                    changed = true;
                } else if (type == POPPLER_FORM_BUTTON_RADIO && !state) {
                    poppler_form_field_button_set_state(field, TRUE);
                    changed = true;
                }
                break;
            }
            case POPPLER_FORM_FIELD_CHOICE: {
                Gtk::Dialog dlg("Choose a value", *this, true);
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
                if (dlg.run() == Gtk::RESPONSE_OK && combo.get_active_row_number() >= 0) {
                    poppler_form_field_choice_unselect_all(field);
                    poppler_form_field_choice_select_item(field,
                                                          combo.get_active_row_number());
                    changed = true;
                }
                break;
            }
            default:
                break;
        }
        break;
    }
    poppler_page_free_form_field_mapping(mappings);
    if (changed) mark_dirty();
}

bool PdfViewer::prompt_text(const Glib::ustring &title, const Glib::ustring &label,
                            std::string &value, bool multiline) {
    Gtk::Dialog dlg(title, *this, true);
    dlg.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    dlg.add_button("_OK", Gtk::RESPONSE_OK);
    dlg.set_default_response(Gtk::RESPONSE_OK);

    Gtk::Box *content = dlg.get_content_area();
    content->set_spacing(6);
    Gtk::Label caption(label);
    caption.set_halign(Gtk::ALIGN_START);
    content->pack_start(caption, Gtk::PACK_SHRINK);

    Gtk::Entry entry;
    Gtk::TextView text_view;
    Gtk::ScrolledWindow scroller;
    if (multiline) {
        text_view.get_buffer()->set_text(value);
        text_view.set_wrap_mode(Gtk::WRAP_WORD);
        scroller.add(text_view);
        scroller.set_size_request(360, 140);
        content->pack_start(scroller, Gtk::PACK_EXPAND_WIDGET);
    } else {
        entry.set_text(value);
        entry.set_activates_default(true);
        content->pack_start(entry, Gtk::PACK_SHRINK);
    }
    dlg.show_all_children();

    if (dlg.run() != Gtk::RESPONSE_OK) return false;
    value = multiline ? std::string(text_view.get_buffer()->get_text())
                      : std::string(entry.get_text());
    return true;
}

// ---------------------------------------------------------------------------
// Editing: document structure (libqpdf)
// ---------------------------------------------------------------------------

// Snapshots the current state, runs `op` on it, and loads the result. The
// original file is untouched until the user saves.
bool PdfViewer::apply_page_op(const PageOp &op, int new_page) {
    if (!doc_) return false;

    std::string in;
    if (!snapshot(in)) return false;
    const std::string out = make_temp_path();
    if (out.empty()) {
        g_remove(in.c_str());
        show_error("Could not create a temporary file.");
        return false;
    }

    std::string err;
    const bool ok = op(in, out, err);
    g_remove(in.c_str());
    if (!ok) {
        g_remove(out.c_str());
        show_error("The operation failed: " + err);
        return false;
    }

    const std::string old_work = work_path_;
    if (!load_document(out, new_page)) {
        g_remove(out.c_str());
        return false;
    }
    if (!old_work.empty() && old_work != path_) g_remove(old_work.c_str());
    work_path_ = out;
    dirty_ = true;
    update_title();
    return true;
}

void PdfViewer::on_rotate(int degrees) {
    if (!doc_) return;
    const int page = current_page_;
    apply_page_op(
        [page, degrees](const std::string &in, const std::string &out,
                        std::string &err) {
            return pdfedit::rotate_page(in, out, page, degrees, err);
        },
        page);
}

void PdfViewer::on_delete_page() {
    if (!doc_) return;
    if (page_count() < 2) {
        show_error("A PDF must keep at least one page.");
        return;
    }
    Gtk::MessageDialog dlg(*this, "Delete page " + std::to_string(current_page_ + 1) + "?",
                           false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);
    if (dlg.run() != Gtk::RESPONSE_YES) return;

    const int page = current_page_;
    apply_page_op(
        [page](const std::string &in, const std::string &out, std::string &err) {
            return pdfedit::delete_page(in, out, page, err);
        },
        page);
}

void PdfViewer::on_move_page(int delta) {
    if (!doc_) return;
    const int from = current_page_;
    const int to = from + delta;
    if (to < 0 || to >= page_count()) return;
    apply_page_op(
        [from, to](const std::string &in, const std::string &out, std::string &err) {
            return pdfedit::move_page(in, out, from, to, err);
        },
        to);
}

void PdfViewer::on_extract_pages() {
    if (!doc_) return;
    const int n = page_count();

    Gtk::Dialog dlg("Extract pages", *this, true);
    dlg.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    dlg.add_button("_Extract", Gtk::RESPONSE_OK);

    Gtk::Grid grid;
    grid.set_row_spacing(6);
    grid.set_column_spacing(8);
    Gtk::Label first_label("First page:"), last_label("Last page:");
    Gtk::SpinButton first_spin(Gtk::Adjustment::create(current_page_ + 1, 1, n, 1));
    Gtk::SpinButton last_spin(Gtk::Adjustment::create(current_page_ + 1, 1, n, 1));
    grid.attach(first_label, 0, 0);
    grid.attach(first_spin, 1, 0);
    grid.attach(last_label, 0, 1);
    grid.attach(last_spin, 1, 1);
    dlg.get_content_area()->pack_start(grid, Gtk::PACK_SHRINK, 8);
    dlg.show_all_children();
    if (dlg.run() != Gtk::RESPONSE_OK) return;
    dlg.hide();

    const int first = first_spin.get_value_as_int() - 1;
    const int last = last_spin.get_value_as_int() - 1;

    std::string dest;
    if (!choose_pdf(*this, "Save extracted pages", Gtk::FILE_CHOOSER_ACTION_SAVE, dest,
                    "extract.pdf")) {
        return;
    }

    std::string in;
    if (!snapshot(in)) return;
    std::string err;
    const bool ok = pdfedit::extract_range(in, dest, first, last, err);
    g_remove(in.c_str());
    if (!ok) show_error("Could not extract pages: " + err);
}

void PdfViewer::on_merge() {
    std::vector<std::string> inputs;
    if (!choose_pdfs(*this, "Select PDFs to merge (merged in the order listed)", inputs)) {
        return;
    }
    std::string dest;
    if (!choose_pdf(*this, "Save merged PDF", Gtk::FILE_CHOOSER_ACTION_SAVE, dest,
                    "merged.pdf")) {
        return;
    }

    std::string err;
    if (!pdfedit::merge(inputs, dest, err)) {
        show_error("Could not merge: " + err);
        return;
    }

    Gtk::MessageDialog dlg(*this, "Merged PDF saved. Open it now?", false,
                           Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);
    if (dlg.run() == Gtk::RESPONSE_YES) open_file(dest);
}
