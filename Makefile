CC      := gcc
CFLAGS  := -Wall -Wextra -std=gnu11 -g -O0
SRC_DIR := src
BUILD_DIR := build
BIN     := shellforge

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

.PHONY: all clean run debug

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

run: $(BIN)
	./$(BIN)

debug: $(BIN)
	gdb ./$(BIN)

clean:
	rm -rf $(BUILD_DIR) $(BIN)
