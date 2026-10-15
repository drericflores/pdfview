#include "features/thumbnails.h"

#include <algorithm>
#include <exception>
#include <string>
#include <vector>

#include "core/document_model.h"
#include "features/page_ops.h"

namespace efpdf {
namespace {
constexpr double kThumbHeight = 80.0;
// Drag-and-drop payload: the index of the page being dragged, as text.
constexpr const char *kDragTarget = "application/x-efpdf-page";
// The move runs a moment after the drop, so the rows that took part in the
// drag are not destroyed while GTK is still finishing it.
constexpr unsigned kMoveDelayMs = 30;
}

Thumbnails::Thumbnails(AppContext &ctx, PageOps &pages) : ctx_(ctx), pages_(pages) {
    scroller_.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    scroller_.add(list_);
    ctx.sidebar.pack_start(scroller_, Gtk::PACK_EXPAND_WIDGET);

    list_.signal_row_activated().connect([this](Gtk::ListBoxRow *row) {
        if (row) ctx_.model.set_current_page(row->get_index());
    });

    ctx.model.signal_loaded.connect(sigc::mem_fun(*this, &Thumbnails::rebuild));
    ctx.model.signal_closed.connect(sigc::mem_fun(*this, &Thumbnails::clear));
    ctx.model.signal_page_changed.connect(sigc::mem_fun(*this, &Thumbnails::select_current));
}

Thumbnails::~Thumbnails() { idle_.disconnect(); }

void Thumbnails::clear() {
    idle_.disconnect();
    for (auto *child : list_.get_children()) list_.remove(*child);
    next_ = 0;
}

// Thumbnails are rendered one per idle callback, so a large document opens
// immediately and the sidebar fills in behind it.
void Thumbnails::rebuild() {
    clear();
    if (!ctx_.model.is_open()) return;
    idle_ = Glib::signal_idle().connect(sigc::mem_fun(*this, &Thumbnails::add_next));
}

bool Thumbnails::add_next() {
    if (!ctx_.model.is_open() || next_ >= ctx_.model.page_count()) return false;

    auto *box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 2));
    Glib::RefPtr<Gdk::Pixbuf> pixbuf;
    PagePtr page = ctx_.model.page(next_);
    if (page) {
        double w = 0, h = 0;
        poppler_page_get_size(page.get(), &w, &h);
        const double s = h > 0 ? kThumbHeight / h : 1.0;
        const int iw = std::max(1, static_cast<int>(w * s));
        const int ih = static_cast<int>(kThumbHeight);

        auto surface = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, iw, ih);
        auto cr = Cairo::Context::create(surface);
        cr->set_source_rgb(1.0, 1.0, 1.0);
        cr->paint();
        cr->scale(s, s);
        poppler_page_render(page.get(), cr->cobj());

        GdkPixbuf *raw = gdk_pixbuf_get_from_surface(surface->cobj(), 0, 0, iw, ih);
        if (raw) {
            pixbuf = Glib::wrap(raw);
            box->pack_start(*Gtk::manage(new Gtk::Image(pixbuf)), Gtk::PACK_SHRINK);
        }
    }
    box->pack_start(*Gtk::manage(new Gtk::Label(std::to_string(next_ + 1))),
                    Gtk::PACK_SHRINK);

    // The event box is the drag handle; the row itself is the drop target.
    auto *handle = Gtk::manage(new Gtk::EventBox());
    handle->add(*box);
    auto *row = Gtk::manage(new Gtk::ListBoxRow());
    row->add(*handle);
    make_draggable(*row, *handle, pixbuf);
    list_.append(*row);
    row->show_all();
    if (next_ == ctx_.model.current_page()) list_.select_row(*row);

    ++next_;
    return true;
}

void Thumbnails::make_draggable(Gtk::ListBoxRow &row, Gtk::EventBox &handle,
                                const Glib::RefPtr<Gdk::Pixbuf> &icon) {
    const std::vector<Gtk::TargetEntry> targets{
        Gtk::TargetEntry(kDragTarget, Gtk::TARGET_SAME_APP)};

    handle.drag_source_set(targets, Gdk::BUTTON1_MASK, Gdk::ACTION_MOVE);
    if (icon) handle.drag_source_set_icon(icon);
    handle.signal_drag_data_get().connect(
        [&row](const Glib::RefPtr<Gdk::DragContext> &, Gtk::SelectionData &data, guint,
               guint) {
            const std::string index = std::to_string(row.get_index());
            data.set(data.get_target(), 8, reinterpret_cast<const guchar *>(index.c_str()),
                     static_cast<int>(index.size()));
        });

    row.drag_dest_set(targets, Gtk::DEST_DEFAULT_ALL, Gdk::ACTION_MOVE);
    row.signal_drag_data_received().connect(
        [this, &row](const Glib::RefPtr<Gdk::DragContext> &, int, int,
                     const Gtk::SelectionData &data, guint, guint) {
            int from = -1;
            try {
                from = std::stoi(data.get_data_as_string());
            } catch (const std::exception &) {
                return;  // not one of our thumbnails
            }
            const int to = row.get_index();
            if (from < 0 || to < 0 || from == to) return;
            Glib::signal_timeout().connect_once(
                [this, from, to] { pages_.move_page(from, to); }, kMoveDelayMs);
        });
}

void Thumbnails::select_current() {
    if (auto *row = list_.get_row_at_index(ctx_.model.current_page())) {
        list_.select_row(*row);
    }
}

}  // namespace efpdf
