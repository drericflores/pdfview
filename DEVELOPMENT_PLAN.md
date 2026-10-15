# EFPDF Development Plan

**EFPDF — Easy & Flexible PDF** (formerly PdfView)
Plan revised: 14 October 2026

## Goals

1. A fast, native Linux PDF viewer that works under X11 and Wayland.
2. Practical editing without leaving the app: annotate, fill forms, reorganize
   pages, merge and split.
3. A small, readable C++17 codebase that stays easy to build and contribute to.

## Decisions made

| Topic | Decision |
| --- | --- |
| Name | **EFPDF** (Easy & Flexible PDF); binary `efpdf`; GTK application id `io.github.drericflores.efpdf` |
| Toolkit | GTKmm 3 (matches what the README always described) |
| Rendering, annotations, forms | Poppler-GLib |
| Page operations, merge, split | libqpdf (Poppler cannot restructure documents) |
| Saving | In place, through a temp file and atomic rename; Save As also available |
| Editing scope | Annotations, form filling, page operations, merge/split |
| Structure | Modular: one header + source file per feature under `src/features/`; a thin window shell; GTK-free `core/` layer |
| Working location | Your computer at `/home/eric/development/cpp/efpdf/` (not linked to this chat; work is delivered through the GitHub repository `drericflores/efpdf`) |

## Phase 0 — Port and foundation (written, not yet compiled)

Status: **code written; never compiled** because the sandbox could not install
GTKmm, Poppler or QPDF development packages.

- [x] Add the missing `pdfviewer.h` (the original repo did not build from a fresh clone)
- [x] Port to GTKmm 3: `Gtk::Window` subclass, menu bar, toolbar, `Gtk::DrawingArea`
- [x] Fix the original leaks (hand-built URI string, error paths) with RAII wrappers
- [x] Fix the second-open bug (thumbnails were appended, never cleared)
- [x] Render thumbnails lazily, one per idle callback
- [x] Dark mode that works (invert the finished page; GTK dark theme for the chrome)
- [x] Annotations: highlight, underline, notes; right-click delete
- [x] Form filling: text, check box, radio, choice
- [x] Page operations: rotate, delete, move; merge; extract range
- [x] Save in place and Save As; unsaved-changes prompt on close, open and quit
- [x] Rename to EFPDF in code, Makefile, README, About dialog and temp files
- [x] README rewritten to match the code
- [x] Restructure into feature modules (`src/core`, `src/ui`, `src/view`, `src/features`, `src/app`)
- [x] Add search, page-jump box, fit width / fit page, Ctrl+wheel zoom, recent files, drag-and-drop open, and remembered settings

## Phase 1 — Verify on your machine (next)

Nothing below can be trusted until the code builds and runs.

1. Install dependencies: `sudo apt install build-essential pkg-config libgtkmm-3.0-dev libpoppler-glib-dev libqpdf-dev`
2. `make` — fix any compile errors and warnings (`-Wall -Wextra`)
3. Smoke test with a multi-page PDF: open, navigate, zoom, thumbnails, dark mode
4. Test every edit and then **reopen the saved file in a second viewer** to confirm
   the edit really persisted:
   highlight, underline, note, delete annotation, fill each form field type,
   rotate, delete page, move page, extract, merge
5. Test edge cases: encrypted PDF, one-page PDF (delete must refuse), second Open
   without restarting, closing with unsaved changes, Save over a read-only file

Known risks to check first:

- Text-search hit rectangles are assumed to be in PDF coordinates (origin bottom-left), like annotations; if highlights land vertically mirrored, flip the y axis in `Search::paint`
- Form-field and annotation hit-testing assumes Poppler returns PDF coordinates
  (origin bottom-left); if clicks land in the wrong place, flip the y axis in
  `edit_form_field_at` / `delete_annotation_at`
- Whether `poppler_page_render` draws newly added annotations without reopening
- QPDF API details (`QPDF` construction, `rotatePage`, remove-then-re-add pages
  in `move_page`) vary slightly between QPDF 10, 11 and 12
- Poppler saving a document that still has an open file handle after atomic rename

## Phase 2 — Finish the rename

- [x] Rename the GitHub repository `pdfview` → `efpdf` (GitHub redirects the old URL); About dialog website updated
- [ ] Rename your local folder to match
- [x] Source files and classes already use the new `efpdf` namespace and `MainWindow`
- [ ] Add a `.desktop` file and icon so EFPDF appears in the app launcher

## Phase 3 — Viewer features (from your earlier list)

- [x] Text search with result highlighting and next/previous (written, unverified)
- [x] Page-jump entry (written, unverified)
- [x] Fit to width / fit to page (written, unverified)
- [ ] Keyboard shortcuts beyond the current set (Home/End, `/` for search, `G` for go-to-page)
- [x] Recent files list (stored in `~/.config/efpdf/`) (written, unverified)
- [x] Remember window size, zoom and dark mode between runs (written, unverified)
- [ ] Continuous scrolling through pages (optional, larger change)

## Phase 4 — Editing polish

- [ ] Undo / redo for annotations, form edits and page operations
- [ ] Choose annotation colors; edit an existing note's text
- [x] Drag-and-drop page reordering in the thumbnail sidebar (written, unverified)
- [ ] Select multiple pages for delete, rotate and extract
- [x] Split into separate files, every N pages or by typed ranges (written, unverified)
- [ ] Insert pages from another PDF
- [ ] Optional: free-text and ink (freehand) annotations

## Look and feel (0.4.0)

- [x] One stylesheet with light and dark palettes (`features/appearance.cpp`), light menu bar with an accent line, icon toolbar, segmented tool picker, card-style thumbnails, framed page (written, unverified on screen)
- [ ] Review the look on a real screen and adjust colors, spacing and icons
- [ ] Optional: a Preferences choice for accent color

## Phase 5 — Quality and release

- [x] Unit tests for the library-free core (`make check`: page ranges, utilities)
- [ ] Automated tests for `pdf_ops` (page counts and order after each operation) using small fixture PDFs
- [ ] CI build on Ubuntu LTS with all three libraries
- [ ] Packaging: `.deb`, and optionally Flatpak or AppImage
- [ ] Tag `v1.0` once Phases 1–3 pass on a clean machine

## Out of scope (for now)

Editing existing body text, OCR, digital signatures, and non-Linux platforms.
Poppler and QPDF do not provide text-reflow editing; that would need a
different library and is a separate project.

## Open questions

- Do you want the repository renamed on GitHub, or kept as `pdfview` with only the app renamed?
- Should the port land on a branch for review (recommended) or directly on `main`?
- Which of the Phase 3 features matters most to you? That one goes first after Phase 1.
