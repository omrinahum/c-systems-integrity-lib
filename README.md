# C Systems Integrity Library

A low-level C library for Linux that automatically detects memory leaks, double-free errors, file descriptor leaks, and concurrency deadlocks in C/C++ applications **without recompilation**.

## Current Status: Phase 1 ✓

**Working Features:**
- ✅ malloc/free/calloc/realloc interception via LD_PRELOAD
- ✅ Memory leak detection
- ✅ Leak reporting at program exit
- ✅ Works on any dynamically-linked C/C++ program

**Coming Soon:**
- 🔄 Stack traces (show source code location of leaks)
- 🔄 Thread safety (proper multi-threaded support)
- 🔄 Double-free and invalid-free detection
- 🔄 File descriptor leak detection
- 🔄 Deadlock detection

## Quick Start

```bash
# Build the profiler
make

# Test it
make test

# Use on your program
LD_PRELOAD=./libprofiler.so ./your_program
```

## How It Works

The profiler uses **LD_PRELOAD** to intercept memory allocation functions before your program calls them:

```
Your Program calls malloc()
         ↓
Our profiler intercepts it
         ↓
We track the allocation
         ↓
We call the REAL malloc()
         ↓
Return pointer to your program
```

At program exit, we report any allocations that were never freed = **memory leaks**.

## Example Output

```
========== MEMORY LEAK REPORT ==========
[LEAK] 0x55555555a2a0: 1024 bytes (allocated at timestamp 1730678400)
[LEAK] 0x55555555a6b0: 512 bytes (allocated at timestamp 1730678400)

Summary: 2 leaks, 1536 bytes total
========================================
```

## Project Structure

```
c-systems-integrity-lib/
├── src/
│   ├── profiler.c           # LD_PRELOAD interception layer
│   └── alloc_tracker.c      # Hash table for tracking allocations
├── include/
│   └── profiler_internal.h  # Internal data structures
├── tests/
│   ├── test_simple_leak.c   # Test with intentional leaks
│   └── test_no_leak.c       # Test with proper cleanup
├── Makefile                 # Build system
├── BUILD_GUIDE.md          # Detailed build instructions
└── CMemoryLeaker.md        # Full project specification
```

## Design Principles

This codebase follows these principles:

1. **Simple before complex** - Start with basics, add features incrementally
2. **One responsibility per file** - Each module has a clear purpose
3. **Clean code** - Readable, maintainable, well-commented
4. **Scalable architecture** - Easy to add new features
5. **Documentation** - Explain the "why", not just the "what"

## Requirements

- Linux (Ubuntu/Debian) or WSL on Windows
- GCC or Clang
- GNU Make

