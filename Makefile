# POSIX9 - POSIX Compatibility Layer for Mac OS 9
# Makefile for cross-compilation (host development/testing)

CC = gcc
AR = ar
CFLAGS = -Wall -Wextra -I./include -g -O2
ARFLAGS = rcs

# Source files
SRCS = src/posix9_file.c \
       src/posix9_dir.c \
       src/posix9_path.c

# Object files
OBJS = $(SRCS:.c=.o)

# Library name
LIB = libposix9.a

# Default target
all: info

# Note: This Makefile is for documentation/structure purposes.
# Actual compilation requires CodeWarrior or Retro68 targeting Mac OS 9.

info:
	@echo "POSIX9 - POSIX Compatibility Layer for Mac OS 9"
	@echo ""
	@echo "This library targets Mac OS 9 and requires:"
	@echo "  - CodeWarrior Pro 8+ for PPC, or"
	@echo "  - Retro68 cross-compiler"
	@echo ""
	@echo "Structure:"
	@echo "  include/posix9.h      - Main header"
	@echo "  include/posix9/*.h    - Type definitions"
	@echo "  src/posix9_file.c     - File I/O"
	@echo "  src/posix9_dir.c      - Directory operations"
	@echo "  src/posix9_path.c     - Path translation"
	@echo ""
	@echo "For CodeWarrior: Import into .mcp project"
	@echo "For Retro68: Use CMakeLists.txt"

# Clean
clean:
	rm -f $(OBJS) $(LIB)

# Phony targets
.PHONY: all clean info
