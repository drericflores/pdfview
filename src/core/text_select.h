// text_select.h - turning a mouse drag into selected text.
//
// Used by the Select tool (to copy text) and by the highlight / underline
// tools (so a highlight hugs the text line by line instead of covering the
// whole dragged rectangle). It only needs Poppler, no GTK.
//
// All coordinates here are PDF points with the origin at the TOP-LEFT of the
// page, which is the convention Poppler's selection functions use. (Annotations
// and links use bottom-left; convert with `page_height - y`.)
#pragma once

#include <poppler.h>

#include <string>
#include <vector>

namespace efpdf::text_select {

struct Selection {
    PopplerRectangle rect{0, 0, 0, 0};   // Poppler's start/end description of the drag
    std::vector<PopplerRectangle> lines; // one rectangle per run of selected text
    bool empty() const { return lines.empty(); }
};

// Selects the text between two points of a drag. The drag may go in any
// direction: the start is always taken as the point that comes first in
// reading order. The result is empty when the drag does not touch any text
// (Poppler would otherwise snap a drag in blank space to the nearest line).
Selection select(PopplerPage *page, double x0, double y0, double x1, double y1);

// The selected text as UTF-8 (empty for an empty selection).
std::string text(PopplerPage *page, const Selection &selection);

}  // namespace efpdf::text_select
