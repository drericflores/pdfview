# EFPDF — Easy & Flexible PDF

EFPDF is a native Linux PDF viewer and editor written in C++17. It is built with
the GTKmm 3 toolkit, the Poppler-GLib rendering library and the QPDF library.
(The project was previously named *PdfView*; the GitHub repository is still
called `pdfview`.)

## Note:  I had some bugs and resorted to Claude Code to help me analyze and fix the bugs.

This project is coded for educational purposes.

## Features

**Viewing**

- Open PDFs from the File menu, the toolbar, or the command line (`efpdf file.pdf`)
- Password prompt for encrypted PDFs
- Page navigation (toolbar, menu, Page Up / Page Down), a page-jump box, and a
  thumbnail sidebar that fills in lazily
- Zoom in/out, actual size, fit width and fit page (Ctrl+mouse wheel also zooms)
- Text search across the whole document with highlighted matches (Ctrl+F)
- Recent files list, drag-and-drop to open, and remembered window size, zoom
  level and dark mode
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

EFPDF is modular: every feature lives in its own header and source file under
`src/features/`, and the window shell only assembles them. A feature plugs in
through a small `AppContext` (menus, toolbar, sidebar, mouse tools, settings,
document model) and never includes another feature's code.

```
src/
  main.cpp                 entry point
  app/main_window.*        window shell: owns shared objects, builds each feature
  core/                    no GTK widgets; reusable on its own
    document_model.*         open document, current page, unsaved flag, signals
    pdf_ops.*                rotate / delete / move / merge / extract (QPDF)
    history.*                undo/redo stack for document edits
    settings.*               ~/.config/efpdf/settings.ini
    util.*, gobject_ptr.h    temp files, URIs, RAII for Poppler objects
  ui/
    dialogs.*                shared dialogs (errors, file choosers, prompts)
    menu_builder.*           lets features add their own menu entries
    text_undo.*              undo for typing inside an editor widget
  view/
    page_view.*              renders the page; forwards mouse input to the active tool
    tool.h, tool_registry.h  the interface mouse tools implement
  features/                one module per feature
    file_commands.*          open / save / save as / close / quit / drag-and-drop
    recent_files.*           File > Open Recent
    navigation.*             previous/next, page-jump box, page shortcuts
    thumbnails.*             thumbnail sidebar
    zoom.*                   zoom, fit width, fit page
    dark_mode.*              dark mode
    search.*                 find bar and match highlighting
    annotations.*            highlight, underline, notes, right-click delete
    forms.*                  fill form fields
    edit_commands.*          Edit > undo, redo, cut, copy, paste
    page_ops.*               rotate, delete, move pages
    merge_split.*            merge PDFs, extract a page range
    tool_selector.*          toolbar drop-down for the active tool
    about.*                  Help > About
```

Poppler can render, annotate and fill forms but cannot restructure a document,
which is why page operations go through QPDF. Page operations work on a
temporary copy; the opened file changes only when you choose Save.

### Adding a feature

1. Create `src/features/my_feature.h` and `.cpp` with a class derived from
   `Feature` whose constructor takes an `AppContext &` and installs its menu
   items, toolbar widgets or tools.
2. Add one `features_.push_back(std::make_unique<MyFeature>(ctx_));` line in
   `src/app/main_window.cpp`.

The Makefile builds every `.cpp` under `src/` automatically.

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
| Ctrl+Home / Ctrl+End | First / last page |
| Ctrl+G | Go to page |
| Ctrl+F / F3 / Shift+F3 | Find / next match / previous match |
| Ctrl+1 / Ctrl+2 | Fit width / fit page |
| Ctrl+Plus / Ctrl+Minus / Ctrl+0 | Zoom in / zoom out / actual size |
| Ctrl+Z / Ctrl+Shift+Z (or Ctrl+Y) | Undo / redo |
| Ctrl+X / Ctrl+C / Ctrl+V | Cut / copy / paste (inside a form field or the search box) |
| Ctrl+L / Ctrl+R | Rotate page left / right |
| Ctrl+Alt+Up / Ctrl+Alt+Down | Move page up / down |
| Ctrl+D | Toggle dark mode |

## Using the editing tools

Pick a tool with the buttons on the right of the toolbar. A line of
instructions under the toolbar always tells you what the active tool does.

- **Fill forms:** click a form field and type directly into it. Enter keeps
  the value, Esc cancels. Check boxes and radio buttons toggle on click, and
  drop-downs open in place.
- **Undo and copy/paste while filling forms:** while typing in a field, Ctrl+Z
  and Ctrl+Shift+Z undo and redo your typing, and Ctrl+X / C / V use the
  clipboard. After pressing Enter, Ctrl+Z undoes the whole fill-in (text,
  drop-down choice, check box or radio button) and Ctrl+Shift+Z redoes it. The
  undo history starts fresh when a file is opened, closed or saved.
- **Highlight / Underline:** press, drag across the text, then release.
- **Note:** click where the note should go, then type its text.
- **Delete an annotation:** right-click it.

Edits stay in memory (the title shows `*`) until you press Ctrl+S.

If something does not behave, run `./efpdf file.pdf 2>&1 | tee run.log`; the
program logs each tool selection and annotation it creates.

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
