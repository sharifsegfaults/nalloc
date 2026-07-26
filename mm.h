#pragma once

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdalign.h>
#include <stddef.h>

#include "memlib.h"

typedef uint32_t hptr_t;
#define NULL_HPTR UINT32_MAX

#define ALIGNMENT alignof(max_align_t)
#define ALIGN(addr) ((addr + ALIGNMENT - 1) & ~(ALIGNMENT-1))
#define EXPANSION_FACTOR 0.35
#define PARTITION_THRESHOLD 20

#ifdef DEBUG
#define dbg_printf printf
#else
#define dbg_printf(...)
#endif

typedef enum : uint8_t {
    RED = 0,
    BLACK = 1
} Color;

typedef struct {
    _Alignas(ALIGNMENT) uint32_t __spfc;
} AllocBlockHeader;

// Thank you NegVorsa!
typedef struct {
    uint32_t __spfc;
    hptr_t left;
    hptr_t right;
    hptr_t parent;
} FreeBlockHeader;

typedef struct {
    uint32_t size;
} BlockFooter;

/* ------------------------- BLOCK MEMBER VARIABLES ------------------------- */

/**
 * @brief Stores the size of the block if it were to be provided for allocation
 * 
 * @pre Assumes header is well-formed
 * 
 * @remark Obtains size from block's header (not footer)
 */
extern uint32_t bk_size(hptr_t block);
extern void bk_set_size(hptr_t block, uint32_t size);

extern bool bk_prev_free(hptr_t block);
extern void bk_set_prev_free(hptr_t block, bool prev_free);

extern hptr_t bk_left(hptr_t block);
extern void bk_set_left(hptr_t block, hptr_t left);

extern hptr_t bk_right(hptr_t block);
extern void bk_set_right(hptr_t block, hptr_t right);

extern hptr_t bk_parent(hptr_t block);
extern void bk_set_parent(hptr_t block, hptr_t parent);

extern Color bk_color(hptr_t block);
extern void bk_set_color(hptr_t block, Color color);

extern bool bk_is_free(hptr_t block);
extern void bk_set_is_free(hptr_t block, bool is_free);

/* ----------------------------- BLOCK NEIGHBOURS ---------------------------- */
extern hptr_t next_block(hptr_t block);
extern hptr_t prev_block(hptr_t block);

/* ---------------------------------- TEMP ---------------------------------- */
extern hptr_t partition_block(hptr_t block, uint32_t size_needed);

/* -------------------------------------------------------------------------- */
/*                               MAIN FUNCTIONS                               */
/* -------------------------------------------------------------------------- */

extern int mm_init (void);

/**
 * @brief Allocate `size` bytes
 * 
 * @returns Pointer to memory section containing at least `size` modifiable bytes
 */
extern void *nalloc (size_t size);

/**
 * @brief Frees a previously allocated memory block.
 * 
 * @param ptr  Pointer to block to be freed. Must be a pointer returned by a previous call to nalloc
 */
extern void mm_free (void *ptr);

/**
 * @brief Reallocates the information in memory block pointed to by `ptr` to
 * a new memory block of size at least `size`
 * 
 * @param size Size of the new memory block
 * 
 * @returns Pointer to the new memory block
 */
extern void *mm_realloc(void *ptr, size_t size);
