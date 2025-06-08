# Makefile for PdfView - GTK+3 + Poppler-based PDF viewer

# Compiler and flags
CXX := g++
CXXFLAGS := -std=c++17 -O2 `pkg-config --cflags gtk+-3.0 poppler-glib`
LDFLAGS := `pkg-config --libs gtk+-3.0 poppler-glib`

# Output binary
TARGET := pdfview

# Source files
SRC := main.cpp pdfviewer.cpp
OBJ := $(SRC:.cpp=.o)

# Default target
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up build artifacts
clean:
	rm -f $(OBJ) $(TARGET)

# Run the app
run: all
	./$(TARGET)

.PHONY: all clean run
