/*
 * test invalid-free detection
 * 
 * this test attempts to free pointers that were never allocated,
 * including stack variables, random addresses, and literals.
 * 
 * expected behavior:
 * - profiler detects pointer not in tracking table
 * - reports error immediately
 * - does NOT call real free() (would crash!)
 */

#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("Test: Invalid-Free Detection\n");
    printf("=============================\n\n");
    
    // scenario 1: free a stack variable
    int stack_var = 42;
    free(&stack_var);  // invalid!
    
    // scenario 2: free a random address
    void *random_addr = (void*)0xDEADBEEF;
    free(random_addr);  // invalid!
    
    // scenario 3: free a string literal
    char *literal = "This is a constant string";
    free(literal);  // invalid!
    
    // scenario 4: free middle of allocated buffer
    char *buffer = malloc(100);
    char *middle = buffer + 50;
    free(middle);  // invalid! only base pointer can be freed
    
    // clean up properly
    free(buffer);
    
    // scenario 5: free after modifying pointer
    int *numbers = malloc(10 * sizeof(int));
    void *original = numbers;
    
    free(numbers);
    
    // modify the pointer slightly
    numbers = (int*)((char*)numbers + 1);
    free(numbers);  // invalid!
    
    printf("\n==========================================\n");
    printf("Expected: 5 corruption errors detected\n");
    printf("  (stack, random, literal, middle, modified)\n");
    printf("==========================================\n\n");
    
    return 0;
}
