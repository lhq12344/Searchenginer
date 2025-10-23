CXX := g++
# Allow overriding fastText install locations
FASTTEXT_INC ?= /usr/local/include
FASTTEXT_LIB ?= /usr/local/lib
# If you built fastText from source without install, set FASTTEXT_ROOT to the repo path.
# We'll also search its src/ for headers like aligned.h
FASTTEXT_ROOT ?= $(HOME)/fastText

CXXFLAGS := -std=c++17 -g -O0 -I. -I$(FASTTEXT_INC) -I$(FASTTEXT_INC)/fasttext -I$(FASTTEXT_ROOT) -I$(FASTTEXT_ROOT)/src
LDFLAGS := -L$(FASTTEXT_LIB) -lstdc++fs -lfasttext -pthread -Wl,-rpath,$(FASTTEXT_LIB)

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
