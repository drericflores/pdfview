// pdfedit.h - structural PDF operations (page rotate/delete/move, merge, extract).
//
// Poppler-GLib can render, annotate and fill forms, but it cannot restructure a
// document, so these operations are implemented with libqpdf. Every function
// reads `in`, writes a complete new file to `out`, and never modifies `in`.
// All page numbers are 0-based. On failure they return false and set `err`.
#pragma once

#include <string>
#include <vector>

namespace pdfedit {

bool rotate_page(const std::string &in, const std::string &out, int page,
                 int degrees, std::string &err);

bool delete_page(const std::string &in, const std::string &out, int page,
                 std::string &err);

// Moves the page at index `from` so that it ends up at index `to`.
bool move_page(const std::string &in, const std::string &out, int from, int to,
               std::string &err);

// Concatenates the given files, in order, into `out`.
bool merge(const std::vector<std::string> &inputs, const std::string &out,
           std::string &err);

// Copies pages first..last (inclusive) of `in` into a new file.
bool extract_range(const std::string &in, const std::string &out, int first,
                   int last, std::string &err);

}  // namespace pdfedit
