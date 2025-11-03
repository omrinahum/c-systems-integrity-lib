# Memory Profiler Makefile
# 
# Builds:
# 1. libprofiler.so - The shared library for LD_PRELOAD
# 2. Test programs - To verify the profiler works
#
# Usage:
#   make          - Build everything
#   make test     - Run tests
#   make clean    - Remove build artifacts

CC = gcc
CFLAGS = -Wall -Wextra -g -fPIC -I./include
LDFLAGS = -shared -ldl

# Output files
PROFILER_LIB = libprofiler.so
TEST_LEAK = tests/test_simple_leak
TEST_NO_LEAK = tests/test_no_leak

# Source files
PROFILER_SOURCES = src/malloc_intercept.c src/hash_table.c src/profiler.c
PROFILER_OBJECTS = $(PROFILER_SOURCES:.c=.o)

# Default target - build everything
all: $(PROFILER_LIB) $(TEST_LEAK) $(TEST_NO_LEAK)
	@echo ""
	@echo "Build complete!"
	@echo "==============="
	@echo "Profiler library: $(PROFILER_LIB)"
	@echo "Test programs: $(TEST_LEAK), $(TEST_NO_LEAK)"
	@echo ""
	@echo "To run tests:"
	@echo "  make test"

# Build the profiler shared library
$(PROFILER_LIB): $(PROFILER_OBJECTS)
	@echo "Linking profiler library..."
	$(CC) $(LDFLAGS) -o $@ $^
	@echo "Created $(PROFILER_LIB)"

# Compile profiler source files
%.o: %.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Build test programs
# Note: We compile with -g for debug symbols (not required for profiler to work)
#       We compile with -rdynamic to export symbols for better stack traces (later)
$(TEST_LEAK): tests/test_simple_leak.c
	@echo "Building test program: $@"
	$(CC) -g -rdynamic $< -o $@

$(TEST_NO_LEAK): tests/test_no_leak.c
	@echo "Building test program: $@"
	$(CC) -g -rdynamic $< -o $@

# Run tests with the profiler
test: all
	@echo ""
	@echo "=========================================="
	@echo "TEST 1: Simple Memory Leak Detection"
	@echo "=========================================="
	LD_PRELOAD=./$(PROFILER_LIB) ./$(TEST_LEAK)
	@echo ""
	@echo ""
	@echo "=========================================="
	@echo "TEST 2: No Leaks (Should Pass Clean)"
	@echo "=========================================="
	LD_PRELOAD=./$(PROFILER_LIB) ./$(TEST_NO_LEAK)
	@echo ""

# Clean build artifacts
clean:
	@echo "Cleaning build files..."
	rm -f $(PROFILER_OBJECTS)
	rm -f $(PROFILER_LIB)
	rm -f $(TEST_LEAK) $(TEST_NO_LEAK)
	@echo "Clean complete"

# Phony targets (not actual files)
.PHONY: all test clean
