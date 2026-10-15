// pdf_ops.h - structural PDF operations (rotate, delete, move, merge, extract).
//
// Poppler-GLib can render, annotate and fill forms, but it cannot restructure a
// document, so these operations use libqpdf. Every function reads `in`, writes
// a complete new file to `out`, and never modifies `in`. Page numbers are
// 0-based. On failure the functions return false and set `err`. `password` is
// the document password (empty for unencrypted files).
#pragma once

#include <string>
#include <vector>

#include "core/page_ranges.h"

namespace efpdf::pdf_ops {

bool rotate_page(const std::string &in, const std::string &out, int page,
                 int degrees, const std::string &password, std::string &err);

bool delete_page(const std::string &in, const std::string &out, int page,
                 const std::string &password, std::string &err);

// Moves the page at index `from` so that it ends up at index `to`.
bool move_page(const std::string &in, const std::string &out, int from, int to,
               const std::string &password, std::string &err);

// Concatenates the given (unencrypted) files, in order, into `out`.
bool merge(const std::vector<std::string> &inputs, const std::string &out,
           std::string &err);

// Copies pages first..last (inclusive) of `in` into a new file.
bool extract_range(const std::string &in, const std::string &out, int first,
                   int last, const std::string &password, std::string &err);

// Writes one new file per range (outputs[i] receives ranges[i]), reading `in`
// only once. All-or-nothing: every file is written under a temporary name
// first, and only when all succeed are they renamed into place, so a failure
// leaves existing files untouched. The outputs are unencrypted.
bool split(const std::string &in, const std::vector<PageRange> &ranges,
           const std::vector<std::string> &outputs, const std::string &password,
           std::string &err);

}  // namespace efpdf::pdf_ops
