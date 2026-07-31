#include "mm_override.h"
#include "mm.h"

void* malloc(size_t size) {
    return nalloc(size);
}

void free(void* ptr) {
    return nfree(ptr);
}

void* realloc(void* ptr, size_t size) {
    return nrealloc(ptr, size);
}
