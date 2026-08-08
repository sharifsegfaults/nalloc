#pragma once

#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "memlib.h"

#include "containers/rbtree.h"

#define ALIGNMENT (alignof(max_align_t))
#define ALIGN(addr) ((addr + ALIGNMENT - 1) & ~(ALIGNMENT - 1))

#define EXPANSION_FACTOR 0.35
#define MIN_BLOCK_SIZE (ALIGN(sizeof(FreeBlockHeader) + sizeof(BlockFooter) - sizeof(AllocBlockHeader)))
#define PARTITION_THRESHOLD (sizeof(AllocBlockHeader) + MIN_BLOCK_SIZE)

#ifdef DEBUG
#define dbg_printf printf
#else
#define dbg_printf(...)
#endif

typedef struct {
    _Alignas(ALIGNMENT) size_t __spfc;
} AllocBlockHeader;

// Thank you NegVorsa!
typedef struct {
    size_t __size_prevfree;
    node_t rbtree_node;
} FreeBlockHeader;

typedef struct {
    size_t size;
} BlockFooter;

typedef FreeBlockHeader* bptr_t;

/* -------------------------------------------------------------------------- */
/*                               MAIN FUNCTIONS                               */
/* -------------------------------------------------------------------------- */
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
