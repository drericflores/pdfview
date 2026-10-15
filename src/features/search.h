// search.h - Edit > Find (Ctrl+F): search the whole document and highlight hits.
#pragma once

#include <poppler.h>

#include <string>
#include <vector>

#include "features/feature.h"

namespace efpdf {

class Search : public Feature {
public:
    explicit Search(AppContext &ctx);

private:
    struct Hit {
        int page;
        PopplerRectangle rect;  // PDF points, origin bottom-left
    };

    void show_bar();
    void hide_bar();
    void run_search();
    void step(int direction);
    void select_hit(int index);
    void clear();
    void paint(const Cairo::RefPtr<Cairo::Context> &cr);
    bool on_entry_key(GdkEventKey *event);

    AppContext &ctx_;
    Gtk::Box bar_{Gtk::ORIENTATION_HORIZONTAL, 6};
    Gtk::Entry entry_;
    Gtk::CheckButton match_case_{"Match case"};
    Gtk::Label status_;
    std::vector<Hit> hits_;
    std::string last_query_;
    int current_ = -1;
};

}  // namespace efpdf
