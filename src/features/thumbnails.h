// thumbnails.h - the page thumbnail sidebar. Click a thumbnail to go to that
// page; drag one onto another to move the page there (needs PageOps).
#pragma once

#include "features/feature.h"

namespace efpdf {

class PageOps;

class Thumbnails : public Feature {
public:
    Thumbnails(AppContext &ctx, PageOps &pages);
    ~Thumbnails() override;

private:
    void make_draggable(Gtk::ListBoxRow &row, Gtk::EventBox &handle,
                        const Glib::RefPtr<Gdk::Pixbuf> &icon);

    void rebuild();
    void clear();
    bool add_next();
    void select_current();

    AppContext &ctx_;
    PageOps &pages_;
    Gtk::ScrolledWindow scroller_;
    Gtk::ListBox list_;
    sigc::connection idle_;
    int next_ = 0;
};

}  // namespace efpdf
