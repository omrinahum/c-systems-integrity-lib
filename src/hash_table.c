/*
 * Hash Table - Allocation Registry
 * 
 * Manages a hash table of active memory allocations.
 * Currently using a simple linked list (will upgrade to uthash later).
 */

#define _GNU_SOURCE  
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <execinfo.h>  
#include <unistd.h>     
#include "../include/profiler_internal.h"

// Global state
static allocation_info_t *g_allocations = NULL;

/*
 * Initialize the tracker
 * Currently does nothing, but we'll add mutex initialization here later
 */
void hash_table_init(void) {
    g_allocations = NULL;
    fprintf(stderr, "[PROFILER] Hash table initialized\n");
}

/*
 * Add an allocation to our tracking table
 * 
 * Called immediately after malloc() succeeds.
 * We allocate memory for the metadata itself - this is important:
 * we use the REAL malloc, not our intercepted one (to avoid recursion)
 */
void hash_table_add(void *ptr, size_t size ,void **trace, int depth) {
    if (!ptr) return;  // Don't track NULL returns
    
    // Don't track if real_malloc_ptr isn't set yet (during early init)
    if (!real_malloc_ptr) return;
    
    // Allocate metadata structure node (for now) using real malloc from lib.c
    // This prevents infinite recursion
    allocation_info_t *info = (allocation_info_t*)real_malloc_ptr(sizeof(allocation_info_t));
    if (!info) {
        fprintf(stderr, "[PROFILER ERROR] Failed to allocate tracking metadata\n");
        return;
    }
    
    info->ptr = ptr;
    info->size = size;
    info->timestamp = time(NULL);
    
    // Allocate and copy stack trace
    info->stack_trace = real_malloc_ptr(depth * sizeof(void*));
    // copy the trace stack data into the sturct array
    if (info->stack_trace) {
        memcpy(info->stack_trace, trace, depth * sizeof(void*));
        info->stack_depth = depth;
    } else {
        info->stack_depth = 0;  // Allocation failed, no trace
    }
    
    // Add to front of linked list
    info->next = g_allocations;
    g_allocations = info;
}

/*
 * Remove an allocation from tracking
 * 
 * Called when free() is called. We search for the pointer
 * and remove it from our list. If not found, it's either:
 * 1. A double-free (freed twice)
 * 2. An invalid free (never allocated)
 * 
 * For now, we'll just remove it. Later we'll add error detection.
 */
void hash_table_remove(void *ptr) {
    if (!ptr) return;
    
    // set for the linked list search
    allocation_info_t *current = g_allocations;
    allocation_info_t *prev = NULL;
    
    // Search for the allocation
    while (current) {
        if (current->ptr == ptr) {
            // Found it - remove from list
            if (prev) {
                prev->next = current->next;
            } else {
                g_allocations = current->next;
            }
            // Free the stack trace array first, then the struct
            if (current->stack_trace) {
                real_free_ptr(current->stack_trace);
            }
            real_free_ptr(current);  // Free the metadata using real free
            return;
        }
        prev = current;
        current = current->next;
    }
    
    // Not found - this is a bug in the application
    // For now, just ignore. We'll add detection in Phase 4
}

/*
 * Report all leaked allocations
 * 
 * Called at program exit. Anything still in our hash table
 * was allocated but never freed = memory leak.
 */
void hash_table_report_leaks(void) {
    // set start node
    allocation_info_t *current = g_allocations;
    int leak_count = 0;
    size_t total_leaked = 0;
    
    fprintf(stderr, "\n========== MEMORY LEAK REPORT ==========\n");
    
    // iterate thorugh all the reamin nodes and print thier metadata
    while (current) {
        fprintf(stderr, "[LEAK] %p: %zu bytes (allocated at timestamp %ld)\n",
                current->ptr, current->size, (long)current->timestamp);
        
        // Print stack trace if available
        if (current->stack_trace && current->stack_depth > 0) {
            fprintf(stderr, "  Allocated at:\n");
            // for me : backtrace_symbols_fd args: addresses (array), count (int), file_descriptor;
            backtrace_symbols_fd(current->stack_trace, current->stack_depth, STDERR_FILENO);
        }
        fprintf(stderr, "\n");  
        
        leak_count++;
        total_leaked += current->size;
        current = current->next;
    }
    
    if (leak_count == 0) { // everything freed
        fprintf(stderr, "No memory leaks detected! ✓\n");
    } else { 
        fprintf(stderr, "\nSummary: %d leaks, %zu bytes total\n", 
                leak_count, total_leaked);
    }
    
    fprintf(stderr, "========================================\n");
}

/*
 * Cleanup tracker state
 * 
 * Free all tracking metadata. Called at exit.
 */
void hash_table_cleanup(void) {
    // set start node
    allocation_info_t *current = g_allocations;
    // iterate through the whole remained list and free the memory using the lib.c malloc
    while (current) {
        allocation_info_t *next = current->next;
        // Free stack trace array first, then the struct
        if (current->stack_trace) {
            real_free_ptr(current->stack_trace);
        }
        real_free_ptr(current);
        current = next;
    }
    g_allocations = NULL;
}
