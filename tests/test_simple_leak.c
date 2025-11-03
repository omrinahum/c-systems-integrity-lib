/*
 * Test: Simple Memory Leak
 * 
 * This program intentionally leaks memory to test our profiler.
 * It allocates memory but never frees it.
 * 
 * Expected output from profiler:
 * - Should detect 2 leaks (1024 bytes + 512 bytes)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    printf("Test: Simple Memory Leak\n");
    printf("========================\n\n");
    
    // Leak 1: Allocate 1024 bytes and forget about it
    printf("Allocating 1024 bytes...\n");
    void *leak1 = malloc(1024);
    if (leak1) {
        memset(leak1, 0, 1024);  // Use it a bit
        printf("  Address: %p\n", leak1);
    }
    
    // Leak 2: Allocate 512 bytes
    printf("Allocating 512 bytes...\n");
    void *leak2 = malloc(512);
    if (leak2) {
        memset(leak2, 0, 512);
        printf("  Address: %p\n", leak2);
    }
    
    // Proper allocation and free (should NOT be reported)
    printf("Allocating 256 bytes and freeing...\n");
    void *proper = malloc(256);
    if (proper) {
        memset(proper, 0, 256);
        printf("  Address: %p\n", proper);
        free(proper);  // This should NOT leak
        printf("  Freed successfully\n");
    }
    
    printf("\nProgram ending...\n");
    printf("Expected: Profiler should detect 1536 bytes leaked (2 allocations)\n");
    
    // Exit without freeing leak1 and leak2
    return 0;
}
