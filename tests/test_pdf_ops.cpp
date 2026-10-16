// test_pdf_ops.cpp - checks every structural PDF operation against a fixture
// whose pages each read "Chapter N", so page order is verified by reading the
// result back with Poppler rather than just counting pages.
//
// Run with `make check-ops` (needs poppler-glib and libqpdf).
#include <poppler.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

#include "core/pdf_ops.h"

namespace ops = efpdf::pdf_ops;

static int g_failures = 0;
static int g_checks = 0;

#define CHECK(cond, msg)                                                      \
    do {                                                                      \
        ++g_checks;                                                           \
        if (!(cond)) {                                                        \
            ++g_failures;                                                     \
            std::cerr << "FAIL: " << msg << "  [" #cond "]  (line " << __LINE__ \
                      << ")\n";                                               \
        }                                                                     \
    } while (0)

static const std::string kFixture = FIXTURE_DIR "/chapters.pdf";
static const std::string kTmp = "build/ops_test_";

static PopplerDocument *open_doc(const std::string &path, const char *password = nullptr) {
    gchar *abs = g_canonicalize_filename(path.c_str(), nullptr);
    gchar *uri = g_filename_to_uri(abs, nullptr, nullptr);
    GError *error = nullptr;
    PopplerDocument *doc = poppler_document_new_from_file(uri, password, &error);
    if (error) g_error_free(error);
    g_free(uri);
    g_free(abs);
    return doc;
}

// Returns the chapter number printed on each page, in document order.
static std::vector<int> chapters(const std::string &path, const char *password = nullptr) {
    std::vector<int> out;
    PopplerDocument *doc = open_doc(path, password);
    if (!doc) return out;
    const int n = poppler_document_get_n_pages(doc);
    for (int i = 0; i < n; ++i) {
        PopplerPage *page = poppler_document_get_page(doc, i);
        gchar *text = poppler_page_get_text(page);
        int chapter = -1;
        if (const char *p = text ? std::strstr(text, "Chapter ") : nullptr) chapter = std::atoi(p + 8);
        out.push_back(chapter);
        g_free(text);
        g_object_unref(page);
    }
    g_object_unref(doc);
    return out;
}

