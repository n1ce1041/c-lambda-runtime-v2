# Makefile for Lambda runtime project with libcurl

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g  # Debugging and extra warnings
LDFLAGS = -lcurl  # Link with libcurl

# Directories
SRC_DIR = src
BIN_DIR = bin

# Source files
SRC = $(SRC_DIR)/runtime.c $(SRC_DIR)/handler.c
OBJ = $(SRC:.c=.o)  # Object files derived from source files

# Output binary
TARGET = $(BIN_DIR)/runtime

# Default target (build the runtime binary)
all: $(TARGET)

# Linking the object files to create the binary
$(TARGET): $(OBJ)
	@mkdir -p $(BIN_DIR)  # Ensure the output directory exists
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)

# Compile the source files into object files
$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean object files and binaries
clean:
	rm -f $(SRC_DIR)/*.o $(BIN_DIR)/runtime

# Phony targets
.PHONY: all clean