#include "pdfviewer.h"
#include <poppler.h>

typedef struct {
    PopplerDocument *document;
    int current_page;
    double scale;
    GtkWidget *image;
    GtkWidget *drawing_area;
    GtkWidget *scrolled_window;
    GdkRGBA background;
} PdfViewerState;

static void render_pdf_page(PdfViewerState *state) {
    if (!state->document) return;

    PopplerPage *page = poppler_document_get_page(state->document, state->current_page);
    if (!page) return;

    double width, height;
    poppler_page_get_size(page, &width, &height);
    width *= state->scale;
    height *= state->scale;

    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, (int)width, (int)height);
    cairo_t *cr = cairo_create(surface);

    cairo_scale(cr, state->scale, state->scale);
    poppler_page_render(page, cr);
    cairo_destroy(cr);
    g_object_unref(page);

    GdkPixbuf *pixbuf = gdk_pixbuf_get_from_surface(surface, 0, 0, (int)width, (int)height);
    gtk_image_set_from_pixbuf(GTK_IMAGE(state->image), pixbuf);
    g_object_unref(pixbuf);
    cairo_surface_destroy(surface);
}

static void on_open_file(GtkButton *button, gpointer user_data) {
    PdfViewerState *state = (PdfViewerState *)user_data;
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Open PDF",
        NULL,
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open", GTK_RESPONSE_ACCEPT,
        NULL);

    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(filter, "*.pdf");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        GError *error = NULL;

        if (state->document) g_object_unref(state->document);
        state->document = poppler_document_new_from_file(g_strdup_printf("file://%s", filename), NULL, &error);
        if (error) {
            g_printerr("Error loading PDF: %s\n", error->message);
            g_error_free(error);
        } else {
            state->current_page = 0;
            render_pdf_page(state);
        }
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
}

static void on_next_page(GtkButton *button, gpointer user_data) {
    PdfViewerState *state = (PdfViewerState *)user_data;
    if (state->document && state->current_page + 1 < poppler_document_get_n_pages(state->document)) {
        state->current_page++;
        render_pdf_page(state);
    }
}

static void on_prev_page(GtkButton *button, gpointer user_data) {
    PdfViewerState *state = (PdfViewerState *)user_data;
    if (state->document && state->current_page > 0) {
        state->current_page--;
        render_pdf_page(state);
    }
}

static void on_zoom_in(GtkButton *button, gpointer user_data) {
    PdfViewerState *state = (PdfViewerState *)user_data;
    state->scale *= 1.2;
    render_pdf_page(state);
}

static void on_zoom_out(GtkButton *button, gpointer user_data) {
    PdfViewerState *state = (PdfViewerState *)user_data;
    state->scale /= 1.2;
    render_pdf_page(state);
}

static void on_toggle_dark_mode(GtkToggleButton *button, gpointer user_data) {
    PdfViewerState *state = (PdfViewerState *)user_data;
    GtkStyleContext *context = gtk_widget_get_style_context(state->scrolled_window);
    if (gtk_toggle_button_get_active(button)) {
        gtk_style_context_add_class(context, "dark");
    } else {
        gtk_style_context_remove_class(context, "dark");
    }
}

GtkWidget* create_pdf_viewer_window() {
    PdfViewerState *state = g_new0(PdfViewerState, 1);
    state->scale = 1.0;
    state->current_page = 0;

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "PDF Viewer");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);

    GtkWidget *open_btn = gtk_button_new_with_label("Open");
    GtkWidget *prev_btn = gtk_button_new_with_label("Previous");
    GtkWidget *next_btn = gtk_button_new_with_label("Next");
    GtkWidget *zoom_in_btn = gtk_button_new_with_label("Zoom In");
    GtkWidget *zoom_out_btn = gtk_button_new_with_label("Zoom Out");
    GtkWidget *dark_toggle = gtk_check_button_new_with_label("Dark Mode");

    gtk_box_pack_start(GTK_BOX(toolbar), open_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), prev_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), next_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), zoom_in_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), zoom_out_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), dark_toggle, FALSE, FALSE, 0);

    state->image = gtk_image_new();
    state->scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(state->scrolled_window), state->image);
    gtk_box_pack_start(GTK_BOX(vbox), state->scrolled_window, TRUE, TRUE, 0);

    g_signal_connect(open_btn, "clicked", G_CALLBACK(on_open_file), state);
    g_signal_connect(prev_btn, "clicked", G_CALLBACK(on_prev_page), state);
    g_signal_connect(next_btn, "clicked", G_CALLBACK(on_next_page), state);
    g_signal_connect(zoom_in_btn, "clicked", G_CALLBACK(on_zoom_in), state);
    g_signal_connect(zoom_out_btn, "clicked", G_CALLBACK(on_zoom_out), state);
    g_signal_connect(dark_toggle, "toggled", G_CALLBACK(on_toggle_dark_mode), state);

    return window;
}
