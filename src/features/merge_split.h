// merge_split.h - Edit > Extract Pages... and Merge PDFs...
#pragma once

#include "features/feature.h"

namespace efpdf {

class FileCommands;

class MergeSplit : public Feature {
public:
    MergeSplit(AppContext &ctx, FileCommands &files);

private:
    void extract_pages();
    void merge();

    AppContext &ctx_;
    FileCommands &files_;
};

}  // namespace efpdf
