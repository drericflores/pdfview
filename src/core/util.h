// util.h - small file and URI helpers shared by all modules.
#pragma once

#include <string>

namespace efpdf::util {

// Absolute file:// URI for `path` (Poppler requires URIs). Empty on failure.
std::string to_uri(const std::string &path);

// Creates an empty temporary .pdf file and returns its path (empty on failure).
std::string make_temp_path();

// Deletes a file, ignoring errors and empty paths.
void remove_file(const std::string &path);

// The last component of a path.
std::string base_name(const std::string &path);

}  // namespace efpdf::util
