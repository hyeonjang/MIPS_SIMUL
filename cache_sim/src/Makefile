CXX=g++
CXXFLAGS=-g -std=c++11
SRCS=main.cpp
OBJS := $(SRCS:.cpp=.o)
TARGET=cache-sim

all: $(TARGET)
	$(CXX) -o $(TARGET) $(OBJS)

$(TARGET): $(OBJS) 
	$(CXX) -c $(SRCS) $(CXXFLAGS)

.PHONY=clean
.PHONY=run

clean:
	rm -f $(OBJS) $(TARGET)

run: $(TARGET)
	./$(TARGET) $(ARGS)
