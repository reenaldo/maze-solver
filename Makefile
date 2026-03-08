CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinc -O2

CORE_SRC := src/graphe.c src/maze.c src/bfs.c src/json_export.c
MAIN_SRC := src/main.c

SRC := $(CORE_SRC) $(MAIN_SRC)

OBJ_DIR = build/obj
BIN_DIR = build/bin
MAZE_BIN = $(BIN_DIR)/maze

.PHONY: run all env clean test bfs_test maze_test check

# Default target: build the maze binary only (do NOT run the interactive app)
all: $(MAZE_BIN)

# Convenience target: build and run the maze inside a python venv (for visualizer)
run: all
	@# Create virtualenv and install requirements if missing
	@if [ ! -d .venv ]; then \
		echo "No .venv found — creating and installing requirements..."; \
		python3 -m venv .venv; \
		. .venv/bin/activate && pip install --upgrade pip setuptools wheel; \
		. .venv/bin/activate && pip install -r requirements.txt; \
	fi
	@echo "Running maze (inside .venv)..."
	@. .venv/bin/activate && ./$(MAZE_BIN)

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

$(OBJ_DIR)/%.o: src/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/bfs_tests.o: tests/bfs_tests.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/maze_tests.o: tests/maze_tests.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

CORE_OBJS = $(patsubst src/%.c,$(OBJ_DIR)/%.o,$(CORE_SRC))
MAIN_OBJS = $(patsubst src/%.c,$(OBJ_DIR)/%.o,$(MAIN_SRC))
OBJS = $(CORE_OBJS) $(MAIN_OBJS)

$(MAZE_BIN): $(OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@

# Test binaries
$(BIN_DIR)/bfs_tests: $(CORE_OBJS) $(OBJ_DIR)/bfs_tests.o | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@

$(BIN_DIR)/maze_tests: $(CORE_OBJS) $(OBJ_DIR)/maze_tests.o | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@

test: maze_test bfs_test 

maze_test: $(BIN_DIR)/maze_tests
	@echo "Running maze tests..."
	@./$(BIN_DIR)/maze_tests || true

bfs_test: $(BIN_DIR)/bfs_tests
	@echo "Running bfs tests..."
	@./$(BIN_DIR)/bfs_tests || true


check: test

env:
	@echo "Creating virtualenv .venv and installing requirements..."
	python3 -m venv .venv
	. .venv/bin/activate && pip install --upgrade pip setuptools wheel
	. .venv/bin/activate && pip install -r requirements.txt

clean:
	rm -rf build/ maze_bfs_steps.json .venv

# Run tests under Valgrind (if available). Useful for detecting leaks and invalid memory
.PHONY: valgrind bfs_valgrind maze_valgrind

valgrind: bfs_valgrind maze_valgrind

bfs_valgrind: $(BIN_DIR)/bfs_tests
	@command -v valgrind >/dev/null 2>&1 || { echo "Valgrind not found. Install valgrind (Homebrew or system) to run this target."; exit 1; }
	@echo "Running bfs_tests under Valgrind..."
	@valgrind --leak-check=full --show-leak-kinds=all --errors-for-leak-kinds=all --track-origins=yes ./$(BIN_DIR)/bfs_tests

maze_valgrind: $(BIN_DIR)/maze_tests
	@command -v valgrind >/dev/null 2>&1 || { echo "Valgrind not found. Install valgrind (Homebrew or system) to run this target."; exit 1; }
	@echo "Running maze_tests under Valgrind..."
	@valgrind --leak-check=full --show-leak-kinds=all --errors-for-leak-kinds=all --track-origins=yes ./$(BIN_DIR)/maze_tests

