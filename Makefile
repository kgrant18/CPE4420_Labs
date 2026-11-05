CXX = g++
#CXXFLAGS = -O0 -Wall -Wextra $(shell pkg-config --cflags opencv4)
#CXXFLAGS = -O3 -fno-tree-loop-vectorize -fno-tree-slp-vectorize -Wall -Wextra $(shell pkg-config --cflags opencv4)
CXXFLAGS = -O3 -pthread -Wall -Wextra $(shell pkg-config --cflags opencv4)
LIBS = $(shell pkg-config --libs opencv4)

TARGET = streamingSobel
SRC = streamingSobel.cpp
OBJ = streamingSobel.o 
BASELINE_TARGET = streamingSobelBaseline
BASELINE_OBJ = streamingSobelBaseline.o

all: $(TARGET) $(BASELINE_TARGET)

$(TARGET): $(OBJ)
	$(CXX) -o $(TARGET) $(OBJ) $(LIBS)

$(OBJ): $(SRC)
	$(CXX) $(CXXFLAGS) -c $(SRC) -o $(OBJ) 

$(BASELINE_TARGET): $(BASELINE_OBJ)
	$(CXX) -o $(BASELINE_TARGET) $(BASELINE_OBJ) $(LIBS)

$(BASELINE_OBJ): streamingSobelBaseline.cpp
	$(CXX) $(CXXFLAGS) -c streamingSobelBaseline.cpp -o $(BASELINE_OBJ)

clean:
	rm -f $(OBJ) $(TARGET) $(BASELINE_OBJ) $(BASELINE_TARGET)

.PHONY: all clean
