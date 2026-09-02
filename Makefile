CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -MMD -MP \
    -Ithird_party/imgui -Ithird_party/imgui-sfml -Isrc \
    $(shell pkg-config --cflags sfml-all)
LDFLAGS := $(shell pkg-config --libs sfml-all) -lGL

BUILD_DIR := build
BIN := bin/rps
BENCH_BIN := bin/benchmark

SRC := $(shell find src third_party -name '*.cpp')
OBJ := $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SRC))

BENCH_SRC := tools/benchmark.cpp src/ai/NeuralOpponent.cpp
BENCH_OBJ := $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(BENCH_SRC))

DEP := $(OBJ:.o=.d) $(BENCH_OBJ:.o=.d)

.PHONY: all clean run benchmark seed seed-eval windows linux-dist precompiled

all: $(BIN)

$(BIN): $(OBJ)
	@mkdir -p $(dir $@)
	$(CXX) $(OBJ) -o $@ $(LDFLAGS)

benchmark: $(BENCH_BIN)

$(BENCH_BIN): $(BENCH_OBJ)
	@mkdir -p $(dir $@)
	$(CXX) $(BENCH_OBJ) -o $@

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(DEP)

# Jawne przetrenowanie ziarna - zawsze przelicza od nowa.
# Uruchom po kazdej zmianie architektury sieci albo hiperparametrow.
seed: $(BENCH_BIN)
	@mkdir -p data
	./$(BENCH_BIN) --train-seed data/seed.txt

# Ziarno do UCZCIWEJ oceny: trenowane bez przeciwnikow, ktorymi potem mierzymy.
# Nie uzywane przez gre - tylko do porownan w benchmarku.
seed-eval: $(BENCH_BIN)
	@mkdir -p data
	./$(BENCH_BIN) --train-seed data/seed_eval.txt --holdout

# Automatyczne wygenerowanie ziarna, ale TYLKO jesli pliku jeszcze nie ma.
# $(BENCH_BIN) jest zaleznoscia order-only (znak |), wiec samo przebudowanie benchmarku
# nie wywoluje ponownego treningu - inaczej ziarno zmienialoby sie po kazdej rekompilacji.
data/seed.txt: | $(BENCH_BIN)
	@mkdir -p data
	./$(BENCH_BIN) --train-seed data/seed.txt

# --- dystrybucja ---
# Windows: cross-kompilacja statycznego .exe (szczegoly w tools/build-windows.sh)
windows: data/seed.txt
	./tools/build-windows.sh

# Linux: binarka + ziarno w jednym folderze
linux-dist: all data/seed.txt
	@mkdir -p precompiled/linux/data
	cp $(BIN) precompiled/linux/
	cp data/seed.txt precompiled/linux/data/
	@echo "Gotowe: precompiled/linux/"

precompiled: linux-dist windows

run: all data/seed.txt
	./$(BIN)

clean:
	rm -rf $(BUILD_DIR) $(BIN) $(BENCH_BIN)
