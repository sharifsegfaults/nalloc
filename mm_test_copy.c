#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "unity.h"
#include "mm.h"

static char* heap;

void setUp() {}
void tearDown() {}

bool TEST_PARTITION_BLOCK() {
    bk_set_size(0, 124);
    partition_block(0, 24);
    
    return true;
}

bool TEST_MALLOC() {
    mm_init();
    char* a1 = nalloc(23);
    strcpy(a1, "This is 23 bytes long!");
    char* a2 = nalloc(42);
    strcpy(a2, "This is 23 bytes long... Oh wait, it's 42");
    
    return true;
}

bool TEST_FREE_1() {
    mm_init();
    char* a1 = nalloc(23);
    strcpy(a1, "This is 23 bytes long!");
    mm_free(a1);
    char* a2 = nalloc(23);
    strcpy(a2, "This is 22 characters!");
    mm_free(a2);
    char* a3 = nalloc(27);
    strcpy(a3, "Hello this is 27 bytes :D!");
    mm_free(a3);
    char* a4 = nalloc(49);
    strcpy(a4, "This is once again 45 characters or something...");
    mm_free(a4);

    return true;
}

// Coalescing
bool TEST_FREE_2() {
    mm_init();
    // Coalesce right
    char* p1 = nalloc(24);
    char* p2 = nalloc(32);
    char* p3 = nalloc(50);
    strcpy(p1, "24 bytes: aaaaaaaaaaaaa");
    strcpy(p2, "32 bytes: aaaaaaaaaaaaaaaaaaaaa");
    strcpy(p3, "50 bytes: aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    mm_free(p3);
    mm_free(p2);
    char* p4 = nalloc(82);
    // Coalesce left
    mm_free(p1);
    mm_free(p4);
    char* p5 = nalloc(106);
    mm_free(p5);
    // Coalesce both at the same time
    p1 = nalloc(24);
    p2 = nalloc(48);
    p3 = nalloc(12);
    mm_free(p3);
    mm_free(p1);
    mm_free(p2);

    return true;
}

// Test for realloc: realloc on a scenario where there is enough space, but only once the block is freed
bool TEST_REALLOC_1() {
    mm_init();
    char* p1 = nalloc(40);
    char* p2 = nalloc(40);
    char* p3 = nalloc(40);

    strcpy(p1, "This is a 40 byte long message... aa!!!");
    strcpy(p2, "This is a 40 byte long message... bb!!!");
    strcpy(p3, "This is a 40 byte long message... cc!!!");

    mm_free(p1);
    mm_free(p3);

    char* p2_ = mm_realloc(p2, 65);
    assert(p2_ == p2);
    // Expect p2 merged with p3 -- plus some extra space
    strcpy(p2_, "52 bytes are represented in this beautiful string!! and now this");
    mm_realloc(p2_, 84);

    return true;
}

bool TEST_REALLOC_2() {
    mm_init();
    char* p1 = nalloc(40);
    char* p2 = nalloc(40);
    char* p3 = nalloc(40);

    strcpy(p1, "This is a 40 byte long message... aa!!!");
    strcpy(p2, "This is a 40 byte long message... bb!!!");
    strcpy(p3, "This is a 40 byte long message... cc!!!");

    mm_free(p1);
    char* p2_ = mm_realloc(p2, 65);
    assert(p2_ == p1);

    mm_free(p3);
    char* p2__ = mm_realloc(p2_, 110);
    assert(p2__ == p2_);
    strcpy(p2__, "This is a 40 byte long message... nevermind, it's actually 110... or I think... :) kjashdjkahsjkdhaksdaaaaaaa");

    return true;
}

int main() {
    heap = malloc(1 * 1024);
    mem_init(heap, 1 * 1024);

    TEST_FREE_1();
    TEST_FREE_2();
    TEST_REALLOC_1();
    TEST_REALLOC_2();
    return 0;
}
