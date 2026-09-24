CXX = g++
CXXFLAGS = -O0 -Wall -Wextra $(shell pkg-config --cflags opencv4)
LIBS = $(shell pkg-config --libs opencv4)

TARGET = sobel_filter
SRC = sobel_filter.cpp
OBJ = sobel_filter.o 

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) -o $(TARGET) $(OBJ) $(LIBS)

$(OBJ): $(SRC)
	$(CXX) $(CXXFLAGS) -c $(SRC) -o $(OBJ) 

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
