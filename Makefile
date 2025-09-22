# -----------------------------------------------------------------------------
# Makefile for IMAGEWTY-Tool
# -----------------------------------------------------------------------------
# This Makefile compiles the IMAGEWTY-Tool project (a tool to analyze, unpack
# and repack Allwinner firmware images).
#
# Targets:
#   all      - Build the executable
#   clean    - Remove all build artifacts
#   install  - Install the binary to /usr/local/bin
#   cleanobj - Remove only object files
#   format   - Format all source/header files using clang-format
# -----------------------------------------------------------------------------

# Compiler and flags
CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -O2 -Iinclude
LDFLAGS = 

# Source files and objects
SRC = \
    src/main.c \
    src/img_header.c \
    src/img_extract.c \
    src/img_repack.c \
    src/checksum.c \
    src/config_file.c \
    src/print_info.c

OBJ = $(SRC:.c=.o)

# Binary name
BIN = imagewty-tool

# Default target: build the binary
all: $(BIN)

# Link the binary from object files
$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)

# Remove all object files and the binary
clean:
	rm -f $(OBJ) $(BIN)

# Install the binary to /usr/local/bin (requires sudo)
install: $(BIN)
	cp $(BIN) /usr/local/bin/$(BIN)

# Remove only object files
cleanobj:
	rm -f $(OBJ)

# Automatically format all source and header files using clang-format
format:
	clang-format -i -style=file $(SRC) include/*.h

.PHONY: all clean install cleanobj format
