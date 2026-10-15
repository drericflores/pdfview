// about.h - Help > About.
#pragma once

#include "features/feature.h"

namespace efpdf {

class About : public Feature {
public:
    explicit About(AppContext &ctx);

private:
    void show();

    AppContext &ctx_;
};

}  // namespace efpdf
