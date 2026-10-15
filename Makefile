# Makefile for EFPDF (Easy & Flexible PDF)
# GTKmm 3 + Poppler-GLib (render/annotate/forms) + QPDF (page operations)

CXX      := g++
PKGS     := gtkmm-3.0 poppler-glib libqpdf
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -MMD -MP $(shell pkg-config --cflags $(PKGS))
LDFLAGS  := $(shell pkg-config --libs $(PKGS))

TARGET := efpdf
SRC    := main.cpp pdfviewer.cpp pdfedit.cpp
OBJ    := $(SRC:.cpp=.o)
DEP    := $(SRC:.cpp=.d)

PREFIX ?= /usr/local

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(DEP) $(TARGET)

run: all
	./$(TARGET)

install: all
	install -Dm755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(TARGET)

-include $(DEP)

.PHONY: all clean run install uninstall
