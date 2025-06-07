# PdfView

This project is coded for educational purposes only. PdfView is a native Linux application written in C++ that provides a clean and functional interface for viewing PDF documents. Built using the GTKmm toolkit and Poppler-GLib rendering backend, PdfView focuses on reliability, responsiveness, and extensibility. Its current implementation provides users with essential features for loading and navigating PDF files through an intuitive graphical interface. PdfView is designed to be lightweight and portable, making it ideal for users who need a no-frills, platform-native viewer with a solid foundation for future expansion.

At the core of PdfView is a custom Gtk::Window subclass that manages the full lifecycle of a document—from file selection, rendering, and page navigation, to graceful closure. Upon launching the application, users are greeted with a main window sized appropriately for document display. The window features a menu bar that includes a "File" menu with options to open a PDF file, close the currently loaded document, or quit the application entirely. These actions are handled through signal connections that bind menu items to specific handler functions, providing a responsive and maintainable design.

Once a document is opened, PdfView uses the Poppler-GLib backend to parse and render individual pages onto a GTK drawing surface. The rendered content is displayed via a custom Gtk::DrawingArea, which is automatically updated whenever the user navigates to a new page or modifies the zoom level. Page rendering takes into account user-defined zoom scaling and dynamically updates the window title with the current page number for context. Navigation is handled through both a toolbar and menu options that allow the user to move forward and backward through the document one page at a time.

The user interface also includes a zoom control that increases or decreases the scaling factor applied to each page. This enables users to closely inspect technical documents or shrink large layouts to fit the viewing area. PdfView internally recalculates the page surface and triggers a redraw whenever the zoom level changes, ensuring consistent rendering without memory leaks or rendering artifacts.

Although still under active development, PdfView includes the framework for additional enhancements. A dark mode toggle has been stubbed into the menu system to support theme switching in future releases. Similarly, an "About" dialog is implemented under the Help menu, which displays the application’s name, author, technology stack, and development context. This information is dynamically presented using GTK’s built-in dialog framework to ensure consistency with modern Linux desktop environments.

The current release of PdfView was authored by Dr. Eric O. Flores in June 2025 and represents a collaborative effort that leverages both manual coding and AI-assisted development through ChatGPT. The codebase adheres to clean architectural principles, separating UI logic from rendering operations, and making use of signal-slot patterns to simplify event handling. PdfView is Wayland-compatible and avoids deprecated GTK2/Cairo rendering paths that often cause segmentation faults in newer display environments.

This application is compiled using a standard GNU Makefile and requires pkg-config, gtkmm-3.0, and poppler-glib development libraries to be installed. Once dependencies are satisfied, building the application is as simple as invoking make from the project directory. Executing the compiled binary (./pdfview) will launch the viewer and present the full GUI environment ready for interaction.

### Installation on Pop!_OS

To build PdfView on Pop!_OS (or any Ubuntu-based system), first ensure the required packages are installed:

```bash
sudo apt update
sudo apt install build-essential pkg-config libgtkmm-3.0-dev libpoppler-glib-dev
```

After installing the dependencies, navigate to the source directory and build the project:

```bash
make
./pdfview
```

If you wish to install PdfView system-wide, you can manually copy the binary to a location in your $PATH, such as /usr/local/bin.

### License

PdfView is released under the MIT License. This permissive license allows you to use, modify, and distribute the code for personal, educational, or commercial purposes, provided that the original license text and copyright
notice are included in all copies or substantial portions of the software.

---

MIT License

```
Copyright (c) 2025 Dr. Eric O. Flores

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the “Software”), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
```

---

### Contributing

Contributions to PdfView are welcome and encouraged. Developers interested in enhancing the functionality—such as implementing search, thumbnail navigation, printing support, or annotations—are invited to fork the repository and submit pull requests. All contributions should be modular, memory-safe, and follow existing coding patterns where possible.

Before submitting patches, please ensure the application builds cleanly with no warnings on a clean system. Bug reports, feature requests, and suggestions for UI improvements are also welcome and may be submitted via the repository’s issues page.

If you are unfamiliar with GTKmm or Poppler-GLib, you are encouraged to review their documentation to better align with PdfView’s architectural principles. Respect for clean signal handling, RAII memory management, and cross-environment compatibility (especially under Wayland) is highly appreciated in contributions.
