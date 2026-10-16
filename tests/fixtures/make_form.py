#!/usr/bin/env python3
"""Regenerate tests/fixtures/form.pdf: one page with a text field, check box,
radio group and drop-down (needs the reportlab package)."""
import sys
from reportlab.pdfgen import canvas
from reportlab.lib.pagesizes import A4

out = sys.argv[1] if len(sys.argv) > 1 else "form.pdf"
c = canvas.Canvas(out, pagesize=A4)
c.setFont("Helvetica", 14)
c.drawString(72, 780, "Name:")
c.drawString(72, 720, "I agree:")
c.drawString(72, 660, "Size:")
c.drawString(72, 600, "Country:")
f = c.acroForm
f.textfield(name="name", tooltip="Name", x=160, y=772, width=250, height=22, borderStyle="inset", fontSize=12)
f.checkbox(name="agree", tooltip="Agree", x=160, y=714, size=20, buttonStyle="check", checked=False)
f.radio(name="size", value="small", x=160, y=654, size=18, selected=False)
f.radio(name="size", value="large", x=220, y=654, size=18, selected=False)
f.choice(name="country", tooltip="Country", value="Chile", x=160, y=592, width=160, height=22,
         options=["Chile", "Peru", "Spain"], fieldFlags="combo", fontSize=12)
c.save()
