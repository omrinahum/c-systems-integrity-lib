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
TEST_COMPLEX = tests/test_complex_leak
TEST_DOUBLE_FREE = tests/test_double_free
TEST_INVALID_FREE = tests/test_invalid_free

# Source files
PROFILER_SOURCES = src/malloc_intercept.c src/hash_table.c src/profiler.c
PROFILER_OBJECTS = $(PROFILER_SOURCES:.c=.o)

# Default target - build everything
all: $(PROFILER_LIB) $(TEST_LEAK) $(TEST_NO_LEAK) $(TEST_COMPLEX) $(TEST_DOUBLE_FREE) $(TEST_INVALID_FREE)
	@echo ""
	@echo "Build complete!"
	@echo "==============="
	@echo "Profiler library: $(PROFILER_LIB)"
	@echo "Test programs: $(TEST_LEAK), $(TEST_NO_LEAK), $(TEST_COMPLEX)"
	@echo "               $(TEST_DOUBLE_FREE), $(TEST_INVALID_FREE)"
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

$(TEST_COMPLEX): tests/test_complex_leak.c
	@echo "Building test program: $@"
	$(CC) -g -rdynamic $< -o $@

$(TEST_DOUBLE_FREE): tests/test_double_free.c
	@echo "Building test program: $@"
	$(CC) -g -rdynamic $< -o $@

$(TEST_INVALID_FREE): tests/test_invalid_free.c
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
	@echo ""
	@echo "=========================================="
	@echo "TEST 3: Complex Leak Detection"
	@echo "=========================================="
	LD_PRELOAD=./$(PROFILER_LIB) ./$(TEST_COMPLEX)
	@echo ""
	@echo ""
	@echo "=========================================="
	@echo "TEST 4: Double-Free Detection"
	@echo "=========================================="
	LD_PRELOAD=./$(PROFILER_LIB) ./$(TEST_DOUBLE_FREE)
	@echo ""
	@echo ""
	@echo "=========================================="
	@echo "TEST 5: Invalid-Free Detection"
	@echo "=========================================="
	LD_PRELOAD=./$(PROFILER_LIB) ./$(TEST_INVALID_FREE)
	@echo ""

# Clean build artifacts
clean:
	@echo "Cleaning build files..."
	rm -f $(PROFILER_OBJECTS)
	rm -f $(PROFILER_LIB)
	rm -f $(TEST_LEAK) $(TEST_NO_LEAK) $(TEST_COMPLEX) $(TEST_DOUBLE_FREE) $(TEST_INVALID_FREE)
	@echo "Clean complete"

# Phony targets (not actual files)
.PHONY: all test clean
