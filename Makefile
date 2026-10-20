CXX = g++
#CXXFLAGS = -O0 -Wall -Wextra $(shell pkg-config --cflags opencv4)
#CXXFLAGS = -O3 -fno-tree-loop-vectorize -fno-tree-slp-vectorize -Wall -Wextra $(shell pkg-config --cflags opencv4)
CXXFLAGS = -O3 -Wall -Wextra $(shell pkg-config --cflags opencv4)
LIBS = $(shell pkg-config --libs opencv4)

TARGET = streamingSobel
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
