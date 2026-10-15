// EFPDF (Easy & Flexible PDF) - entry point.
#include <gtkmm.h>

#include <cstdio>
#include <cstring>
#include <string>

#include "app/main_window.h"
#include "core/version.h"
#include "ui/app_icon.h"

int main(int argc, char *argv[]) {
    if (argc > 1 && std::strcmp(argv[1], "--version") == 0) {
        std::printf("efpdf %s (%s)\n", efpdf::kRelease, efpdf::version());
        return 0;
    }
    g_message("efpdf %s (%s) starting", efpdf::kRelease, efpdf::version());
    auto app = Gtk::Application::create(argc, argv, "io.github.drericflores.efpdf");

    efpdf::ui::install_app_icon();

    efpdf::MainWindow window;
    // efpdf [file.pdf]. Anything that looks like an option is ignored.
    std::string file;
    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] != '-') {
            file = argv[i];
            break;
        }
    }
    window.open_startup_file(file);

    // The file argument was handled above; keep it away from GApplication.
    return app->run(window, 1, argv);
}
