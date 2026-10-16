// print.h - File > Print.
//
// Opens the desktop's print dialog and prints every page, including any
// annotations and form values that are still unsaved.
#pragma once

#include <gtkmm.h>

#include "features/feature.h"

namespace efpdf {

class Print : public Feature {
public:
    explicit Print(AppContext &ctx);

private:
    void print();

    AppContext &ctx_;
    // Kept between prints so the chosen printer and options are remembered.
    Glib::RefPtr<Gtk::PrintSettings> settings_;
};

}  // namespace efpdf
