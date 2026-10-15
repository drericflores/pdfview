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
	@$(MAKE) --no-print-directory check-install PREFIX=$(PREFIX)
	@echo "If the menu entry or icon is missing, log out and back in once."

# Reports what is installed under PREFIX (run automatically after install).
check-install:
	@echo "--- EFPDF install check (PREFIX=$(PREFIX)) ---"
	@for f in bin/efpdf share/applications/$(DESKTOP) share/efpdf/efpdf2.png; do \
	  if [ -e "$(PREFIX)/$$f" ]; then echo "  ok       $(PREFIX)/$$f"; else echo "  MISSING  $(PREFIX)/$$f"; fi; done
	@ls $(PREFIX)/share/icons/hicolor/*/apps/efpdf.png 2>/dev/null | sed 's/^/  ok       /' || true
	@if ! ls $(PREFIX)/share/icons/hicolor/*/apps/efpdf.png >/dev/null 2>&1; then \
	  echo "  MISSING  icon in $(PREFIX)/share/icons/hicolor (is resources/efpdf2.png present?)"; fi
	@if command -v desktop-file-validate >/dev/null 2>&1 && [ -e "$(PREFIX)/share/applications/$(DESKTOP)" ]; then \
	  desktop-file-validate $(PREFIX)/share/applications/$(DESKTOP) && echo "  ok       launcher file is valid"; fi
	@case ":$$XDG_DATA_DIRS:" in *":$(PREFIX)/share:"*) echo "  ok       $(PREFIX)/share is a desktop search folder";; \
	  *) if [ "$(PREFIX)" = "$$HOME/.local" ]; then echo "  ok       ~/.local/share is always searched"; \
	     else echo "  NOTE     $(PREFIX)/share is not in XDG_DATA_DIRS; the menu may not see it"; fi;; esac

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(TARGET)
	rm -f $(DESTDIR)$(PREFIX)/share/applications/$(DESKTOP)
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/*/apps/efpdf.png
	rm -rf $(DESTDIR)$(PREFIX)/share/efpdf
	@if [ -z "$(DESTDIR)" ]; then \
	  gtk-update-icon-cache -q -t -f $(PREFIX)/share/icons/hicolor 2>/dev/null || true; \
	  update-desktop-database -q $(PREFIX)/share/applications 2>/dev/null || true; \
	fi

# Unit tests for the library-free core code. `make check` needs only a C++17
# compiler and GLib; it does not need GTKmm, Poppler or QPDF.
GLIB_CFLAGS := $(shell pkg-config --cflags glib-2.0 2>/dev/null)
GLIB_LIBS   := $(shell pkg-config --libs glib-2.0 2>/dev/null)
TEST_FLAGS  := -std=c++17 -Wall -Wextra -Isrc

check: build/test_page_ranges build/test_util
	./build/test_page_ranges
	./build/test_util

build/test_page_ranges: tests/test_page_ranges.cpp src/core/page_ranges.cpp
	@mkdir -p build
	$(CXX) $(TEST_FLAGS) $^ -o $@

build/test_util: tests/test_util.cpp src/core/util.cpp
	@mkdir -p build
	$(CXX) $(TEST_FLAGS) $(GLIB_CFLAGS) $^ -o $@ $(GLIB_LIBS)

-include $(DEP)

.PHONY: all clean run install uninstall check check-install FORCE
