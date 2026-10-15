// zoom.h - zoom in/out, actual size, fit width, fit page; remembers the level.
#pragma once

#include <string>

#include "features/feature.h"

namespace efpdf {

class Zoom : public Feature {
public:
    explicit Zoom(AppContext &ctx);

private:
    void apply_open_zoom();

    AppContext &ctx_;
    std::string last_path_;  // the zoom preference applies once per file
};

}  // namespace efpdf
