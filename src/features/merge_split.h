// merge_split.h - Edit > Extract Pages..., Split into Files... and Merge PDFs...
#pragma once

#include "features/feature.h"

namespace efpdf {

class FileCommands;

class MergeSplit : public Feature {
public:
    MergeSplit(AppContext &ctx, FileCommands &files);

private:
    void extract_pages();
    void split_files();
    void merge();

    AppContext &ctx_;
    FileCommands &files_;
};

}  // namespace efpdf
