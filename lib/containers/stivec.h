#pragma once

#include <stddef.h>

typedef struct {
    size_t* ids;
    size_t* elems;
    size_t size;
} stivec_t;

stivec_t create_stivec(size_t max_size);
size_t stivec_insert(stivec_t* stivec, size_t val);
void stivec_remove(stivec_t* stivec, size_t id);
size_t stivec_get(stivec_t* stivec, size_t id);
size_t stivec_random_pop(stivec_t* stivec);
