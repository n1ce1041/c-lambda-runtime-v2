# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g       # Debugging and extra warnings
LDFLAGS = -lcurl                # Link with libcurl

# Directories
SRC_DIR = src
BUILD_DIR = bin

# Main program target
TARGET = $(BUILD_DIR)/main_program

# Source and object files for main program
SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRC_FILES))

# Default target
all: $(TARGET)

# Build main program
$(TARGET): $(OBJ_FILES)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(OBJ_FILES) -o $@ $(LDFLAGS)

# Compile source files into object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -rf $(BUILD_DIR)

# Ensure phony targets are always executed
.PHONY: all clean
