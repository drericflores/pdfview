// settings.h - persistent user preferences (~/.config/efpdf/settings.ini).
#pragma once

#include <glibmm.h>
#include <sigc++/sigc++.h>

#include <string>
#include <vector>

namespace efpdf {

class Settings {
public:
    Settings();

    int get_int(const char *group, const char *key, int fallback) const;
    double get_double(const char *group, const char *key, double fallback) const;
    bool get_bool(const char *group, const char *key, bool fallback) const;
    std::string get_string(const char *group, const char *key,
                           const std::string &fallback) const;
    std::vector<std::string> get_list(const char *group, const char *key) const;

    void set_int(const char *group, const char *key, int value);
    void set_double(const char *group, const char *key, double value);
    void set_bool(const char *group, const char *key, bool value);
    void set_string(const char *group, const char *key, const std::string &value);
    void set_list(const char *group, const char *key,
                  const std::vector<std::string> &values);

    // Emitted after any value changes, with the group and key. Features use it
    // to react to Preferences immediately.
    sigc::signal<void, const std::string &, const std::string &> signal_changed;

    // Writes the file; failures are ignored because preferences are optional.
    void save();

private:
    Glib::KeyFile keyfile_;
    std::string path_;
};

}  // namespace efpdf
