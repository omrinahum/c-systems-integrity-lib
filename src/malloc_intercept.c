/*
 * Memory Profiler - Main Interception Layer
 * 
 * This file implements the LD_PRELOAD magic that intercepts malloc/free.
 * 
 * HOW IT WORKS:
 * 1. We define malloc() and free() functions here
 * 2. When the program is run with LD_PRELOAD=libprofiler.so, the dynamic
 *    linker loads OUR functions BEFORE libc's
 * 3. Our functions intercept the call, track it, then call the REAL malloc/free
 * 
 * THE BOOTSTRAP PROBLEM:
 * Our tracking code needs to allocate memory for metadata. If we track
 * that allocation, we get infinite recursion:
 *   malloc() -> track() -> malloc() -> track() -> ...
 * 
 * SOLUTION (for now):
 * We'll use a simple flag. When we're inside profiler code, we won't track.
 * Later we'll make this thread-safe with thread-local storage.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>
#include <execinfo.h>  // For backtrace()
#include "../include/profiler_internal.h"

// Maximum stack frames to capture
#define MAX_STACK_FRAMES 16

// Function pointers to the lib.c real malloc/free 
static void* (*real_malloc)(size_t) = NULL;
static void (*real_free)(void*) = NULL;
static void* (*real_calloc)(size_t, size_t) = NULL;
static void* (*real_realloc)(void*, size_t) = NULL;

// Export these for hash_table.c to use
void* (*real_malloc_ptr)(size_t) = NULL;
void (*real_free_ptr)(void*) = NULL;

// Bootstrap protection - prevents tracking our own allocations
static int in_profiler = 0;

// Initialization flags  
static int profiler_initialized = 0;
static int first_malloc_done = 0;

/*
 * Initialize the profiler
 * 
 * This is called once, on the first malloc/free call.
 * We use dlsym() to get pointers to the real libc functions.
 * 
 */
static void profiler_init(void) {
    if (profiler_initialized) return;
    
    // Mark as initialized to prevent re-entry
    profiler_initialized = 1;
    
    // Prevent recursion during initialization 
    in_profiler = 1;
    
    // Get real function pointers before any output
    // for me: dlsym gets the next implemntation of the second arg (eg malloc) function 
    // in the linking- in our instacne the lib.c ones
    real_malloc = dlsym(RTLD_NEXT, "malloc");
    real_free = dlsym(RTLD_NEXT, "free");
    real_calloc = dlsym(RTLD_NEXT, "calloc");
    real_realloc = dlsym(RTLD_NEXT, "realloc");
    
    // coudlnt catch the lib.c fucntions- exit
    if (!real_malloc || !real_free) {
        fprintf(stderr, "[PROFILER ERROR] Failed to find real malloc/free\n");
        exit(1);
    }
    
    // Export for hash_table to use
    real_malloc_ptr = real_malloc;
    real_free_ptr = real_free;
    
    // Now we can print (fprintf may call malloc internally)
    fprintf(stderr, "[PROFILER] Initializing memory profiler...\n");
    
    // Initialize our tracking system
    hash_table_init();
    
    fprintf(stderr, "[PROFILER] Initialization complete\n");
    // turn off the flag so we could keep track
    in_profiler = 0;
}

/*
 * Cleanup function - called at program exit
 * 
 * We use __attribute__((destructor)) to tell GCC to call this 
 * automatically when the shared library is unloaded (program exit).
 * when it happends we create the report
 */
__attribute__((destructor))
static void profiler_cleanup(void) {
    fprintf(stderr, "[PROFILER] Generating final report...\n");
    hash_table_report_leaks();
    hash_table_cleanup();
}

/*
 * INTERCEPTED malloc()
 * 
 * This is the function that gets called instead of libc's malloc.
 * 
 * Flow:
 * 1. Initialize profiler if first call
 * 2. Call real malloc
 * 3. Track the allocation (unless we're already in profiler code)
 * 4. Return the pointer
 */
void* malloc(size_t size) {
    // Initialize on first call
    if (!profiler_initialized) {
        profiler_init();
    }
    
    // Call the real malloc and save the adress
    void *ptr = real_malloc(size);
    
    // temp soltion:Skip tracking until after first malloc completes-
    // im assuming first malloc would be for printf buffers- so i drop it for now 
    if (!first_malloc_done) {
        first_malloc_done = 1;
        return ptr;
    }
    
    // Track it only if were not in the profiler code- so we can prevent infinity loops 
    // for me: eg malloc() -> track() -> malloc() -> track() -> ...
    if (!in_profiler && ptr) {
        in_profiler = 1;
        
        // Capture stack trace- backtrace is keeping the return address in the trace array (all of the stack return addresses)
        // lets say main calls help calls help 2, both main and help get inside the array, also return the depth of the stack
        void *trace[MAX_STACK_FRAMES];
        int depth = backtrace(trace, MAX_STACK_FRAMES);
        
        // Track the allocation with stack trace
        hash_table_add(ptr, size, trace, depth);
        in_profiler = 0;
    }
    
    return ptr;
}

/*
 * INTERCEPTED free()
 * 
 * Similar to malloc - intercept, track, delegate.
 */
void free(void *ptr) {
    // Initialize if needed (shouldn't happen, but be safe)
    if (!profiler_initialized) {
        profiler_init();
    }
    
    // Don't free NULL 
    if (!ptr) return;
    
    // Remove from tracking
    if (!in_profiler) {
        in_profiler = 1;
        hash_table_remove(ptr);
        in_profiler = 0;
    }
    
    // Call real free
    real_free(ptr);
}

/*
 * INTERCEPTED calloc()
 * 
 * calloc allocates AND zeros memory. track it like malloc.
 */
void* calloc(size_t nmemb, size_t size) {
    if (!profiler_initialized) {
        profiler_init();
    }
    
    // call real lib.c calloc and track it
    void *ptr = real_calloc(nmemb, size);
    
    // when pointer isnt null and its the first calloc, track
    if (!in_profiler && ptr) {
        in_profiler = 1;
        
        // Capture stack trace
        void *trace[MAX_STACK_FRAMES];
        int depth = backtrace(trace, MAX_STACK_FRAMES);
        
        hash_table_add(ptr, nmemb * size, trace, depth);
        in_profiler = 0;
    }
    
    return ptr;
}

/*
 * INTERCEPTED realloc()
 * 
 * - If ptr is NULL, acts like malloc
 * - If size is 0, acts like free
 * - Otherwise, may move the allocation to a new address
 * 
 * We need to remove the old tracking and add new tracking.
 */
void* realloc(void *ptr, size_t size) {
    if (!profiler_initialized) {
        profiler_init();
    }
    
    // If ptr is NULL, this is just malloc
    if (!ptr) {
        return malloc(size);
    }
    
    // If size is 0, this is just free
    if (size == 0) {
        free(ptr);
        return NULL;
    }
    
    // Call real realloc from lib.c
    void *new_ptr = real_realloc(ptr, size);
    
    // Update tracking: remove old, add new
    if (!in_profiler) {
        in_profiler = 1;
        hash_table_remove(ptr);
        if (new_ptr) {
            // Capture stack trace
            void *trace[MAX_STACK_FRAMES];
            int depth = backtrace(trace, MAX_STACK_FRAMES);
            
            hash_table_add(new_ptr, size, trace, depth);
        }
        in_profiler = 0;
    }
    
    return new_ptr;
}
