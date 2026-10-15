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

install: all
	install -Dm755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(TARGET)

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

.PHONY: all clean run install uninstall check FORCE
