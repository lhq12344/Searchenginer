CXX := g++
CXXFLAGS := -std=c++17 -g -O2 -I.
LDFLAGS := -lstdc++fs

SRCS := main.cpp MakePage.cpp rcc.cpp ../src/tinyxml2.cpp
OBJS := $(SRCS:.cpp=.o)
TARGET := makepage

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
