#include "core/settings.h"

#include <glib/gstdio.h>

namespace efpdf {

Settings::Settings() {
    const std::string dir = Glib::build_filename(Glib::get_user_config_dir(), "efpdf");
    path_ = Glib::build_filename(dir, "settings.ini");
    g_mkdir_with_parents(dir.c_str(), 0755);
    try {
        keyfile_.load_from_file(path_);
    } catch (const Glib::Error &) {
        // First run or unreadable file: start with defaults.
    }
}

int Settings::get_int(const char *group, const char *key, int fallback) const {
    try {
        if (keyfile_.has_key(group, key)) return keyfile_.get_integer(group, key);
    } catch (const Glib::Error &) {
    }
    return fallback;
}

double Settings::get_double(const char *group, const char *key,
                            double fallback) const {
    try {
        if (keyfile_.has_key(group, key)) return keyfile_.get_double(group, key);
    } catch (const Glib::Error &) {
    }
    return fallback;
}

bool Settings::get_bool(const char *group, const char *key, bool fallback) const {
    try {
        if (keyfile_.has_key(group, key)) return keyfile_.get_boolean(group, key);
    } catch (const Glib::Error &) {
    }
    return fallback;
}

std::string Settings::get_string(const char *group, const char *key,
                                 const std::string &fallback) const {
    try {
        if (keyfile_.has_key(group, key)) return keyfile_.get_string(group, key);
    } catch (const Glib::Error &) {
    }
    return fallback;
}

std::vector<std::string> Settings::get_list(const char *group, const char *key) const {
    std::vector<std::string> result;
    try {
        if (keyfile_.has_key(group, key)) {
            for (const auto &item : keyfile_.get_string_list(group, key)) {
                result.push_back(item);
            }
        }
    } catch (const Glib::Error &) {
    }
    return result;
}

void Settings::set_int(const char *group, const char *key, int value) {
    keyfile_.set_integer(group, key, value);
    signal_changed.emit(group, key);
}

void Settings::set_double(const char *group, const char *key, double value) {
    keyfile_.set_double(group, key, value);
    signal_changed.emit(group, key);
}

void Settings::set_bool(const char *group, const char *key, bool value) {
    keyfile_.set_boolean(group, key, value);
    signal_changed.emit(group, key);
}

void Settings::set_list(const char *group, const char *key,
                        const std::vector<std::string> &values) {
    std::vector<Glib::ustring> items(values.begin(), values.end());
    keyfile_.set_string_list(group, key, items);
    signal_changed.emit(group, key);
}

void Settings::set_string(const char *group, const char *key, const std::string &value) {
    keyfile_.set_string(group, key, value);
    signal_changed.emit(group, key);
}

void Settings::save() {
    try {
        keyfile_.save_to_file(path_);
    } catch (const Glib::Error &) {
    }
}

}  // namespace efpdf
