CXX = g++
CXXFLAGS = -O3 -pthread -Wall -Wextra $(shell pkg-config --cflags opencv4)
LIBS = $(shell pkg-config --cflags --libs opencv4)

TARGET = sobel_optimized
SRC = streamingSobel.cpp
OBJ = streamingSobel.o 

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) -o $(TARGET) $(OBJ) $(LIBS)

$(OBJ): $(SRC)
	$(CXX) $(CXXFLAGS) -c $(SRC) -o $(OBJ) 

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
