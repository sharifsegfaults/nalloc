#pragma once

#include <stdint.h>

typedef struct {
    uint32_t* ids;
    uint32_t* elems;
    uint32_t size;
} stivec_t;

stivec_t create_stivec(uint32_t max_size);
uint32_t stivec_insert(stivec_t* stivec, uint32_t val);
void stivec_remove(stivec_t* stivec, uint32_t id);
uint32_t stivec_get(stivec_t* stivec, uint32_t id);
uint32_t stivec_random_pop(stivec_t* stivec);
