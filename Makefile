# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude -I/opt/mqm/inc
LDFLAGS = -L/opt/mqm/lib64 -lmqm -Wl,-rpath,/opt/mqm/lib64 

# Directories
SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj
BIN_DIR = bin

# Target executable
TARGET = $(BIN_DIR)/diserr

# Source and object files
SRC_FILES = main.c cmdline.c mqutil.c

SOURCES = $(addprefix $(SRC_DIR)/, $(SRC_FILES))
OBJECTS = $(addprefix $(OBJ_DIR)/, $(SRC_FILES:.c=.o))

# Default target
all: $(TARGET)

# Link object files to create executable
$(TARGET): $(OBJECTS) | $(BIN_DIR)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

# Compile source files to object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create directories if they don't exist
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Clean build artifacts
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# Phony targets
.PHONY: all clean
