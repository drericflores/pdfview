// EFPDF (Easy & Flexible PDF) - entry point.
#include <gtkmm.h>

#include "app/main_window.h"

int main(int argc, char *argv[]) {
    auto app = Gtk::Application::create(argc, argv, "io.github.drericflores.efpdf");

    efpdf::MainWindow window;
    if (argc > 1) window.open_file(argv[1]);  // efpdf file.pdf

    return app->run(window);
}
