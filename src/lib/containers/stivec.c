#include <stdlib.h>
#include <assert.h>
#include "containers/stivec.h"

stivec_t create_stivec(size_t max_size) {
    stivec_t stivec;
    stivec.ids = malloc(sizeof(size_t) * max_size);
    stivec.elems = malloc(sizeof(size_t) * max_size);
    stivec.size = 0;

    for (int i = 0; i < max_size; ++i) {
        stivec.ids[i] = i;
    }

    return stivec;
};

size_t stivec_insert(stivec_t* stivec, size_t elem) {
    stivec->elems[stivec->size] = elem;
    ++stivec->size;
    return stivec->ids[stivec->size-1];
}

void stivec_remove(stivec_t* stivec, size_t id) {
    size_t pos = stivec->ids[id];
    size_t tmp_elm = stivec->elems[pos];

    stivec->elems[pos] = stivec->elems[stivec->size-1];
    stivec->ids[pos] = stivec->ids[stivec->size-1];
    stivec->ids[stivec->size-1] = pos;

    --stivec->size;
}

size_t stivec_get(stivec_t* stivec, size_t id) {
    size_t pos = stivec->ids[id];
    return stivec->elems[pos];
}

size_t stivec_random_pop(stivec_t* stivec) {
    assert(stivec->size >= 0);
    size_t pos = rand() % (stivec->size);
    size_t tmp_elem = stivec->elems[pos];

    stivec->elems[pos] = stivec->elems[stivec->size-1];
    stivec->ids[pos] = stivec->ids[stivec->size-1];
    stivec->ids[stivec->size-1] = pos;

    --stivec->size;
    return tmp_elem;
}
