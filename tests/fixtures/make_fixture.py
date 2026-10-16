#!/usr/bin/python3
"""Regenerate the test fixtures (needs python3-cairo):

    chapters.pdf  5 A4 pages, each reading "Chapter N"
    lines.pdf     1 A4 page with a three-line paragraph at known positions
                  (text baselines 100, 130 and 160 points from the top)

usage: make_fixture.py [chapters.pdf [lines.pdf]]
"""
import sys
import cairo

out = sys.argv[1] if len(sys.argv) > 1 else "chapters.pdf"
surface = cairo.PDFSurface(out, 595, 842)
surface.set_metadata(cairo.PDF_METADATA_TITLE, "efpdf test fixture")
cr = cairo.Context(surface)
cr.select_font_face("DejaVu Sans", cairo.FONT_SLANT_NORMAL, cairo.FONT_WEIGHT_NORMAL)
for n in range(1, 6):
    cr.set_font_size(28)
    cr.move_to(72, 100)
    cr.show_text("Chapter %d" % n)
    cr.set_font_size(14)
    cr.move_to(72, 400)
    cr.show_text("NEEDLE on page %d" % n)
    surface.show_page()
surface.finish()

# ---- lines.pdf -------------------------------------------------------------
out2 = sys.argv[2] if len(sys.argv) > 2 else "lines.pdf"
surface = cairo.PDFSurface(out2, 595, 842)
cr = cairo.Context(surface)
cr.select_font_face("DejaVu Sans", cairo.FONT_SLANT_NORMAL, cairo.FONT_WEIGHT_NORMAL)
cr.set_font_size(14)
for y, text in ((100, "Alpha beta gamma"), (130, "delta epsilon zeta"), (160, "eta theta iota")):
    cr.move_to(72, y)
    cr.show_text(text)
surface.show_page()
surface.finish()
