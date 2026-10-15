#include "core/util.h"

#include <glib.h>
#include <glib/gstdio.h>

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

}  // namespace efpdf::util
