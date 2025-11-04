# C Systems Integrity Library

A low-level C library for Linux that automatically detects memory leaks, double-free errors, file descriptor leaks, and concurrency deadlocks in C/C++ applications **without recompilation**.

## Current Status: Phase 6 ✓

**Working Features:**
- ✅ malloc/free/calloc/realloc interception via LD_PRELOAD
- ✅ Memory leak detection with O(1) hash table (uthash)
- ✅ Stack trace capture showing allocation sites
- ✅ Thread-safe operation with pthread mutexes
- ✅ False positive filtering (libc infrastructure detection)
- ✅ Double-free detection
- ✅ Invalid-free detection (stack vars, random addresses, etc.)
- ✅ Clean, minimal output format
- ✅ Comprehensive test suite (5 tests)

**Coming Soon:**
- 🔄 Symbol resolution (addr2line for readable stack traces)
- 🔄 File descriptor leak detection
- 🔄 Deadlock detection
- 🔄 JSON/HTML report generation

## Quick Start

```bash
# Build the profiler
make

# Test it
make test

# Use on your program (with stack traces - default)
LD_PRELOAD=./libprofiler.so ./your_program

# Disable stack traces for cleaner output
PROFILER_STACK_TRACES=0 LD_PRELOAD=./libprofiler.so ./your_program
```

## Configuration

Control profiler behavior with environment variables:

- `PROFILER_STACK_TRACES` - Show stack traces (default: enabled)
  - `1` or unset: Show compact stack traces (top 7 frames)
  - `0`: Hide stack traces, show only addresses

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

**Memory Leak Detection:**
```
========== MEMORY LEAKS ==========
[LEAK] 0x55555555a2a0: 1024 bytes
./libprofiler.so(malloc+0x84)[0x7f...]
./your_program(create_buffer+0x1a)[0x55...]
./your_program(main+0x43)[0x55...]

[LEAK] 0x55555555a6b0: 512 bytes
./libprofiler.so(malloc+0x84)[0x7f...]
./your_program(helper_function+0x30)[0x55...]
./your_program(main+0xd4)[0x55...]

Summary:
  Real leaks: 2 allocation(s), 1536 bytes
  Libc infrastructure: 1 allocation(s), 1024 bytes (ignored)
==================================
```

**Corruption Detection:**
```
[CORRUPTION] Double-Free or Invalid-Free at 0x55555555a2a0
./libprofiler.so(+0x15dd)[0x7f...]
./libprofiler.so(free+0x85)[0x7f...]
./your_program(main+0x8b)[0x55...]
```

## Project Structure

```
c-systems-integrity-lib/
├── src/
│   ├── malloc_intercept.c   # LD_PRELOAD interception + corruption detection
│   ├── hash_table.c         # O(1) allocation tracking with uthash
│   └── profiler.c           # Library initialization
├── include/
│   ├── profiler_internal.h  # Internal data structures
│   └── uthash.h             # Third-party hash table library
├── tests/
│   ├── test_simple_leak.c   # Basic leak detection
│   ├── test_no_leak.c       # Zero-leak verification
│   ├── test_complex_leak.c  # Real-world simulation (7 leaks)
│   ├── test_double_free.c   # Double-free detection (4 scenarios)
│   └── test_invalid_free.c  # Invalid-free detection (5 scenarios)
├── Makefile                 # Build system
└── CMemoryLeaker.md         # Full project specification
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

