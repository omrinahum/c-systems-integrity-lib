/*
 * comprehensive memory leak test
 * 
 * simulates real-world code with:
 * - helper functions
 * - library-style abstraction
 * - deep call stacks
 * - mixed leaked and properly freed allocations
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// library-style abstraction (simulates external library code)
// ============================================================================

typedef struct {
    char *data;
    size_t size;
} buffer_t;

/*
 * library function: creates a buffer (deep in call stack)
 */
buffer_t* buffer_create(size_t size) {
    buffer_t *buf = malloc(sizeof(buffer_t));
    if (!buf) return NULL;
    
    buf->data = malloc(size);
    buf->size = size;
    
    if (buf->data) {
        memset(buf->data, 0, size);
    }
    
    return buf;
}

/*
 * library function: frees a buffer
 */
void buffer_free(buffer_t *buf) {
    if (buf) {
        free(buf->data);
        free(buf);
    }
}

/*
 * library function: resize buffer (tests realloc)
 */
int buffer_resize(buffer_t *buf, size_t new_size) {
    if (!buf) return -1;
    
    char *new_data = realloc(buf->data, new_size);
    if (!new_data) return -1;
    
    buf->data = new_data;
    buf->size = new_size;
    return 0;
}

// ============================================================================
// helper functions (simulates application helper code)
// ============================================================================

/*
 * helper: allocates message buffer (will leak)
 */
char* helper_create_message(const char *text) {
    size_t len = strlen(text) + 1;
    char *msg = malloc(len);
    if (msg) {
        strcpy(msg, text);
    }
    return msg;
}

/*
 * helper: processes data with temporary allocation (properly freed)
 */
void helper_process_data(const char *input) {
    // temporary buffer - will be freed
    char *temp = malloc(256);
    if (temp) {
        snprintf(temp, 256, "Processed: %s", input);
        free(temp);  // properly freed
    }
}

/*
 * deep call stack: level 3
 */
void* deep_level_3(size_t size) {
    return malloc(size);
}

/*
 * deep call stack: level 2
 */
void* deep_level_2(size_t size) {
    return deep_level_3(size);
}

/*
 * deep call stack: level 1
 */
void* deep_level_1(size_t size) {
    return deep_level_2(size);
}

// ============================================================================
// main test scenarios
// ============================================================================

int main(void) {
    printf("Test: Comprehensive Memory Leak Detection\n");
    printf("==========================================\n\n");
    
    // scenario 1: library-style buffer management
    buffer_t *buf1 = buffer_create(1024);
    printf("buf1: %p (LEAK)\n", (void*)buf1);
    
    buffer_t *buf2 = buffer_create(512);
    buffer_free(buf2);
    printf("buf2: freed \n");
    
    buffer_t *buf3 = buffer_create(256);
    buffer_resize(buf3, 768);
    buffer_free(buf3);
    printf("buf3: freed \n\n");
    
    // scenario 2: helper functions with mixed behavior
    char *msg1 = helper_create_message("Important message");
    printf("msg1: %p (LEAK)\n", (void*)msg1);
    
    char *msg2 = helper_create_message("Temporary message");
    free(msg2);
    printf("msg2: freed \n");
    
    helper_process_data("test data");
    printf("helper: freed \n\n");
    
    // scenario 3: deep call stack allocations
    void *deep1 = deep_level_1(2048);
    printf("deep1: %p (LEAK)\n", deep1);
    
    void *deep2 = deep_level_1(1536);
    free(deep2);
    printf("deep2: freed \n\n");
    
    // scenario 4: array of allocations (partial leak)
    void *array[5];
    for (int i = 0; i < 5; i++) {
        array[i] = malloc(128 * (i + 1));
    }
    
    for (int i = 0; i < 3; i++) {
        free(array[i]);
    }
    printf("array[0-2]: freed \n");
    printf("array[3-4]: %p, %p (LEAK)\n\n", array[3], array[4]);
    
    // scenario 5: calloc usage
    int *numbers1 = calloc(100, sizeof(int));
    printf("numbers1: %p (LEAK)\n", (void*)numbers1);
    
    int *numbers2 = calloc(200, sizeof(int));
    free(numbers2);
    printf("numbers2: freed \n\n");
    
    // summary
    printf("==========================================\n");
    printf("Expected Leaks: 7 allocations\n");
    printf("  1. Buffer metadata (16 bytes) - buf1 struct\n");
    printf("  2. Buffer data (1024 bytes) - buf1->data\n");
    printf("  3. Message (18 bytes) - msg1\n");
    printf("  4. Deep allocation (2048 bytes) - deep1\n");
    printf("  5. Array[3] (512 bytes)\n");
    printf("  6. Array[4] (640 bytes)\n");
    printf("  7. Calloc (400 bytes) - numbers1\n");
    printf("Total: 4658 bytes leaked\n");
    printf("==========================================\n");
    
    return 0;
}
