# Makefile for EFPDF (Easy & Flexible PDF)
# GTKmm 3 + Poppler-GLib (render/annotate/forms) + QPDF (page operations)
#
# Every .cpp under src/ is built automatically, so adding a feature module
# needs no Makefile change.

CXX      := g++
PKGS     := gtkmm-3.0 poppler-glib libqpdf
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -MMD -MP -Isrc $(shell pkg-config --cflags $(PKGS))
LDFLAGS  := $(shell pkg-config --libs $(PKGS))

TARGET := efpdf
SRC    := $(shell find src -name '*.cpp' | sort)
OBJ    := $(SRC:src/%.cpp=build/%.o)
DEP    := $(OBJ:.o=.d)

PREFIX ?= /usr/local

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o $@ $(LDFLAGS)

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

-include $(DEP)

.PHONY: all clean run install uninstall
