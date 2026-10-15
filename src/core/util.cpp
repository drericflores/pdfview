#include "core/util.h"

#include <glib.h>
#include <glib/gstdio.h>

#include <algorithm>
#include <cctype>

namespace efpdf::util {

std::string to_uri(const std::string &path) {
    gchar *absolute = g_canonicalize_filename(path.c_str(), nullptr);
    gchar *uri = g_filename_to_uri(absolute, nullptr, nullptr);
    std::string result = uri ? uri : "";
    g_free(absolute);
    g_free(uri);
    return result;
}

std::string make_temp_path() {
    gchar *name = nullptr;
    GError *error = nullptr;
    int fd = g_file_open_tmp("efpdf-XXXXXX.pdf", &name, &error);
    if (fd < 0) {
        if (error) g_error_free(error);
        return "";
    }
    g_close(fd, nullptr);
    std::string path = name;
    g_free(name);
    return path;
}

void remove_file(const std::string &path) {
    if (!path.empty()) g_remove(path.c_str());
}

std::string base_name(const std::string &path) {
    gchar *base = g_path_get_basename(path.c_str());
    std::string result = base ? base : "";
    g_free(base);
    return result;
}

std::string join_path(const std::string &dir, const std::string &name) {
    gchar *joined = g_build_filename(dir.c_str(), name.c_str(), nullptr);
    std::string result = joined ? joined : "";
    g_free(joined);
    return result;
}

bool file_exists(const std::string &path) {
    return g_file_test(path.c_str(), G_FILE_TEST_EXISTS);
}

std::string pdf_stem(const std::string &path) {
    std::string name = base_name(path);
    if (name.size() > 4) {
        std::string ext = name.substr(name.size() - 4);
        std::transform(ext.begin(), ext.end(), ext.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (ext == ".pdf") name.erase(name.size() - 4);
    }
    return name;
}

}  // namespace efpdf::util
