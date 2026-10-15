// page_ops.h - Edit menu: rotate, delete and move the current page.
//
// Poppler cannot restructure documents, so each operation snapshots the
// current state, runs a libqpdf operation on the copy, and loads the result.
// The opened file is untouched until the user saves.
#pragma once

#include "features/feature.h"

namespace efpdf {

class PageOps : public Feature {
public:
    explicit PageOps(AppContext &ctx);

private:
    void rotate(int degrees);
    void delete_current();
    void move_current(int delta);

    AppContext &ctx_;
};

}  // namespace efpdf
