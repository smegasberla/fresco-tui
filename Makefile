# fresco-tui — generic C++ build
# Sources are discovered automatically under src/ and the project root.

TARGET      := fresco-tui
BUILD_DIR   := build
BIN_DIR     := bin

SRC         := $(shell find src -name '*.cpp' 2>/dev/null; find . -maxdepth 1 -name '*.cpp' | sort)
OBJ         := $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SRC))
DEP         := $(OBJ:.o=.d)

CXX         ?= g++
CXXFLAGS    ?= -std=c++20 -O2 -g -Wall -Wextra -MMD -MP
CPPFLAGS    += -Isrc/include -Iinclude -I.
LDFLAGS     ?=
LDLIBS      ?=

BINARY      := $(BIN_DIR)/$(TARGET)

.PHONY: all build clean run

all: build

build: clean $(BINARY)

$(BINARY): $(OBJ)
	@mkdir -p $(@D)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

run: build
	./$(BINARY)

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

-include $(DEP)
