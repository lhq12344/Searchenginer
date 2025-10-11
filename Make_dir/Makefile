# Makefile for Make_dir (dictionary & index builder)
.SILENT:
.PHONY: all clean run dirs

ROOT_DIR := $(abspath ..)
THIS_DIR := $(abspath .)
SRC_DIR  := $(THIS_DIR)
INC_DIRS := $(THIS_DIR) $(ROOT_DIR)/include
BIN_DIR  := $(THIS_DIR)
BUILD_DIR:= $(THIS_DIR)/.build

TARGET   := $(BIN_DIR)/MakeDictTool

CXX      ?= g++
STD      ?= c++17
WARN     ?= -Wall -Wextra -Wpedantic
OPT      ?= -O2
DEBUG    ?= -g

CPPFLAGS := $(foreach d,$(INC_DIRS),-I$(d))
CXXFLAGS := -std=$(STD) $(WARN) $(OPT) $(DEBUG) -MMD -MP
LDFLAGS  :=
LDLIBS   := -lpthread

SOURCES  := $(SRC_DIR)/main.cpp $(SRC_DIR)/MakeDictAndIndex.cpp
OBJECTS  := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))
DEPS     := $(OBJECTS:.o=.d)

all: dirs $(TARGET)
	@echo "Built: $(TARGET)"

dirs:
	@mkdir -p $(BIN_DIR) $(BUILD_DIR)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS) $(LDLIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | dirs
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

run: all
	@mkdir -p $(THIS_DIR)/dict
	$(TARGET)

clean:
	@rm -rf $(BUILD_DIR) $(TARGET)
	@echo "Cleaned."

-include $(DEPS)
