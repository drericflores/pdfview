# Makefile for EFPDF (Easy & Flexible PDF)
# GTKmm 3 + Poppler-GLib (render/annotate/forms) + QPDF (page operations)
#
# Every .cpp under src/ is built automatically, so adding a feature module
# needs no Makefile change.

CXX      := g++
PKGS     := gtkmm-3.0 poppler-glib libqpdf
# -Wno-cpp silences QPDF 11's "POINTERHOLDER_TRANSITION is not defined" #warning.
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Wno-cpp -MMD -MP -Isrc $(shell pkg-config --cflags $(PKGS))
LDFLAGS  := $(shell pkg-config --libs $(PKGS))

TARGET := efpdf
SRC    := $(shell find src -name '*.cpp' | sort)
OBJ    := $(SRC:src/%.cpp=build/%.o)
DEP    := $(OBJ:.o=.d)

# The git revision is compiled in so `efpdf --version` and Help > About show
# exactly which build is running. build/version.cpp is rewritten only when the
# revision changes.
VERSION := $(shell git describe --always --dirty 2>/dev/null || echo unknown)
VERSION_SRC := build/version.cpp
VERSION_OBJ := build/version.o

PREFIX ?= /usr/local

all: $(TARGET)

$(TARGET): $(OBJ) $(VERSION_OBJ)
	$(CXX) $(OBJ) $(VERSION_OBJ) -o $@ $(LDFLAGS)
	@echo "Built $(TARGET) $(VERSION)  ->  $(CURDIR)/$(TARGET)"

$(VERSION_SRC): FORCE
	@mkdir -p build
	@printf 'namespace efpdf { const char *version() { return "%s"; } }\n' "$(VERSION)" > $@.new
	@if cmp -s $@.new $@; then rm $@.new; else mv $@.new $@; fi

$(VERSION_OBJ): $(VERSION_SRC)
	$(CXX) $(CXXFLAGS) -c $< -o $@

FORCE:

build/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf build $(TARGET)

run: all
	./$(TARGET)

# --- installation ---------------------------------------------------------
# `sudo make install` installs system-wide; `make install PREFIX=$$HOME/.local`
# installs for the current user only (no sudo). Both add the program, the
# application icon and a launcher, so EFPDF appears in the GNOME menu.
ICON_SRC  := resources/efpdf2.png
DESKTOP   := io.github.drericflores.efpdf.desktop
# Pick the hicolor size directory that matches the PNG.
ICON_PX   := $(shell file $(ICON_SRC) 2>/dev/null | sed -n 's/.*PNG image data, \([0-9]*\) x.*/\1/p')
ICON_DIM  := $(shell px=$(or $(ICON_PX),256); \
               if [ $$px -ge 512 ]; then echo 512; elif [ $$px -ge 256 ]; then echo 256; \
               elif [ $$px -ge 128 ]; then echo 128; elif [ $$px -ge 64 ]; then echo 64; \
               else echo 48; fi)

install: all
	install -Dm755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)
	@if [ -f $(ICON_SRC) ]; then \
	  install -Dm644 $(ICON_SRC) $(DESTDIR)$(PREFIX)/share/icons/hicolor/$(ICON_DIM)x$(ICON_DIM)/apps/efpdf.png; \
	  install -Dm644 $(ICON_SRC) $(DESTDIR)$(PREFIX)/share/efpdf/efpdf2.png; \
	else echo "warning: $(ICON_SRC) not found; installing without an icon"; fi
	@mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	sed 's|@BINDIR@|$(PREFIX)/bin|' resources/efpdf.desktop.in > $(DESTDIR)$(PREFIX)/share/applications/$(DESKTOP)
	@chmod 644 $(DESTDIR)$(PREFIX)/share/applications/$(DESKTOP)
	@if [ -z "$(DESTDIR)" ]; then \
	  gtk-update-icon-cache -q -t -f $(PREFIX)/share/icons/hicolor 2>/dev/null || true; \
	  update-desktop-database -q $(PREFIX)/share/applications 2>/dev/null || true; \
	fi
	@echo "Installed EFPDF to $(PREFIX). If the menu entry or icon is missing, log out and back in."

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(TARGET)
	rm -f $(DESTDIR)$(PREFIX)/share/applications/$(DESKTOP)
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/*/apps/efpdf.png
	rm -rf $(DESTDIR)$(PREFIX)/share/efpdf
	@if [ -z "$(DESTDIR)" ]; then \
	  gtk-update-icon-cache -q -t -f $(PREFIX)/share/icons/hicolor 2>/dev/null || true; \
	  update-desktop-database -q $(PREFIX)/share/applications 2>/dev/null || true; \
	fi

-include $(DEP)

.PHONY: all clean run install uninstall FORCE
