#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include "mm.h"
#include "utils.h"
#include "containers/stivec.h"
#include <getopt.h>

static char *heap;

/* -------------------------------------------------------------------------- */
/*                                    UTILS                                   */
/* -------------------------------------------------------------------------- */
uint32_t curr_seed = 1;

uint32_t randint(uint32_t max) {
    return rand() % (max + 1);
}
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

/* -------------------------------------------------------------------------- */
/*                             OPERATION TRACKERS                             */
/* -------------------------------------------------------------------------- */
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

/* -------------------------------------------------------------------------- */
/*                                   FUZZER                                   */
/* -------------------------------------------------------------------------- */
bool fuzzer(size_t seed, uint32_t num_ops) {
    mm_init();

    uint32_t ops = 0;
    uint32_t numblocks = 0;
    Block blocks[num_ops];
    stivec_t allocbks = create_stivec(num_ops);

    for (int i = 0; i < num_ops; ++i) {
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

void setUp(size_t seed, size_t max_heap_size) {
    srand(seed);

    heap = malloc(max_heap_size);
    mem_init(heap, max_heap_size);
}

typedef struct {
    size_t seed;
    size_t max_heap_size;
    size_t numops;
} Options;

void help(char* command) {
    printf(
        "Usage: %s [--(s)eed <seed number>]\n"
        "[--(m)axheap <max heap size>]\n"
        "[--(n)umops <number of operations>]"
    , command);
}

void get_options(int argc, char** argv, Options* options) {
    opterr = false;
    int choice;
    int index = 0;

    struct option long_options[] = {
        {"help"     , no_argument       , NULL  , 'h'},

        {"seed"     , optional_argument , NULL  , 's'},
        {"maxheap"  , optional_argument , NULL  , 'm'},
        {"numops"   , optional_argument , NULL  , 'n'},
        {0          , 0                 , 0     , 0  }
    };

    while((choice = getopt_long(argc, argv, "hs::m::n::", long_options, &index)) != -1) {
        switch (choice) {
        case 'h':
            help(*argv);
            exit(0);
            break;

        case 's': {
            if (optarg == NULL && optind < argc && argv[optind][0] != '-') {
                optarg = argv[optind++];
            }
            size_t arg = (size_t)optarg;
            options->seed = arg;
            break;
        }

        case 'm': {
            if (optarg == NULL && optind < argc && argv[optind][0] != '-') {
                optarg = argv[optind++];
            }
            size_t arg = (size_t)optarg;
            options->max_heap_size = arg;
            break;
        }

        case 'n': {
            if (optarg == NULL && optind < argc && argv[optind][0] != '-') {
                optarg = argv[optind++];
            }
            char* endptr;
            size_t arg = strtoull(optarg, &endptr, 10);
            options->numops = arg;
            break;
        }

        default:
            printf("Unknown option\n");
            exit(1);
            break;
        }
    }
}

void print_options(Options* options) {
    printf(
        "==[ NFUZZER ]====================\n"
        "Seed: %zu\n"
        "Max heap size: %zu\n"
        "Number of operations: %zu\n"
        "=================================\n"
    , options->seed, options->max_heap_size, options->numops);
}

int main(int argc, char *argv[]) {
    Options options = {
        // Defaults
        time(NULL),
        1 * 1024 * 1024,
        100
    };

    get_options(argc, argv, &options);
    print_options(&options);
    setUp(options.seed, options.max_heap_size);

    fuzzer(options.seed, options.numops);
    return 0;
}
