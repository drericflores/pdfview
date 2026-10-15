#include "features/thumbnails.h"

#include <algorithm>
#include <string>

#include "core/document_model.h"

namespace efpdf {
namespace {
constexpr double kThumbHeight = 80.0;
}

Thumbnails::Thumbnails(AppContext &ctx) : ctx_(ctx) {
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
            Glib::RefPtr<Gdk::Pixbuf> pixbuf = Glib::wrap(raw);
            box->pack_start(*Gtk::manage(new Gtk::Image(pixbuf)), Gtk::PACK_SHRINK);
        }
    }
    box->pack_start(*Gtk::manage(new Gtk::Label(std::to_string(next_ + 1))),
                    Gtk::PACK_SHRINK);

    auto *row = Gtk::manage(new Gtk::ListBoxRow());
    row->add(*box);
    list_.append(*row);
    row->show_all();
    if (next_ == ctx_.model.current_page()) list_.select_row(*row);

    ++next_;
    return true;
}

void Thumbnails::select_current() {
    if (auto *row = list_.get_row_at_index(ctx_.model.current_page())) {
        list_.select_row(*row);
    }
}

}  // namespace efpdf
