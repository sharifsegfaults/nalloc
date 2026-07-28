#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <time.h>
#include "mm.h"
#include "utils.h"
#include "containers/stivec.h"

static char *heap;

void setUp() {
    srand(time(NULL));
}

uint32_t curr_seed = 1;

char* randbytes(uint32_t size) {
    char* bytes = malloc(size);
    for (int i = 0; i < size; ++i) {
        bytes[i] = curr_seed;
    }
    ++curr_seed;
    return bytes;
}

typedef struct {
    bool free;
    uint32_t data_size;
    char* data;
    char* memptr;
} Block;

Block create_block(uint32_t size) {
    Block bk;
    bk.data = randbytes(size);
    bk.memptr = nalloc(size);
    memcpy(bk.memptr, bk.data, size);
    bk.data_size = size;
    bk.free = false;

    return bk;
}

void free_block(Block bk) {
    nfree(bk.memptr);
    bk.free = true;
}

Block realloc_block(Block bk, uint32_t size) {
    assert(!memcmp(bk.memptr, bk.data, bk.data_size));
    bk.memptr = nrealloc(bk.memptr, size);
    // Check data is not lost
    assert(!memcmp(bk.memptr, bk.data, MIN(bk.data_size, size)));
    free(bk.data);
    bk.data = randbytes(size);
    memcpy(bk.memptr, bk.data, size);
    bk.data_size = size;
    assert(!memcmp(bk.memptr, bk.data, bk.data_size));
    return bk;
}

/* --------------------------------- RANDOM --------------------------------- */
uint32_t randint(uint32_t max) {
    return rand() % (max + 1);
}

bool E2E_TEST_1(size_t seed) {
    mm_init();
    printf("SEED: %zu\n", seed);

    uint32_t ops = 0;
    uint32_t numblocks = 0;
    Block blocks[10000];
    stivec_t allocbks = create_stivec(10000);

    for (int i = 0; i < 10000; ++i) {
        // Pick random operation
        uint32_t op = randint(2);
        // Nalloc
        if (op == 0) {
            uint32_t size = randint(256);
            printf("[%d] MALLOC(%d)\n", ops, size);
            ++ops;
            blocks[numblocks] = create_block(size);
            stivec_insert(&allocbks, numblocks);
            ++numblocks;
            continue;
        }
        // Pick a random ALLOCATED block -- allocbk
        if (allocbks.size == 0) {
            --i;
            continue;
        }
        uint32_t bkpos = stivec_random_pop(&allocbks);
        Block* bk = &blocks[bkpos];
        // Free
        if (op == 1) {
            printf("[%d] FREE(%zu)\n", ops, (uint32_t)(bk->memptr - (char*)mem_heap_lo()) - sizeof(AllocBlockHeader));
            ++ops;
            free_block(*bk);
        }
        // Realloc
        if (op == 2) {
            stivec_insert(&allocbks, bkpos);
            uint32_t size = randint(256);
            printf("[%d] REALLOC(%zu, %d)\n", ops, (uint32_t)(bk->memptr - (char*)mem_heap_lo()) - sizeof(AllocBlockHeader), size);
            ++ops;
            *bk = realloc_block(*bk, size);
        }
    }

    return true;
}

bool E2E_TESTS() {
    size_t seed = time(NULL);
    srand(seed);

    return (
        E2E_TEST_1(seed)
    );
}

int main() {
    setUp();
    uint32_t heapsize = 1 * 1024 * 1024;
    heap = malloc(heapsize);
    mem_init(heap, heapsize);

    E2E_TESTS();
    return 0;
}
