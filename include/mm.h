#pragma once

#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "memlib.h"

#include "rbtree.h"

#define ALIGNMENT (alignof(max_align_t))
#define ALIGN(addr) ((addr + ALIGNMENT - 1) & ~(ALIGNMENT - 1))

#define EXPANSION_FACTOR 0.35
#define PARTITION_THRESHOLD 20
#define MIN_BLOCK_SIZE (ALIGN(sizeof(FreeBlockHeader) + sizeof(BlockFooter) - sizeof(AllocBlockHeader)))

#ifdef DEBUG
#define dbg_printf printf
#else
#define dbg_printf(...)
#endif

typedef struct {
    _Alignas(ALIGNMENT) uint32_t __spfc;
} AllocBlockHeader;

// Thank you NegVorsa!
typedef struct {
    uint32_t __size_prevfree;
    node_t rbtree_node;
} FreeBlockHeader;

typedef struct {
    uint32_t size;
} BlockFooter;

typedef FreeBlockHeader* bptr_t;

/* ------------------------- BLOCK MEMBER VARIABLES ------------------------- */

/**
 * @brief Stores the size of the block if it were to be provided for allocation
 *
 * @pre Assumes header is well-formed
 *
 * @remark Obtains size from block's header (not footer)
 */
extern uint32_t bk_size(bptr_t block);
extern void bk_set_size(bptr_t block, uint32_t size);

extern bool bk_prev_free(bptr_t block);
extern void bk_set_prev_free(bptr_t block, bool prev_free);

extern bool bk_is_free(bptr_t block);
extern void bk_set_is_free(bptr_t block, bool is_free);

/* ----------------------------- BLOCK NEIGHBOURS ---------------------------- */
extern bptr_t next_block(bptr_t block);
extern bptr_t prev_block_if_free(bptr_t block);

/* -------------------------------------------------------------------------- */
/*                               MAIN FUNCTIONS                               */
/* -------------------------------------------------------------------------- */

extern int mm_init(void);

/**
 * @brief Allocate `size` bytes
 *
 * @returns Pointer to memory section containing at least `size` modifiable bytes
 */
extern void* nalloc(size_t size);

/**
 * @brief Frees a previously allocated memory block.
 *
 * @param ptr  Pointer to block to be freed. Must be a pointer returned by a previous call to nalloc
 */
extern void nfree(void* ptr);

/**
 * @brief Reallocates the information in memory block pointed to by `ptr` to
 * a new memory block of size at least `size`
 *
 * @param size Size of the new memory block
 *
 * @returns Pointer to the new memory block
 */
extern void* nrealloc(void* ptr, size_t size);

/* -------------------------------- DEBUGGING ------------------------------- */
void print_heap();
