// page_ranges.h - page range parsing and splitting plans (no GTK, no QPDF).
//
// Kept free of every library so it can be unit-tested with a plain compiler
// (`make check`). Page numbers typed by the user are 1-based; PageRange is
// 0-based and inclusive on both ends, like the rest of the core.
#pragma once

#include <string>
#include <vector>

namespace efpdf {

struct PageRange {
    int first = 0;
    int last = 0;
};

// Parses text such as "1-3, 4, 7-9" into ranges. Pages are 1-based in the
// text. Ranges may be separated by commas or semicolons. Returns false and
// sets `err` (a sentence for the user) on bad input or numbers outside
// 1..page_count.
bool parse_page_ranges(const std::string &text, int page_count,
                       std::vector<PageRange> &out, std::string &err);

// Chunks of `per_file` pages covering the whole document; the last chunk may
// be shorter. per_file < 1 is treated as 1.
std::vector<PageRange> chunk_ranges(int page_count, int per_file);

// File-name part describing a range, zero-padded so files sort in page order:
// "p004" for a single page, "p001-003" for several (1-based in the name).
std::string range_suffix(const PageRange &range, int page_count);

}  // namespace efpdf
