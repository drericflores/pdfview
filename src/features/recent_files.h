// recent_files.h - the File > Open Recent submenu, remembered between runs.
#pragma once

#include <string>
#include <vector>

#include "features/feature.h"

namespace efpdf {

class FileCommands;

class RecentFiles : public Feature {
public:
    RecentFiles(AppContext &ctx, FileCommands &files);

private:
    static constexpr size_t kMaxEntries = 10;

    void remember(const std::string &path);
    void rebuild_menu();

    AppContext &ctx_;
    FileCommands &files_;
    Gtk::Menu &submenu_;
    std::vector<std::string> paths_;
};

}  // namespace efpdf
