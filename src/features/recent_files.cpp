#include "features/recent_files.h"

#include <algorithm>

#include "core/settings.h"
#include "core/util.h"
#include "features/file_commands.h"
#include "ui/dialogs.h"
#include "ui/menu_builder.h"

namespace efpdf {

RecentFiles::RecentFiles(AppContext &ctx, FileCommands &files)
    : ctx_(ctx),
      files_(files),
      // Position 1 puts the submenu right after "Open...".
      submenu_(ctx.menus.insert_submenu("_File", "Open _Recent", 1)) {
    paths_ = ctx.settings.get_list("recent", "files");
    if (paths_.size() > kMaxEntries) paths_.resize(kMaxEntries);

    files_.signal_file_opened.connect(sigc::mem_fun(*this, &RecentFiles::remember));
    rebuild_menu();
}

void RecentFiles::remember(const std::string &path) {
    paths_.erase(std::remove(paths_.begin(), paths_.end(), path), paths_.end());
    paths_.insert(paths_.begin(), path);
    if (paths_.size() > kMaxEntries) paths_.resize(kMaxEntries);

    ctx_.settings.set_list("recent", "files", paths_);
    ctx_.settings.save();
    rebuild_menu();
}

void RecentFiles::rebuild_menu() {
    for (auto *child : submenu_.get_children()) submenu_.remove(*child);

    if (paths_.empty()) {
        auto *empty = Gtk::manage(new Gtk::MenuItem("(none)"));
        empty->set_sensitive(false);
        submenu_.append(*empty);
    }

    for (const std::string &path : paths_) {
        auto *item = Gtk::manage(new Gtk::MenuItem(util::base_name(path)));
        item->set_tooltip_text(path);
        item->signal_activate().connect([this, path] {
            if (!Glib::file_test(path, Glib::FILE_TEST_EXISTS)) {
                ui::show_error(ctx_.window, "That file no longer exists:\n" + path);
                paths_.erase(std::remove(paths_.begin(), paths_.end(), path),
                             paths_.end());
                ctx_.settings.set_list("recent", "files", paths_);
                ctx_.settings.save();
                rebuild_menu();
                return;
            }
            files_.open_path(path);
        });
        submenu_.append(*item);
    }

    if (!paths_.empty()) {
        submenu_.append(*Gtk::manage(new Gtk::SeparatorMenuItem()));
        auto *clear = Gtk::manage(new Gtk::MenuItem("Clear list"));
        clear->signal_activate().connect([this] {
            paths_.clear();
            ctx_.settings.set_list("recent", "files", paths_);
            ctx_.settings.save();
            rebuild_menu();
        });
        submenu_.append(*clear);
    }
    submenu_.show_all();
}

}  // namespace efpdf
