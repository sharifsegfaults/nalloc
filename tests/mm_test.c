#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include "mm.h"

static char *heap;

void setUp() {
    srand(time(NULL));
}

/* -------------------------------------------------------------------------- */
/*                                 UNIT TESTS                                 */
/* -------------------------------------------------------------------------- */
/* ------------------------------ MALLOC TESTS ------------------------------ */
bool TEST_MALLOC()
{
    printf("Starting MALLOC Test...\n");
    mm_init();
    char *a1 = nalloc(23);
    strcpy(a1, "This is 23 bytes long!");
    char *a2 = nalloc(42);
    strcpy(a2, "This is 23 bytes long... Oh wait, it's 42");
    printf("... Finished MALLOC Test\n");
    return true;
}

/* ------------------------------- FREE TESTS ------------------------------- */
bool TEST_FREE_1()
{
    printf("Starting FREE_1 Test...\n");
    mm_init();
    char *a1 = nalloc(23);
    strcpy(a1, "This is 23 bytes long!");
    nfree(a1);
    char *a2 = nalloc(23);
    strcpy(a2, "This is 22 characters!");
    nfree(a2);
    char *a3 = nalloc(27);
    strcpy(a3, "Hello this is 27 bytes :D!");
    nfree(a3);
    char *a4 = nalloc(49);
    strcpy(a4, "This is once again 45 characters or something...");
    nfree(a4);
    printf("... Finished FREE_1 Test\n");

    return true;
}

bool TEST_FREE_2()
{
    printf("Starting FREE_2 Test...\n");
    mm_init();
    // Coalesce right
    char *p1 = nalloc(24);
    char *p2 = nalloc(32);
    char *p3 = nalloc(50);
    strcpy(p1, "24 bytes: aaaaaaaaaaaaa");
    strcpy(p2, "32 bytes: aaaaaaaaaaaaaaaaaaaaa");
    strcpy(p3, "50 bytes: aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    nfree(p3);
    nfree(p2);
    char *p4 = nalloc(82);
    // Coalesce left
    nfree(p1);
    nfree(p4);
    char *p5 = nalloc(106);
    nfree(p5);
    // Coalesce both at the same time
    p1 = nalloc(24);
    p2 = nalloc(48);
    p3 = nalloc(12);
    nfree(p3);
    nfree(p1);
    nfree(p2);
    printf("... Finished FREE_2 Test\n");

    return true;
}

/* ------------------------------ REALLOC TESTS ----------------------------- */
bool TEST_REALLOC_1()
{
    printf("Starting REALLOC_1 Test...\n");
    mm_init();
    char *p1 = nalloc(40);
    char *p2 = nalloc(40);
    char *p3 = nalloc(40);

    strcpy(p1, "This is a 40 byte long message... aa!!!");
    strcpy(p2, "This is a 40 byte long message... bb!!!");
    strcpy(p3, "This is a 40 byte long message... cc!!!");

    nfree(p1);
    nfree(p3);

    char *p2_ = nrealloc(p2, 65);
    assert(p2_ == p2);
    // Expect p2 merged with p3 -- plus some extra space
    strcpy(p2_, "52 bytes are represented in this beautiful string!! and now this");
    nrealloc(p2_, 84);
    printf("... Finished REALLOC_1 Test\n");

    return true;
}

bool TEST_REALLOC_2()
{
    printf("... Starting REALLOC_2 Test\n");

    mm_init();
    char *p1 = nalloc(40);
    char *p2 = nalloc(40);
    char *p3 = nalloc(40);

    strcpy(p1, "This is a 40 byte long message... aa!!!");
    strcpy(p2, "This is a 40 byte long message... bb!!!");
    strcpy(p3, "This is a 40 byte long message... cc!!!");

    nfree(p1);
    char *p2_ = nrealloc(p2, 65);
    assert(p2_ == p1);

    nfree(p3);
    char *p2__ = nrealloc(p2_, 110);
    assert(p2__ == p2_);
    strcpy(p2__, "This is a 40 byte long message... nevermind, it's actually 110... or I think... :) kjashdjkahsjkdhaksdaaaaaaa");

    printf("... Finished REALLOC_2 Test\n");
    return true;
}

bool TEST_REALLOC_SHRINK()
{
    printf("Starting REALLOC_SHRINK Test...\n");
    mm_init();

    char *p1 = nalloc(64);
    strcpy(p1, "Hello everybody, this is 40 bytes long! Nope, it's 64 instead..");
    char *p2 = nalloc(64);
    strcpy(p2, "This is p2's thoughts. the end is near! Nope, it's 64 instead..");

    char *p1_ = nrealloc(p1, 24);
    nfree(p2);

    char *p3 = nalloc(128);
    strcpy(p3, "128 bytes, that's a lot of information... not sure how we are going to fill that up with a single string and allocation... bye!");
    printf("... Finished REALLOC_SHRINK Test\n");
    return true;
}

bool UNIT_TESTS()
{
    return (
        TEST_FREE_1() &&
        TEST_FREE_2() &&
        TEST_REALLOC_1() &&
        TEST_REALLOC_2() &&
        TEST_REALLOC_SHRINK());
}

/* -------------------------------------------------------------------------- */
/*                              END-TO-END TESTS                              */
/* -------------------------------------------------------------------------- */
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
    bk.data = randbytes(size);
    bk.data_size = size;    
    memcpy(bk.memptr, bk.data, size);
    assert(!memcmp(bk.memptr, bk.data, bk.data_size));
    return bk;
}

/* --------------------------------- RANDOM --------------------------------- */
uint32_t randint(uint32_t max) {
    return rand() % (max + 1);
}

bool E2E_TEST_1() {
    mm_init();

    uint32_t numblocks = 0;
    uint32_t freebks = 0;
    Block blocks[10];
    for (int i = 0; i < 10; ++i) {
        // Pick random operation
        uint32_t op = randint(2);
        // Nalloc
        if (op == 0) {
            blocks[numblocks] = create_block(randint(256));
            ++numblocks;
            continue;
        }
        // Pick a random ALLOCATED block -- allocbk
        
        // Free
        if (op == 1) {
            free_block(*bk);
            ++freebks;
        }
        // Realloc
        if (op == 2) {
            *bk = realloc_block(*bk, randint(256));
        }
    }

    return true;
}

bool E2E_TESTS() {
    return (
        E2E_TEST_1()
    );
}

int main() {
    setUp();
    heap = malloc(1 * 1024);
    mem_init(heap, 1 * 1024);

    UNIT_TESTS();
    E2E_TESTS();
    return 0;
}
