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
    gboolean dark_mode;
    GtkCssProvider *css_provider;
    GtkListBox *thumbnail_list;
} PdfViewerState;

static void render_pdf_page(PdfViewerState *state);

static void populate_thumbnails(PdfViewerState *state) {
    if (!state->document || !state->thumbnail_list) return;

    gtk_list_box_invalidate_filter(state->thumbnail_list);

    int total_pages = poppler_document_get_n_pages(state->document);
    for (int i = 0; i < total_pages; ++i) {
        PopplerPage *page = poppler_document_get_page(state->document, i);
        if (!page) continue;

        double width, height;
        poppler_page_get_size(page, &width, &height);
        double scale = 50.0 / height;
        width *= scale;
        height = 50.0;

        cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, (int)width, (int)height);
        cairo_t *cr = cairo_create(surface);
        cairo_scale(cr, scale, scale);
        poppler_page_render(page, cr);
        cairo_destroy(cr);
        g_object_unref(page);

        GdkPixbuf *pixbuf = gdk_pixbuf_get_from_surface(surface, 0, 0, (int)width, (int)height);
        cairo_surface_destroy(surface);

        GtkWidget *image = gtk_image_new_from_pixbuf(pixbuf);
        g_object_unref(pixbuf);

        GtkWidget *row = gtk_list_box_row_new();
        gtk_container_add(GTK_CONTAINER(row), image);
        gtk_list_box_insert(state->thumbnail_list, row, -1);
        g_object_set_data(G_OBJECT(row), "page-index", GINT_TO_POINTER(i));
        gtk_widget_show_all(row);
    }
    gtk_widget_show_all(GTK_WIDGET(state->thumbnail_list));
}

static void on_thumbnail_activated(GtkListBox *box, GtkListBoxRow *row, gpointer user_data) {
    PdfViewerState *state = (PdfViewerState *)user_data;
    int page = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "page-index"));
    state->current_page = page;
    render_pdf_page(state);
}

static void on_open_file(GtkButton *button, gpointer user_data) {
    PdfViewerState *state = (PdfViewerState *)user_data;
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Open PDF", NULL, GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL, "_Open", GTK_RESPONSE_ACCEPT, NULL);

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
            populate_thumbnails(state);
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
    state->dark_mode = gtk_toggle_button_get_active(button);

    const char *dark_css =
        "* { background-color: #1e1e1e; color: #ffffff; }\n"
        "GtkScrolledWindow { background-color: #1e1e1e; }\n"
        "GtkImage { background-color: #1e1e1e; }";

    const char *light_css =
        "* { background-color: #ffffff; color: #000000; }\n";

    if (!state->css_provider) {
        state->css_provider = gtk_css_provider_new();
    }

    gtk_css_provider_load_from_data(
        state->css_provider,
        state->dark_mode ? dark_css : light_css,
        -1, NULL);

    GtkStyleContext *context = gtk_widget_get_style_context(state->scrolled_window);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(state->css_provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
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
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_container_add(GTK_CONTAINER(window), hbox);

    GtkWidget *thumbnail_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(thumbnail_scroll, 100, -1);
    state->thumbnail_list = GTK_LIST_BOX(gtk_list_box_new());
    gtk_container_add(GTK_CONTAINER(thumbnail_scroll), GTK_WIDGET(state->thumbnail_list));
    gtk_box_pack_start(GTK_BOX(hbox), thumbnail_scroll, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);

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
    g_signal_connect(state->thumbnail_list, "row-activated", G_CALLBACK(on_thumbnail_activated), state);

    return window;
}

static void render_pdf_page(PdfViewerState *state) {
    if (!state->document) return;

    PopplerPage *page = poppler_document_get_page(state->document, state->current_page);
    if (!page) {
        g_printerr("Could not load page %d\n", state->current_page);
        return;
    }

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
    cairo_surface_destroy(surface);

    gtk_image_set_from_pixbuf(GTK_IMAGE(state->image), pixbuf);
    if (pixbuf) g_object_unref(pixbuf);
}
