CXX = g++
CXXFLAGS = `pkg-config --cflags gtk+-3.0 poppler-glib` -O2 -std=c++17
LDFLAGS = `pkg-config --libs gtk+-3.0 poppler-glib`

SRC = main.cpp pdfviewer.cpp
OBJ = $(SRC:.cpp=.o)
TARGET = pdfview

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
