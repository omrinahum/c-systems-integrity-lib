/*
 * hash table - allocation registry
 * 
 * manages a hash table of active memory allocations.
 * uses uthash for O(1) performance.
 * thread-safe with pthread mutex.
 */

#define _GNU_SOURCE  
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <execinfo.h>  
#include <unistd.h>
#include <pthread.h>     
#include "../include/profiler_internal.h"
#include "../include/uthash.h"

// global state
static allocation_info_t *g_allocations = NULL;

// mutex to protect hash table from concurrent access
// PTHREAD_MUTEX_INITIALIZER: static initialization, safe before any threads exist
static pthread_mutex_t hash_table_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * initialize the tracker
 * 
 * currently minimal - just resets the allocation list.
 */
void hash_table_init(void) {
    g_allocations = NULL;
}

/*
 * add an allocation to our tracking table
 * 
 * called immediately after malloc() succeeds.
 * we use real_malloc_ptr to allocate metadata (avoids recursion).
 * uthash's HASH_ADD_PTR does O(1) insertion.
 */
void hash_table_add(void *ptr, size_t size ,void **trace, int depth, int is_suspicious) {
    if (!ptr) return;
    
    // don't track if real_malloc_ptr isn't set yet (during early init)
    if (!real_malloc_ptr) return;
    
    // allocate metadata structure using real malloc (prevents infinite recursion)
    allocation_info_t *info = (allocation_info_t*)real_malloc_ptr(sizeof(allocation_info_t));
    if (!info) {
        fprintf(stderr, "[PROFILER ERROR] Failed to allocate tracking metadata\n");
        return;
    }
    
    info->ptr = ptr;
    info->size = size;
    info->timestamp = time(NULL);
    info->is_suspicious = is_suspicious;
    
    // allocate and copy stack trace
    info->stack_trace = real_malloc_ptr(depth * sizeof(void*));
    if (info->stack_trace) {
        memcpy(info->stack_trace, trace, depth * sizeof(void*));
        info->stack_depth = depth;
    } else {
        info->stack_depth = 0;
    }
    
    // lock before modifying shared hash table
    pthread_mutex_lock(&hash_table_mutex);
    
    // add to hash table - O(1) operation
    // for me : HASH_ADD_PTR(head, keyfield, item)
    HASH_ADD_PTR(g_allocations, ptr, info);
    
    // unlock after modification complete
    pthread_mutex_unlock(&hash_table_mutex);
}

/*
 * remove an allocation from tracking
 * 
 * called when free() is called.
 * uthash's HASH_FIND_PTR does O(1) lookup.
 * 
 * thread safety: protected by hash_table_mutex
 */
void hash_table_remove(void *ptr) {
    if (!ptr) return;
    
    allocation_info_t *found;
    
    // lock before accessing shared hash table
    pthread_mutex_lock(&hash_table_mutex);
    
    // find the entry in hash table - O(1) operation
    // for me : HASH_FIND_PTR(head, key_ptr, output)
    HASH_FIND_PTR(g_allocations, &ptr, found);
    
    if (found) {
        // remove from hash table - O(1) operation
        HASH_DEL(g_allocations, found);
    }
    
    // unlock before freeing memory (don't need lock for that)
    pthread_mutex_unlock(&hash_table_mutex);
    
    // free outside the critical section 
    if (found) {
        // free the stack trace array first, then the struct
        if (found->stack_trace) {
            real_free_ptr(found->stack_trace);
        }
        real_free_ptr(found);
    }
    
    // not found - could be double-free or invalid-free
    // for now, just ignore. we'll add detection in phase 4
}

/*
 * check if an allocation exists in the hash table
 * 
 * called by free() to validate pointer before freeing.
 * returns 1 if found, 0 if not found.
 * 
 * thread safety: protected by hash_table_mutex
 */
int hash_table_find(void *ptr) {
    if (!ptr) return 0;
    
    allocation_info_t *found;
    
    // lock before accessing shared hash table
    pthread_mutex_lock(&hash_table_mutex);
    
    // find the entry in hash table - O(1) operation
    HASH_FIND_PTR(g_allocations, &ptr, found);
    
    // unlock immediately after lookup
    pthread_mutex_unlock(&hash_table_mutex);
    
    return (found != NULL) ? 1 : 0;
}

/*
 * report all leaked allocations
 * 
 * called at program exit.
 * anything still in our table was allocated but never freed = memory leak.
 * uses HASH_ITER to safely iterate through all hash table entries.
 * 
 * separates output into confirmed leaks vs suspicious leaks (likely libc).
 */
void hash_table_report_leaks(void) {
    allocation_info_t *current, *tmp;
    int confirmed_count = 0;
    int suspicious_count = 0;
    size_t confirmed_bytes = 0;
    size_t suspicious_bytes = 0;
    
    // first pass: count and report confirmed leaks (user code)
    HASH_ITER(hh, g_allocations, current, tmp) {
        if (!current->is_suspicious) {
            if (confirmed_count == 0) {
                fprintf(stderr, "\n========== MEMORY LEAKS ==========\n");
            }
            fprintf(stderr, "[LEAK] %p: %zu bytes\n", current->ptr, current->size);
            
            // show stack trace if enabled (compact format - top 7 frames only)
            if (show_stack_traces && current->stack_trace && current->stack_depth > 0) {
                int frames_to_show = (current->stack_depth < 7) ? current->stack_depth : 7;
                backtrace_symbols_fd(current->stack_trace, frames_to_show, STDERR_FILENO);
            }
            fprintf(stderr, "\n");
            
            confirmed_count++;
            confirmed_bytes += current->size;
        } else {
            suspicious_count++;
            suspicious_bytes += current->size;
        }
    }
    
    // summary
    if (confirmed_count > 0 || suspicious_count > 0) {
        fprintf(stderr, "\nSummary:\n");
        fprintf(stderr, "  Real leaks: %d allocation(s), %zu bytes\n", confirmed_count, confirmed_bytes);
        if (suspicious_count > 0) {
            fprintf(stderr, "  Libc infrastructure: %d allocation(s), %zu bytes (ignored)\n", 
                    suspicious_count, suspicious_bytes);
        }
        fprintf(stderr, "==================================\n\n");
    }
}

/*
 * cleanup tracker state
 * 
 * free all tracking metadata. called at exit.
 * uses HASH_ITER to safely delete all entries.
 * 
 * thread safety: at exit, program is single-threaded, so no need to lock
 */
void hash_table_cleanup(void) {
    allocation_info_t *current, *tmp;
    
    // iterate through the remain data in the hash and delete them
    // at program exit, we're single-threaded, so no lock needed
    HASH_ITER(hh, g_allocations, current, tmp) {
        HASH_DEL(g_allocations, current);  // remove from hash table
        
        // free stack trace array first, then the struct
        if (current->stack_trace) {
            real_free_ptr(current->stack_trace);
        }
        real_free_ptr(current);
    }
    
    g_allocations = NULL;
}
