// Unit tests for core/page_ranges. Needs only a C++17 compiler: `make check`.
#include <cstdio>
#include <string>
#include <vector>

#include "core/page_ranges.h"

using efpdf::PageRange;

static int failures = 0;

#define CHECK(cond)                                                      \
    do {                                                                 \
        if (!(cond)) {                                                   \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            ++failures;                                                  \
        }                                                                \
    } while (0)

static bool same(const std::vector<PageRange> &a, const std::vector<PageRange> &b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i].first != b[i].first || a[i].last != b[i].last) return false;
    }
    return true;
}

static bool parses(const std::string &text, int pages, std::vector<PageRange> &out) {
    std::string err;
    return efpdf::parse_page_ranges(text, pages, out, err);
}

int main() {
    std::vector<PageRange> r;
    std::string err;

    // --- parse_page_ranges: valid input ------------------------------------
    CHECK(parses("1-3, 4, 7-9", 10, r));
    CHECK(same(r, {{0, 2}, {3, 3}, {6, 8}}));
    CHECK(parses("  2 ; 5 - 6 ,", 6, r));
    CHECK(same(r, {{1, 1}, {4, 5}}));
    CHECK(parses("1,,2", 2, r));
    CHECK(same(r, {{0, 0}, {1, 1}}));
    CHECK(parses("1-10", 10, r));
    CHECK(same(r, {{0, 9}}));

    // --- parse_page_ranges: invalid input ----------------------------------
    CHECK(!parses("", 10, r));
    CHECK(!parses(" , ", 10, r));
    CHECK(!parses("0", 10, r));
    CHECK(!parses("11", 10, r));
    CHECK(!parses("3-1", 10, r));
    CHECK(!parses("a-b", 10, r));
    CHECK(!parses("1-", 10, r));
    CHECK(!parses("-3", 10, r));
    CHECK(!parses("1-2-3", 10, r));
    CHECK(!parses("99999999999", 10, r));
    CHECK(!efpdf::parse_page_ranges("x", 10, r, err) || false);
    CHECK(!err.empty());

    // --- chunk_ranges -------------------------------------------------------
    CHECK(same(efpdf::chunk_ranges(5, 1), {{0, 0}, {1, 1}, {2, 2}, {3, 3}, {4, 4}}));
    CHECK(same(efpdf::chunk_ranges(5, 2), {{0, 1}, {2, 3}, {4, 4}}));
    CHECK(same(efpdf::chunk_ranges(4, 4), {{0, 3}}));
    CHECK(same(efpdf::chunk_ranges(3, 10), {{0, 2}}));
    CHECK(same(efpdf::chunk_ranges(3, 0), {{0, 0}, {1, 1}, {2, 2}}));
    CHECK(efpdf::chunk_ranges(0, 3).empty());

    // --- range_suffix -------------------------------------------------------
    CHECK(efpdf::range_suffix({3, 3}, 10) == "p004");
    CHECK(efpdf::range_suffix({0, 2}, 10) == "p001-003");
    CHECK(efpdf::range_suffix({0, 0}, 1200) == "p0001");
    CHECK(efpdf::range_suffix({998, 1199}, 1200) == "p0999-1200");

    if (failures == 0) std::printf("page_ranges: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
