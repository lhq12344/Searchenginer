CXX := g++
CXXFLAGS := -std=c++17 -g -O0 -I.
LDFLAGS := -lstdc++fs

SRCS := main.cpp MakePage.cpp rcc.cpp ../src/tinyxml2.cpp
# place object files into build/ to keep workspace tidy
OBJDIR := build
OBJS := $(patsubst %.cpp,$(OBJDIR)/%.o,$(notdir $(SRCS)))
TARGET := makepage

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

$(OBJDIR)/%.o: %.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# special-case object from ../src
$(OBJDIR)/tinyxml2.o: ../src/tinyxml2.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR):
	@mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) $(TARGET)
