/*
 * Test: No Memory Leaks
 * 
 * This program properly frees all allocations.
 * The profiler should report ZERO leaks.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    printf("Test: No Memory Leaks\n");
    printf("=====================\n\n");
    
    // allocate and free multiple times
    for (int i = 0; i < 5; i++) {
        void *ptr = malloc(1024);
        if (ptr) {
            memset(ptr, i, 1024);
            free(ptr);
        }
    }
    printf("loop(5x): freed \n");
    
    // test calloc
    void *ptr2 = calloc(512, 4);
    if (ptr2) {
        free(ptr2);
    }
    printf("calloc: freed \n");
    
    // test realloc
    void *ptr3 = malloc(100);
    ptr3 = realloc(ptr3, 200);
    free(ptr3);
    printf("realloc: freed \n\n");
    
    printf("==========================================\n");
    printf("Expected Leaks: 0 allocations\n");
    printf("All memory properly freed\n");
    printf("==========================================\n");
    
    return 0;
}
