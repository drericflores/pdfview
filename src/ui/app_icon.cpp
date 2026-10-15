#include "ui/app_icon.h"

#include <string>
#include <vector>

namespace efpdf::ui {
namespace {

constexpr const char *kIconName = "efpdf";
constexpr const char *kIconFile = "efpdf2.png";

// Where icon files may live, best candidate first.
std::vector<std::string> candidate_files() {
    std::vector<std::string> files;
    gchar *exe = g_file_read_link("/proc/self/exe", nullptr);
    if (exe) {
        const std::string dir = Glib::path_get_dirname(exe);
        g_free(exe);
        files.push_back(Glib::build_filename(dir, "resources", kIconFile));
        files.push_back(Glib::build_filename(dir, "..", "resources", kIconFile));
        files.push_back(Glib::build_filename(dir, "..", "share", "efpdf", kIconFile));
    }
    files.push_back(Glib::build_filename("resources", kIconFile));
    return files;
}

std::string find_icon_file() {
    for (const std::string &file : candidate_files()) {
        if (Glib::file_test(file, Glib::FILE_TEST_IS_REGULAR)) return file;
    }
    return "";
}

}  // namespace

void install_app_icon() {
    Glib::set_application_name("EFPDF");

    const std::string file = find_icon_file();
    if (!file.empty()) {
        try {
            Gtk::Window::set_default_icon_from_file(file);
            return;
        } catch (const Glib::Error &e) {
            g_warning("efpdf: cannot load icon %s: %s", file.c_str(), e.what().c_str());
        }
    }
    // Installed copy, looked up through the icon theme.
    if (Gtk::IconTheme::get_default()->has_icon(kIconName)) {
        Gtk::Window::set_default_icon_name(kIconName);
    } else {
        g_message("efpdf: icon %s not found; using the default icon", kIconFile);
    }
}

Glib::RefPtr<Gdk::Pixbuf> app_icon_pixbuf(int size) {
    try {
        const std::string file = find_icon_file();
        if (!file.empty()) return Gdk::Pixbuf::create_from_file(file, size, size, true);
        if (Gtk::IconTheme::get_default()->has_icon(kIconName)) {
            return Gtk::IconTheme::get_default()->load_icon(kIconName, size);
        }
    } catch (const Glib::Error &) {
    }
    return Glib::RefPtr<Gdk::Pixbuf>();
}

}  // namespace efpdf::ui
