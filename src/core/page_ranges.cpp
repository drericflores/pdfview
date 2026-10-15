#include "core/page_ranges.h"

#include <algorithm>
#include <cctype>
#include <cstdio>

namespace efpdf {
namespace {

std::string trim(const std::string &s) {
    size_t b = 0, e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return s.substr(b, e - b);
}

// Parses a whole string of digits (at most 9, so it always fits an int).
bool parse_number(const std::string &s, int &value) {
    if (s.empty() || s.size() > 9) return false;
    int v = 0;
    for (char c : s) {
        if (!std::isdigit(static_cast<unsigned char>(c))) return false;
        v = v * 10 + (c - '0');
    }
    value = v;
    return true;
}

int digits(int n) {
    int d = 1;
    while (n >= 10) {
        n /= 10;
        ++d;
    }
    return d;
}

std::string padded(int n, int width) {
    std::string s = std::to_string(n);
    if (static_cast<int>(s.size()) < width) s.insert(0, width - s.size(), '0');
    return s;
}

}  // namespace

bool parse_page_ranges(const std::string &text, int page_count,
                       std::vector<PageRange> &out, std::string &err) {
    out.clear();

    std::vector<std::string> parts;
    std::string current;
    for (char c : text) {
        if (c == ',' || c == ';') {
            parts.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    parts.push_back(current);

    for (const std::string &raw : parts) {
        const std::string part = trim(raw);
        if (part.empty()) continue;  // tolerate "1-3,, 4" and a trailing comma

        int first = 0, last = 0;
        const size_t dash = part.find('-');
        if (dash == std::string::npos) {
            if (!parse_number(trim(part), first)) {
                err = "\"" + part + "\" is not a page number or a range like 3-5.";
                return false;
            }
            last = first;
        } else if (!parse_number(trim(part.substr(0, dash)), first) ||
                   !parse_number(trim(part.substr(dash + 1)), last)) {
            err = "\"" + part + "\" is not a page number or a range like 3-5.";
            return false;
        }

        if (first < 1 || last < 1 || first > page_count || last > page_count) {
            err = "\"" + part + "\" is outside the document (pages 1 to " +
                  std::to_string(page_count) + ").";
            return false;
        }
        if (first > last) {
            err = "\"" + part + "\": the first page must not be after the last page.";
            return false;
        }
        out.push_back({first - 1, last - 1});
    }

    if (out.empty()) {
        err = "Enter at least one page or range, for example 1-3, 4, 7-9.";
        return false;
    }
    return true;
}

std::vector<PageRange> chunk_ranges(int page_count, int per_file) {
    std::vector<PageRange> ranges;
    if (page_count < 1) return ranges;
    per_file = std::max(1, per_file);
    for (int first = 0; first < page_count; first += per_file) {
        ranges.push_back({first, std::min(first + per_file, page_count) - 1});
    }
    return ranges;
}

std::string range_suffix(const PageRange &range, int page_count) {
    const int width = std::max(3, digits(page_count));
    if (range.first == range.last) return "p" + padded(range.first + 1, width);
    return "p" + padded(range.first + 1, width) + "-" + padded(range.last + 1, width);
}

}  // namespace efpdf
