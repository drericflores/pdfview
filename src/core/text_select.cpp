#include "core/text_select.h"

#include <cairo.h>

#include <algorithm>
#include <cmath>

namespace efpdf::text_select {
namespace {

// The selection region comes back in whole device pixels, so ask for it at a
// high scale to keep sub-point precision (1/8 point).
constexpr double kRegionScale = 8.0;
// Two points closer than this vertically count as being on the same line.
constexpr double kSameLine = 3.0;
// How far outside the dragged box a text run may be and still count as hit.
constexpr double kSlop = 2.0;

}  // namespace

Selection select(PopplerPage *page, double x0, double y0, double x1, double y1) {
    Selection result;
    if (!page) return result;

    // Reading order: an earlier line first; on the same line, the left point first.
    const bool swap = (y1 < y0 - kSameLine) ||
                      (std::abs(y1 - y0) <= kSameLine && x1 < x0);
    if (swap) {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }
    result.rect = PopplerRectangle{x0, y0, x1, y1};

    PopplerRectangle sel = result.rect;  // Poppler wants a mutable pointer
    cairo_region_t *region =
        poppler_page_get_selected_region(page, kRegionScale, POPPLER_SELECTION_GLYPH, &sel);
    if (!region) return result;

    const double left = std::min(x0, x1) - kSlop, right = std::max(x0, x1) + kSlop;
    const double top = std::min(y0, y1) - kSlop, bottom = std::max(y0, y1) + kSlop;
    bool touches_drag = false;

    const int n = cairo_region_num_rectangles(region);
    for (int i = 0; i < n; ++i) {
        cairo_rectangle_int_t r;
        cairo_region_get_rectangle(region, i, &r);
        PopplerRectangle line{r.x / kRegionScale, r.y / kRegionScale,
                              (r.x + r.width) / kRegionScale, (r.y + r.height) / kRegionScale};
        result.lines.push_back(line);
        if (line.x2 >= left && line.x1 <= right && line.y2 >= top && line.y1 <= bottom) {
            touches_drag = true;
        }
    }
    cairo_region_destroy(region);

    if (!touches_drag) result.lines.clear();  // blank space: select nothing
    return result;
}

std::string text(PopplerPage *page, const Selection &selection) {
    if (!page || selection.empty()) return "";
    PopplerRectangle sel = selection.rect;
    gchar *raw = poppler_page_get_selected_text(page, POPPLER_SELECTION_GLYPH, &sel);
    std::string out = raw ? raw : "";
    g_free(raw);
    return out;
}

}  // namespace efpdf::text_select
