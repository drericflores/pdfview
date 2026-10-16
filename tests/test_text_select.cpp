// test_text_select.cpp - the drag-to-text logic behind Select, Highlight and
// Underline, checked against tests/fixtures/lines.pdf:
//     "Alpha beta gamma"   baseline 100 pt from the top
//     "delta epsilon zeta" baseline 130
//     "eta theta iota"     baseline 160
// Run with `make check-text` (needs poppler-glib).
#include <poppler.h>

#include <cstdlib>
#include <iostream>
#include <string>

#include "core/text_select.h"

namespace ts = efpdf::text_select;

static int g_failures = 0, g_checks = 0;
#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        ++g_checks;                                                            \
        if (!(cond)) {                                                         \
            ++g_failures;                                                      \
            std::cerr << "FAIL: " << msg << "  [" #cond "]  (line " << __LINE__ \
                      << ")\n";                                                \
        }                                                                      \
    } while (0)

static bool contains(const std::string &hay, const char *needle) {
    return hay.find(needle) != std::string::npos;
}

int main() {
    gchar *abs = g_canonicalize_filename(FIXTURE_DIR "/lines.pdf", nullptr);
    gchar *uri = g_filename_to_uri(abs, nullptr, nullptr);
    GError *error = nullptr;
    PopplerDocument *doc = poppler_document_new_from_file(uri, nullptr, &error);
    g_free(abs);
    g_free(uri);
    if (!doc) {
        std::cerr << "cannot open fixture: " << (error ? error->message : "?") << "\n";
        return 2;
    }
    PopplerPage *page = poppler_document_get_page(doc, 0);

    // One whole line.
    {
        auto s = ts::select(page, 60, 92, 500, 102);
        const std::string t = ts::text(page, s);
        CHECK(!s.empty(), "a drag over text selects something");
        CHECK(contains(t, "Alpha beta gamma"), "whole first line, got '" + t + "'");
        CHECK(!contains(t, "delta"), "does not leak into the next line");
        CHECK(s.lines.size() == 1, "one line gives one rectangle");
        if (!s.lines.empty()) {
            const auto &r = s.lines[0];
            CHECK(r.y1 > 80 && r.y1 < 100 && r.y2 > 100 && r.y2 < 112,
                  "line rectangle sits on the first line (top-left coords)");
            CHECK(r.x1 < 80 && r.x2 > 150, "line rectangle spans the words");
        }
    }

    // Two lines, and the gap between them is not covered.
    {
        auto s = ts::select(page, 60, 92, 500, 132);
        const std::string t = ts::text(page, s);
        CHECK(contains(t, "Alpha beta gamma") && contains(t, "delta epsilon zeta"),
              "two lines selected, got '" + t + "'");
        CHECK(!contains(t, "theta iota"), "third line not selected");
        CHECK(s.lines.size() == 2, "two lines give two rectangles");
        if (s.lines.size() == 2) {
            CHECK(s.lines[0].y2 < s.lines[1].y1, "the gap between lines stays uncovered");
        }
    }

    // Direction does not matter.
    {
        const std::string forward = ts::text(page, ts::select(page, 60, 92, 500, 132));
        const std::string backward = ts::text(page, ts::select(page, 500, 132, 60, 92));
        CHECK(forward == backward, "reversed drag selects the same text");
        // Selection follows the text flow, like dragging in a text editor: a drag
        // from the bottom-left up to the top-right starts at the END of line 1 and
        // finishes at the START of line 2, so it covers neither line's words.
        const std::string up_right = ts::text(page, ts::select(page, 60, 132, 500, 92));
        CHECK(!contains(up_right, "Alpha") && !contains(up_right, "delta"),
              "bottom-left to top-right follows text flow, got '" + up_right + "'");
    }

    // Part of a line.
    {
        const std::string t = ts::text(page, ts::select(page, 72, 92, 100, 102));
        CHECK(!t.empty() && t.size() < 12 && std::string("Alpha beta gamma").find(t) == 0,
              "a short drag selects a prefix, got '" + t + "'");
    }

    // Blank space selects nothing (Poppler alone would snap to the nearest line).
    {
        auto below = ts::select(page, 60, 400, 300, 420);
        CHECK(below.empty() && ts::text(page, below).empty(), "drag below all text selects nothing");
        auto right_of_text = ts::select(page, 400, 92, 500, 102);
        CHECK(right_of_text.empty(), "drag right of a short line selects nothing");
        auto above = ts::select(page, 60, 10, 300, 30);
        CHECK(above.empty(), "drag in the top margin selects nothing");
    }

    // A tiny drag across one word still counts.
    {
        auto s = ts::select(page, 76, 96, 82, 98);
        CHECK(!s.empty(), "a drag a few points long over a letter selects it");
    }

    CHECK(ts::select(nullptr, 0, 0, 10, 10).empty(), "null page is safe");
    CHECK(ts::text(nullptr, ts::Selection{}).empty(), "text of nothing is empty");

    g_object_unref(page);
    g_object_unref(doc);
    std::cout << "text_select: " << (g_checks - g_failures) << "/" << g_checks << " checks passed\n";
    return g_failures ? 1 : 0;
}
