/*
 * test double-free detection
 * 
 * this test intentionally frees the same pointer twice
 * to verify the profiler catches it.
 * 
 * expected behavior:
 * - first malloc: tracked
 * - first free: removed from tracking
 * - second free: detected as double-free, error reported
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    printf("Test: Double-Free Detection\n");
    printf("============================\n\n");
    
    // scenario 1: basic double-free
    char *buffer1 = malloc(100);
    strcpy(buffer1, "This will be double-freed");
    
    free(buffer1);
    free(buffer1);  // double-free!
    
    // scenario 2: double-free with interleaved allocations
    char *buffer2 = malloc(200);
    char *buffer3 = malloc(300);
    
    free(buffer2);
    free(buffer3);
    free(buffer2);  // double-free!
    
    // scenario 3: triple-free
    int *numbers = malloc(10 * sizeof(int));
    
    free(numbers);
    free(numbers);  // double-free!
    free(numbers);  // triple-free!
    
    printf("\n==========================================\n");
    printf("Expected: 4 corruption errors detected\n");
    printf("  (1 basic, 1 interleaved, 2 triple)\n");
    printf("==========================================\n\n");
    
    return 0;
}
