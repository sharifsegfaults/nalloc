#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "unity.h"
#include "mm.h"

#define S(x) (ALIGN(sizeof(uint32_t) + (x)) - sizeof(uint32_t))

static char* heap;

void setUp() {
    heap = malloc(1 * 1024);
    mem_init(heap, 1 * 1024);
}

void tearDown() {}

// TODO: Test the bk_* functions

typedef struct {
    uint32_t size;
    bool free;
    char* ptr;
    char* expected_data;
} Block;

uint32_t blockId = 1;

char* randstr(uint32_t size) {
    // Generate random string
    char* str = malloc(size);
    for (int i = 0; i < size; ++i) {
        str[i] = blockId;
    }
    ++blockId;
    return str;
}

Block bnalloc(uint32_t size) {
    char* str = randstr(size);
    Block block = {size, false, nalloc(size), str};
    strcpy(block.ptr, str);
    return block;
}

void bfree(Block b) {
    b.free = true;
    mm_free(b.ptr);
}

void EXPECT_BLOCK(char* ptr, uint32_t size, bool is_free) {
    hptr_t block = (uintptr_t)ptr - (uintptr_t)mem_heap_lo() - sizeof(uint32_t);
    TEST_ASSERT_EQUAL(bk_size(block), size);
    TEST_ASSERT_EQUAL(bk_is_free(block), is_free);
}

void EXPECT_HEAP(Block blocks[], uint32_t n) {
    // ! Not accounting for padding ~~~v
    uint32_t curr_bk = ALIGN(sizeof(BlockHeader) + sizeof(BlockFooter)); // Everything starts after ghost node
    for (uint32_t i = 0; i < n; i++) {
        TEST_ASSERT_EQUAL(bk_size(curr_bk), blocks[i].size);
        TEST_ASSERT_EQUAL(bk_is_free(curr_bk), blocks[i].free);
        TEST_ASSERT_EQUAL_STRING(
            (char*)mem_heap_lo() + curr_bk + sizeof(uint32_t),
            blocks[i].expected_data
        );
        curr_bk = next_block(curr_bk);
    }
}

void TEST_MALLOC() {
    mm_init();
    Block b1 = bnalloc(S(23));
    Block b2 = bnalloc(S(53));
    Block b3 = bnalloc(S(12));

    bfree(b2);

    // ...

    EXPECT_HEAP((Block[2]){
        {}
    }, 2);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(TEST_MALLOC);
    return UNITY_END();
    return 0;
}
