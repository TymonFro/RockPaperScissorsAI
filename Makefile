CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -MMD -MP \
    -Ithird_party/imgui -Ithird_party/imgui-sfml -Isrc \
    $(shell pkg-config --cflags sfml-all)
LDFLAGS := $(shell pkg-config --libs sfml-all) -lGL

BUILD_DIR := build
BIN := rps
BENCH_BIN := benchmark

SRC := $(shell find src third_party -name '*.cpp')
OBJ := $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SRC))

BENCH_SRC := tools/benchmark.cpp src/ai/NeuralOpponent.cpp
BENCH_OBJ := $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(BENCH_SRC))

DEP := $(OBJ:.o=.d) $(BENCH_OBJ:.o=.d)

.PHONY: all clean run

all: $(BIN)

$(BIN): $(OBJ)
	$(CXX) $(OBJ) -o $@ $(LDFLAGS)

$(BENCH_BIN): $(BENCH_OBJ)
	$(CXX) $(BENCH_OBJ) -o $@

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(DEP)

run: all
	./$(BIN)

clean:
	rm -rf $(BUILD_DIR) $(BIN) $(BENCH_BIN)
