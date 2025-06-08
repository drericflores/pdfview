#include <gtk/gtk.h>
#include "pdfviewer.h"

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    GtkWidget *window = create_pdf_viewer_window();
    gtk_widget_show_all(window);  // ✅ Required to make all widgets visible

    gtk_main();
    return 0;
}
