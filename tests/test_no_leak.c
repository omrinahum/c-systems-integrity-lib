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
    
    // Allocate and free multiple times
    for (int i = 0; i < 5; i++) {
        printf("Iteration %d: Allocating 1024 bytes...\n", i);
        void *ptr = malloc(1024);
        if (ptr) {
            memset(ptr, i, 1024);
            printf("  Address: %p\n", ptr);
            free(ptr);
            printf("  Freed\n");
        }
    }
    
    // Test calloc
    printf("\nTesting calloc (2048 bytes)...\n");
    void *ptr2 = calloc(512, 4);
    if (ptr2) {
        printf("  Address: %p\n", ptr2);
        free(ptr2);
        printf("  Freed\n");
    }
    
    // Test realloc
    printf("\nTesting realloc...\n");
    void *ptr3 = malloc(100);
    printf("  Initial address: %p (100 bytes)\n", ptr3);
    ptr3 = realloc(ptr3, 200);
    printf("  After realloc: %p (200 bytes)\n", ptr3);
    free(ptr3);
    printf("  Freed\n");
    
    printf("\nProgram ending...\n");
    printf("Expected: NO memory leaks detected\n");
    
    return 0;
}
