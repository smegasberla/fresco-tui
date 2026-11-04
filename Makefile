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

TEST_DIR    := tests
TEST_BUILD  := $(BUILD_DIR)/tests
TEST_SRC    := $(wildcard $(TEST_DIR)/*.cpp)
TEST_BIN    := $(TEST_BUILD)/run
# everything except main.cpp: the test binary brings its own main
TEST_LIB    := $(filter-out src/main.cpp,$(SRC))
TEST_DEPS   := $(wildcard $(TEST_DIR)/*.hpp) $(wildcard src/include/*.hpp)

.PHONY: all build clean run test

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

# make test            run all tests
# make test ARGS=raw   run only tests whose name contains "raw"
test: $(TEST_BIN)
	./$(TEST_BIN) $(ARGS)

$(TEST_BIN): $(TEST_SRC) $(TEST_LIB) $(TEST_DEPS)
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) -I$(TEST_DIR) $(CXXFLAGS) $^ -o $@

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

-include $(DEP)
