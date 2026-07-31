#include <stddef.h>

extern void* malloc(size_t size);
extern void free(void* ptr);
extern void* realloc(void* ptr, size_t size);