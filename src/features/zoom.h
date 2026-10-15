// zoom.h - zoom in/out, actual size, fit width, fit page; remembers the level.
#pragma once

#include "features/feature.h"

namespace efpdf {

class Zoom : public Feature {
public:
    explicit Zoom(AppContext &ctx);

private:
    AppContext &ctx_;
};

}  // namespace efpdf
