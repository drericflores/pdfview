// Unit tests for core/util (needs GLib only). Run with `make check`.
#include <cstdio>
#include <string>

#include "core/util.h"

static int failures = 0;

#define CHECK(cond)                                                      \
    do {                                                                 \
        if (!(cond)) {                                                   \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            ++failures;                                                  \
        }                                                                \
    } while (0)

int main() {
    using namespace efpdf::util;

    CHECK(pdf_stem("/a/b/Report.PDF") == "Report");
    CHECK(pdf_stem("report.pdf") == "report");
    CHECK(pdf_stem("/a/notes.txt") == "notes.txt");
    CHECK(pdf_stem("/a/.pdf") == ".pdf");        // nothing left to name the files after
    CHECK(pdf_stem("/a/my.file.pdf") == "my.file");

    CHECK(join_path("/tmp/out", "x.pdf") == "/tmp/out/x.pdf");
    CHECK(join_path("/tmp/out/", "x.pdf") == "/tmp/out/x.pdf");

    CHECK(file_exists("/"));
    CHECK(!file_exists("/definitely/not/here.pdf"));

    if (failures == 0) std::printf("util: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
