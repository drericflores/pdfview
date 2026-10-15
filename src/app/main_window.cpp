#include "app/main_window.h"

#include "features/about.h"
#include "features/annotations.h"
#include "features/dark_mode.h"
#include "features/file_commands.h"
#include "features/forms.h"
#include "features/merge_split.h"
#include "features/navigation.h"
#include "features/page_ops.h"
#include "features/recent_files.h"
#include "features/search.h"
#include "features/thumbnails.h"
#include "features/tool_selector.h"
#include "features/zoom.h"
#include "core/util.h"

namespace efpdf {

MainWindow::MainWindow()
    : accel_(Gtk::AccelGroup::create()),
      menus_(menubar_, accel_),
      view_(model_),
      ctx_{*this, model_, view_, menus_, toolbar_, top_area_, sidebar_, settings_, tools_} {
    set_default_size(settings_.get_int("window", "width", 1100),
                     settings_.get_int("window", "height", 800));
    add_accel_group(accel_);
    add(root_);

    // Top-level menus, in display order. Features add their own items.
    menus_.declare({"_File", "_Edit", "_View", "_Help"});

    // --- features: one line each ------------------------------------------
    auto files = std::make_unique<FileCommands>(ctx_);
    files_ = files.get();
    features_.push_back(std::move(files));
    features_.push_back(std::make_unique<RecentFiles>(ctx_, *files_));
    features_.push_back(std::make_unique<Navigation>(ctx_));
    features_.push_back(std::make_unique<Zoom>(ctx_));
    features_.push_back(std::make_unique<Thumbnails>(ctx_));
    features_.push_back(std::make_unique<Search>(ctx_));
    features_.push_back(std::make_unique<Forms>(ctx_));        // registers the default tool
    features_.push_back(std::make_unique<Annotations>(ctx_));
    features_.push_back(std::make_unique<PageOps>(ctx_));
    features_.push_back(std::make_unique<MergeSplit>(ctx_, *files_));
    features_.push_back(std::make_unique<DarkMode>(ctx_));
    features_.push_back(std::make_unique<ToolSelector>(ctx_));  // after all tools exist
    features_.push_back(std::make_unique<About>(ctx_));

    // --- layout ------------------------------------------------------------
    toolbar_.set_border_width(4);
    sidebar_.set_size_request(140, -1);
    paned_.pack1(sidebar_, false, false);
    paned_.pack2(view_, true, true);
    root_.pack_start(menubar_, Gtk::PACK_SHRINK);
    root_.pack_start(toolbar_, Gtk::PACK_SHRINK);
    root_.pack_start(top_area_, Gtk::PACK_SHRINK);
    root_.pack_start(paned_, Gtk::PACK_EXPAND_WIDGET);

    // The title tracks the document: file name, page and unsaved-changes mark.
    model_.signal_loaded.connect(sigc::mem_fun(*this, &MainWindow::update_title));
    model_.signal_closed.connect(sigc::mem_fun(*this, &MainWindow::update_title));
    model_.signal_page_changed.connect(sigc::mem_fun(*this, &MainWindow::update_title));
    model_.signal_dirty_changed.connect(sigc::mem_fun(*this, &MainWindow::update_title));

    show_all_children();
    update_title();
}

MainWindow::~MainWindow() = default;

bool MainWindow::open_file(const std::string &path) { return files_->open_path(path); }

bool MainWindow::on_delete_event(GdkEventAny *event) {
    if (!files_->confirm_discard()) return true;  // keep the window open
    return Gtk::Window::on_delete_event(event);
}

void MainWindow::on_hide() {
    int width = 0, height = 0;
    get_size(width, height);
    if (width > 0 && height > 0) {
        settings_.set_int("window", "width", width);
        settings_.set_int("window", "height", height);
    }
    settings_.save();
    Gtk::Window::on_hide();
}

void MainWindow::update_title() {
    if (!model_.is_open()) {
        set_title("EFPDF");
        return;
    }
    set_title(std::string(model_.dirty() ? "*" : "") + util::base_name(model_.path()) +
              " - Page " + std::to_string(model_.current_page() + 1) + " of " +
              std::to_string(model_.page_count()) + " - EFPDF");
}

}  // namespace efpdf
