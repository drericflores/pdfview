#include "features/search.h"

#include "core/document_model.h"
#include "ui/menu_builder.h"
#include "view/page_view.h"

namespace efpdf {

Search::Search(AppContext &ctx) : ctx_(ctx) {
    bar_.set_border_width(4);

    auto *label = Gtk::manage(new Gtk::Label("Find:"));
    auto *prev = Gtk::manage(new Gtk::Button("_Previous", true));
    auto *next = Gtk::manage(new Gtk::Button("_Next", true));
    auto *close = Gtk::manage(new Gtk::Button("Close"));
    entry_.set_width_chars(28);

    bar_.pack_start(*label, Gtk::PACK_SHRINK);
    bar_.pack_start(entry_, Gtk::PACK_SHRINK);
    bar_.pack_start(*prev, Gtk::PACK_SHRINK);
    bar_.pack_start(*next, Gtk::PACK_SHRINK);
    bar_.pack_start(match_case_, Gtk::PACK_SHRINK);
    bar_.pack_start(status_, Gtk::PACK_SHRINK);
    bar_.pack_end(*close, Gtk::PACK_SHRINK);
    ctx.top_area.pack_start(bar_, Gtk::PACK_SHRINK);

    // Show the children now, then hide the bar and exclude it from the
    // window's show_all(). (Calling show_all() on a no-show-all widget later
    // would do nothing, so show_bar() uses plain show().)
    bar_.show_all();
    bar_.set_no_show_all(true);
    bar_.hide();

    // Enter searches (or steps to the next hit); Shift+Enter goes backwards.
    entry_.signal_activate().connect([this] {
        if (hits_.empty() || std::string(entry_.get_text()) != last_query_) {
            run_search();
        } else {
            step(1);
        }
    });
    entry_.signal_key_press_event().connect(
        sigc::mem_fun(*this, &Search::on_entry_key), false);
    next->signal_clicked().connect([this] { step(1); });
    prev->signal_clicked().connect([this] { step(-1); });
    close->signal_clicked().connect(sigc::mem_fun(*this, &Search::hide_bar));
    match_case_.signal_toggled().connect(sigc::mem_fun(*this, &Search::run_search));

    ctx.menus.add_item("_Edit", "_Find...", sigc::mem_fun(*this, &Search::show_bar),
                       GDK_KEY_f, Gdk::CONTROL_MASK);
    ctx.menus.add_item("_Edit", "Find _Next", [this] { step(1); }, GDK_KEY_F3);
    ctx.menus.add_item("_Edit", "Find Pre_vious", [this] { step(-1); }, GDK_KEY_F3,
                       Gdk::SHIFT_MASK);
    ctx.menus.add_separator("_Edit");

    ctx.view.signal_paint_overlay.connect(sigc::mem_fun(*this, &Search::paint));
    ctx.model.signal_loaded.connect(sigc::mem_fun(*this, &Search::clear));
    ctx.model.signal_closed.connect(sigc::mem_fun(*this, &Search::clear));
}

void Search::show_bar() {
    if (!ctx_.model.is_open()) return;
    bar_.show();
    entry_.grab_focus();
    entry_.select_region(0, -1);
}

void Search::hide_bar() {
    bar_.hide();
    clear();
    ctx_.view.area().grab_focus();
}

bool Search::on_entry_key(GdkEventKey *event) {
    if (event->keyval == GDK_KEY_Escape) {
        hide_bar();
        return true;
    }
    if ((event->keyval == GDK_KEY_Return || event->keyval == GDK_KEY_KP_Enter) &&
        (event->state & GDK_SHIFT_MASK)) {
        step(-1);
        return true;
    }
    return false;
}

void Search::clear() {
    hits_.clear();
    current_ = -1;
    status_.set_text("");
    ctx_.view.area().queue_draw();
}

void Search::run_search() {
    hits_.clear();
    current_ = -1;
    const std::string query = entry_.get_text();
    last_query_ = query;
    DocumentModel &model = ctx_.model;
    if (query.empty() || !model.is_open()) {
        status_.set_text("");
        ctx_.view.area().queue_draw();
        return;
    }

    const PopplerFindFlags flags =
        match_case_.get_active() ? POPPLER_FIND_CASE_SENSITIVE : POPPLER_FIND_DEFAULT;
    for (int i = 0; i < model.page_count(); ++i) {
        PagePtr page = model.page(i);
        if (!page) continue;
        GList *matches = poppler_page_find_text_with_options(page.get(), query.c_str(), flags);
        for (GList *l = matches; l; l = l->next) {
            hits_.push_back({i, *static_cast<PopplerRectangle *>(l->data)});
        }
        g_list_free_full(matches, reinterpret_cast<GDestroyNotify>(poppler_rectangle_free));
    }

    if (hits_.empty()) {
        status_.set_text("No matches");
        ctx_.view.area().queue_draw();
        return;
    }

    // Start from the first hit on or after the page being viewed.
    int start = 0;
    for (size_t i = 0; i < hits_.size(); ++i) {
        if (hits_[i].page >= model.current_page()) {
            start = static_cast<int>(i);
            break;
        }
    }
    select_hit(start);
}

void Search::step(int direction) {
    if (hits_.empty()) {
        // F3 with no active search: open the bar, and search if text is waiting.
        show_bar();
        if (!entry_.get_text().empty()) run_search();
        return;
    }
    const int n = static_cast<int>(hits_.size());
    select_hit(((current_ + direction) % n + n) % n);
}

void Search::select_hit(int index) {
    current_ = index;
    const Hit hit = hits_[index];
    status_.set_text("Match " + std::to_string(index + 1) + " of " +
                     std::to_string(hits_.size()));

    ctx_.model.set_current_page(hit.page);
    ctx_.view.area().queue_draw();
    // Wait for the new page to be laid out before scrolling to the hit.
    Glib::signal_idle().connect_once(
        [this, hit] { ctx_.view.scroll_to_pdf_point(hit.rect.x1, hit.rect.y2); });
}

void Search::paint(const Cairo::RefPtr<Cairo::Context> &cr) {
    if (hits_.empty()) return;
    for (size_t i = 0; i < hits_.size(); ++i) {
        const Hit &hit = hits_[i];
        if (hit.page != ctx_.model.current_page()) continue;

        double x0, y0, x1, y1;
        ctx_.view.from_pdf(hit.rect.x1, hit.rect.y2, x0, y0);
        ctx_.view.from_pdf(hit.rect.x2, hit.rect.y1, x1, y1);
        cr->rectangle(x0, y0, x1 - x0, y1 - y0);
        if (static_cast<int>(i) == current_) {
            cr->set_source_rgba(1.0, 0.55, 0.0, 0.45);
        } else {
            cr->set_source_rgba(1.0, 0.9, 0.0, 0.40);
        }
        cr->fill();
    }
}

}  // namespace efpdf
