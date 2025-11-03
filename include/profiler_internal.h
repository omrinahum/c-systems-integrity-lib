#ifndef PROFILER_INTERNAL_H
#define PROFILER_INTERNAL_H

#include <stddef.h>
#include <stdint.h>
#include <time.h>

/* 
 * Allocation metadata stored for each malloc() call
 * 
 * We store the information needed to detect leaks:
 * - ptr: The address returned by malloc (used as hash key)
 * - size: Number of bytes allocated
 * - timestamp: When allocation occurred (for debugging)
 * - stack_trace: Array of return addresses (from backtrace)
 * - stack_depth: Number of frames captured
 */
typedef struct allocation_info {
    void *ptr;              // The allocated address
    size_t size;            // Bytes allocated
    time_t timestamp;       // When it was allocated
    void **stack_trace;     // Array of return addresses
    int stack_depth;        // Number of frames in stack_trace
    struct allocation_info *next;  // For linked list (will become uthash later)
} allocation_info_t;

/*
 * Global state for the profiler
 * 
 * This structure holds everything the profiler needs to track.
 * For now, it's just a pointer to the hash table, but we'll
 * expand this as we add features (mutex, stats, etc.)
 */
typedef struct profiler_state {
    allocation_info_t *allocations;  // Hash table of active allocations
    int initialized;                  // Flag to prevent re-initialization
} profiler_state_t;

// Function declarations for hash table (allocation tracking)
void hash_table_init(void);
void hash_table_add(void *ptr, size_t size, void **trace, int depth);
void hash_table_remove(void *ptr);
void hash_table_report_leaks(void);
void hash_table_cleanup(void);

// Real libc function pointers (set by malloc_intercept.c)
extern void* (*real_malloc_ptr)(size_t);
extern void (*real_free_ptr)(void*);

#endif // PROFILER_INTERNAL_H
