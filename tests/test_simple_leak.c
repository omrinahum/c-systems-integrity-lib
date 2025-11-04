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
    
    // leak 1: 1024 bytes
    void *leak1 = malloc(1024);
    if (leak1) {
        memset(leak1, 0, 1024);
    }
    printf("leak1: %p (LEAK)\n", leak1);
    
    // leak 2: 512 bytes
    void *leak2 = malloc(512);
    if (leak2) {
        memset(leak2, 0, 512);
    }
    printf("leak2: %p (LEAK)\n", leak2);
    
    // proper allocation and free
    void *proper = malloc(256);
    if (proper) {
        memset(proper, 0, 256);
        free(proper);
    }
    printf("proper: freed \n\n");
    
    printf("==========================================\n");
    printf("Expected Leaks: 2 allocations\n");
    printf("  1. leak1 (1024 bytes)\n");
    printf("  2. leak2 (512 bytes)\n");
    printf("Total: 1536 bytes leaked\n");
    printf("==========================================\n");
    
    return 0;
}
