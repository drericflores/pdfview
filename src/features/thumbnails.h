// thumbnails.h - the page thumbnail sidebar.
#pragma once

#include "features/feature.h"

namespace efpdf {

class Thumbnails : public Feature {
public:
    explicit Thumbnails(AppContext &ctx);
    ~Thumbnails() override;

private:
    void rebuild();
    void clear();
    bool add_next();
    void select_current();

    AppContext &ctx_;
    Gtk::ScrolledWindow scroller_;
    Gtk::ListBox list_;
    sigc::connection idle_;
    int next_ = 0;
};

}  // namespace efpdf
