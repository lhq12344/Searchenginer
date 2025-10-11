# Makefile for SearchEngine
.SILENT:
.PHONY: all clean run dirs print-vars

ROOT_DIR := $(abspath .)
SRC_DIR  := src
INC_DIR  := include
BIN_DIR  := bin
BUILD_DIR:= build

TARGET   := $(BIN_DIR)/Search_Engine_Server

CXX      ?= g++
STD      ?= c++17
WARN     ?= -Wall -Wextra -Wpedantic
OPT      ?= -O2
DEBUG    ?= -g

# Prefer pkg-config
PKG_CFLAGS := $(shell pkg-config --cflags wfrest workflow openssl 2>/dev/null)
PKG_LIBS   := $(shell pkg-config --libs   wfrest workflow openssl 2>/dev/null)
FALLBACK_LIBS := -lwfrest -lworkflow -lssl -lcrypto -lpthread -lz

CPPFLAGS += -I$(INC_DIR) $(PKG_CFLAGS)
CXXFLAGS += -std=$(STD) $(WARN) $(OPT) $(DEBUG) -MMD -MP $(EXTRA_CXXFLAGS)
LDFLAGS  += $(EXTRA_LDFLAGS)
LDLIBS   += $(if $(PKG_LIBS),$(PKG_LIBS),$(FALLBACK_LIBS)) $(EXTRA_LIBS)

SOURCES  := $(wildcard $(SRC_DIR)/*.cpp)
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
	$(TARGET)

clean:
	@rm -rf $(BUILD_DIR) $(TARGET)
	@echo "Cleaned."

print-vars:
	@echo CXX=$(CXX)
	@echo CPPFLAGS=$(CPPFLAGS)
	@echo CXXFLAGS=$(CXXFLAGS)
	@echo LDFLAGS=$(LDFLAGS)
	@echo LDLIBS=$(LDLIBS)

-include $(DEPS)