static std::string slurp(const std::string &path) {
    std::ifstream f(path, std::ios::binary);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static std::string show(const std::vector<int> &v) {
    std::string s = "[";
    for (size_t i = 0; i < v.size(); ++i) s += (i ? "," : "") + std::to_string(v[i]);
    return s + "]";
}

int main() {
    std::string err;
    const std::vector<int> original = {1, 2, 3, 4, 5};
    const std::string before = slurp(kFixture);
    CHECK(chapters(kFixture) == original, "fixture should read 1..5, got " + show(chapters(kFixture)));

    // ---- delete ----------------------------------------------------------
    {
        const std::string out = kTmp + "del.pdf";
        CHECK(ops::delete_page(kFixture, out, 1, "", err), "delete page 2: " + err);
        CHECK(chapters(out) == (std::vector<int>{1, 3, 4, 5}), "delete order " + show(chapters(out)));
        CHECK(ops::delete_page(kFixture, out, 0, "", err), "delete first");
        CHECK(chapters(out) == (std::vector<int>{2, 3, 4, 5}), "delete first order");
        CHECK(ops::delete_page(kFixture, out, 4, "", err), "delete last");
        CHECK(chapters(out) == (std::vector<int>{1, 2, 3, 4}), "delete last order");
        CHECK(!ops::delete_page(kFixture, out, 5, "", err), "delete out of range must fail");
        CHECK(!ops::delete_page(kFixture, out, -1, "", err), "delete negative must fail");

        // A one-page document must refuse to lose its only page.
        const std::string one = kTmp + "one.pdf";
        CHECK(ops::extract_range(kFixture, one, 2, 2, "", err), "extract single page");
        CHECK(!ops::delete_page(one, out, 0, "", err) && !err.empty(), "deleting the only page must fail");
    }

    // ---- move: every (from, to) pair against a simple model -----------------
    {
        const std::string out = kTmp + "move.pdf";
        int bad = 0;
        for (int from = 0; from < 5; ++from) {
            for (int to = 0; to < 5; ++to) {
                std::vector<int> expect = original;
                int moving = expect[from];
                expect.erase(expect.begin() + from);
                expect.insert(expect.begin() + to, moving);
                const bool ok = ops::move_page(kFixture, out, from, to, "", err);
                const auto got = ok ? chapters(out) : std::vector<int>{};
                if (!ok || got != expect) {
                    ++bad;
                    std::cerr << "  move " << from << "->" << to << " expected " << show(expect)
                              << " got " << show(got) << (ok ? "" : " (" + err + ")") << "\n";
                }
            }
        }
        CHECK(bad == 0, "all 25 move combinations must match the model (" + std::to_string(bad) + " wrong)");
        CHECK(!ops::move_page(kFixture, out, 0, 9, "", err), "move out of range must fail");
    }

    // ---- rotate ----------------------------------------------------------
    {
        const std::string out = kTmp + "rot.pdf";
        auto size_of = [](const std::string &path, int index, double &w, double &h) {
            PopplerDocument *doc = open_doc(path);
            PopplerPage *page = poppler_document_get_page(doc, index);
            poppler_page_get_size(page, &w, &h);
            g_object_unref(page);
            g_object_unref(doc);
        };
        double w0, h0, w1, h1;
        size_of(kFixture, 2, w0, h0);
        CHECK(ops::rotate_page(kFixture, out, 2, 90, "", err), "rotate 90: " + err);
        size_of(out, 2, w1, h1);
        CHECK(w1 == h0 && h1 == w0, "rotated page swaps width and height");
        size_of(out, 1, w1, h1);
        CHECK(w1 == w0 && h1 == h0, "other pages are not rotated");
        CHECK(chapters(out) == original, "rotation keeps the order");
        // -90 is normalised, and rotating 90 then -90 returns to the original.
        const std::string out2 = kTmp + "rot2.pdf";
        CHECK(ops::rotate_page(out, out2, 2, -90, "", err), "rotate -90: " + err);
        size_of(out2, 2, w1, h1);
        CHECK(w1 == w0 && h1 == h0, "90 then -90 restores the page");
        CHECK(ops::rotate_page(kFixture, out, 0, 270, "", err), "rotate 270");
        CHECK(!ops::rotate_page(kFixture, out, 7, 90, "", err), "rotate out of range must fail");
    }

    // ---- extract -----------------------------------------------------------
    {
        const std::string out = kTmp + "ext.pdf";
        CHECK(ops::extract_range(kFixture, out, 1, 3, "", err), "extract 2-4: " + err);
        CHECK(chapters(out) == (std::vector<int>{2, 3, 4}), "extract order " + show(chapters(out)));
        CHECK(!ops::extract_range(kFixture, out, 3, 1, "", err), "reversed range must fail");
        CHECK(!ops::extract_range(kFixture, out, 0, 5, "", err), "range past the end must fail");
    }

    // ---- merge -------------------------------------------------------------
    {
        const std::string head = kTmp + "m_a.pdf", tail = kTmp + "m_b.pdf", out = kTmp + "merged.pdf";
        CHECK(ops::extract_range(kFixture, head, 0, 1, "", err), "prep head");
        CHECK(ops::extract_range(kFixture, tail, 2, 4, "", err), "prep tail");
        CHECK(ops::merge({tail, head}, out, err), "merge: " + err);
        CHECK(chapters(out) == (std::vector<int>{3, 4, 5, 1, 2}), "merge keeps input order " + show(chapters(out)));
        CHECK(ops::merge({kFixture, kFixture, kFixture}, out, err), "merge same file x3");
        CHECK(chapters(out).size() == 15, "merged page count");
        CHECK(!ops::merge({}, out, err), "merge of nothing must fail");
        CHECK(!ops::merge({kFixture, "/nonexistent/file.pdf"}, out, err), "merge with missing input must fail");
    }

    // ---- split -------------------------------------------------------------
    {
        const std::string a = kTmp + "s_a.pdf", b = kTmp + "s_b.pdf", c = kTmp + "s_c.pdf";
        std::remove(a.c_str()); std::remove(b.c_str()); std::remove(c.c_str());
        const std::vector<efpdf::PageRange> ranges = {{0, 1}, {2, 2}, {3, 4}};
        CHECK(ops::split(kFixture, ranges, {a, b, c}, "", err), "split: " + err);
        CHECK(chapters(a) == (std::vector<int>{1, 2}), "split part 1");
        CHECK(chapters(b) == (std::vector<int>{3}), "split part 2");
        CHECK(chapters(c) == (std::vector<int>{4, 5}), "split part 3");

        // All-or-nothing: an unwritable output must leave existing files alone.
        const std::string a_before = slurp(a);
        const std::vector<efpdf::PageRange> two = {{0, 0}, {1, 1}};
        CHECK(!ops::split(kFixture, two, {a, "/nonexistent-dir/x.pdf"}, "", err), "split to a bad path must fail");
        CHECK(slurp(a) == a_before, "failed split must not touch existing files");
        CHECK(!ops::split(kFixture, {{0, 9}}, {a}, "", err), "split range past the end must fail");
        CHECK(slurp(a) == a_before, "rejected split must not touch existing files");
    }

    // ---- encrypted input ---------------------------------------------------
    {
        const std::string out = kTmp + "enc_out.pdf";
        const std::string enc = kTmp + "locked.pdf";
        const std::string cmd = "qpdf --encrypt secret owner 256 -- " + kFixture + " " + enc;
        if (std::system(cmd.c_str()) == 0) {
            CHECK(!ops::delete_page(enc, out, 0, "", err) && !err.empty(), "no password must fail");
            CHECK(!ops::delete_page(enc, out, 0, "wrong", err), "wrong password must fail");
            CHECK(ops::delete_page(enc, out, 0, "secret", err), "right password works: " + err);
            // Editing keeps the protection (qpdf preserves encryption), so read it back with the password.
            CHECK(chapters(out).empty(), "edited output of a protected file stays protected");
            CHECK(chapters(out, "secret") == (std::vector<int>{2, 3, 4, 5}), "protected result order");
        } else {
            std::cout << "SKIP: qpdf command line tool not installed (encrypted-input tests)\n";
        }
    }

    // ---- inputs are never modified -------------------------------------------
    CHECK(slurp(kFixture) == before, "operations must never modify their input file");

    std::cout << "pdf_ops: " << (g_checks - g_failures) << "/" << g_checks << " checks passed\n";
    return g_failures ? 1 : 0;
}
