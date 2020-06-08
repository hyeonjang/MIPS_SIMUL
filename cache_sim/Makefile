CXX=g++
SRCS=main.cpp
OBJS := $(SRCS:.cpp=.o)
TARGET=cache-sim

all: $(TARGET)
	$(CXX) -o $(TARGET) $(OBJS)
$(TARGET): 
	$(CXX) -c $(SRCS)
clean:
	rm -f $(OBJS) $(TARGET)

run: $(TARGET)
	./$(TARGET)