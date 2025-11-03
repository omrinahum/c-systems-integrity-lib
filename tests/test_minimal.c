/*
 * Minimal test - just one malloc, one free
 */
#include <stdlib.h>

int main() {
    void *ptr = malloc(500);
    free(ptr);
    return 0;
}
