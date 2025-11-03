/*
 * Profiler Initialization & Coordination
 * 
 * This file handles library-wide initialization and cleanup.
 * Currently minimal since malloc_intercept.c handles most init logic.
 */

#define _GNU_SOURCE // to expose advanced linux internal fucntions
#include <stdio.h>
#include "../include/profiler_internal.h"

// Library constructor - runs when .so is loaded
__attribute__((constructor))
static void profiler_lib_init(void) {
    // Currently initialization happens on first malloc call
    // This could be extended to read config, setup signal handlers, etc.
}

// Library destructor - runs when .so is unloaded  
__attribute__((destructor))
static void profiler_lib_cleanup(void) {
    // Final cleanup happens in malloc_intercept.c
}
