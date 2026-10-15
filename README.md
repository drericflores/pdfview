# EFPDF — Easy & Flexible PDF

EFPDF is a native Linux PDF viewer and editor written in C++17. It is built with
the GTKmm 3 toolkit, the Poppler-GLib rendering library and the QPDF library.
(The project was previously named *PdfView*; the GitHub repository is still
called `pdfview`.)

This project is coded for educational purposes.

## Features

**Viewing**

- Open PDFs from the File menu, the toolbar, or the command line (`efpdf file.pdf`)
- Password prompt for encrypted PDFs
- Page navigation (toolbar, menu, Page Up / Page Down) and a thumbnail sidebar
- Zoom in, zoom out and actual size
- Dark mode (inverts the page and switches the GTK theme to its dark variant)
- Window title shows the file name, the current page and an `*` for unsaved edits

**Editing**

- **Annotations:** highlight, underline and sticky notes; right-click an
  annotation to delete it
- **Forms:** fill text fields, check boxes, radio buttons and drop-down choices
- **Pages:** rotate, delete and move pages
- **Merge and split:** merge several PDFs into one; extract a page range to a new file
- **Saving:** Save writes back to the opened file (through a temporary file and
  an atomic rename, so a failed write cannot destroy the original); Save As
  writes a new file. Edits stay in memory until you save, and EFPDF asks before
  discarding them.

## Architecture

| File | Role |
| --- | --- |
| `main.cpp` | Creates the `Gtk::Application` and the main window |
| `pdfviewer.h` / `pdfviewer.cpp` | `PdfViewer`, a `Gtk::Window` subclass: menus, toolbar, thumbnails, `Gtk::DrawingArea` page view, annotations and form filling (Poppler) |
| `pdfedit.h` / `pdfedit.cpp` | Structural operations (rotate, delete, move, merge, extract) implemented with QPDF |

Poppler can render, annotate and fill forms but cannot restructure a document,
which is why page operations go through QPDF. Page operations work on a
temporary copy; the opened file changes only when you choose Save.

## Building

EFPDF needs `pkg-config` and the development packages for GTKmm 3, Poppler-GLib
and QPDF.

On Pop!_OS, Ubuntu and Debian:

```bash
sudo apt update
sudo apt install build-essential pkg-config libgtkmm-3.0-dev libpoppler-glib-dev libqpdf-dev
```

Then build and run:

```bash
make
./efpdf [file.pdf]
```

To install system-wide (default prefix `/usr/local`):

```bash
sudo make install
```

## Keyboard shortcuts

| Shortcut | Action |
| --- | --- |
| Ctrl+O / Ctrl+S / Ctrl+Shift+S | Open / Save / Save As |
| Ctrl+W / Ctrl+Q | Close document / Quit |
| Page Up / Page Down | Previous / next page |
| Ctrl+Plus / Ctrl+Minus / Ctrl+0 | Zoom in / zoom out / actual size |
| Ctrl+L / Ctrl+R | Rotate page left / right |
| Ctrl+Alt+Up / Ctrl+Alt+Down | Move page up / down |
| Ctrl+D | Toggle dark mode |

## Using the editing tools

Pick a tool from the drop-down on the right of the toolbar:

- **Select / Fill forms:** click a form field to edit it
- **Highlight / Underline:** drag a rectangle over the text
- **Add note:** click where the note should go, then type its text

## License

EFPDF is released under the MIT License. This permissive license allows you to use, modify, and distribute the code for personal, educational, or commercial purposes, provided that the original license text and copyright
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

## Contributing

Contributions are welcome. Ideas that would fit well: text search, fit-to-width
zoom, a recent-files list, undo/redo for edits, printing and packaging.
Please keep changes modular, use RAII for resource management, and make sure the
project builds cleanly with `-Wall -Wextra` before opening a pull request.
Bug reports and feature requests can be filed on the repository's issues page.
