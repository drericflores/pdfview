// EFPDF (Easy & Flexible PDF) - entry point.
#include <gtkmm.h>

#include <cstdio>
#include <cstring>

#include "app/main_window.h"
#include "core/version.h"

int main(int argc, char *argv[]) {
    if (argc > 1 && std::strcmp(argv[1], "--version") == 0) {
        std::printf("efpdf %s (%s)\n", efpdf::kRelease, efpdf::version());
        return 0;
    }
    g_message("efpdf %s (%s) starting", efpdf::kRelease, efpdf::version());
    auto app = Gtk::Application::create(argc, argv, "io.github.drericflores.efpdf");

    efpdf::MainWindow window;
    if (argc > 1) window.open_file(argv[1]);  // efpdf file.pdf

    // The file argument was handled above; keep it away from GApplication.
    return app->run(window, 1, argv);
}
