#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include "common.h"
#include "mm.h"
#include "rbtree.h"

/* -------------------------------------------------------------------------- */
/*                              STATIC VARIABLES                              */
/* -------------------------------------------------------------------------- */
rbtree_t rbtree;
uint32_t padding;

// Have ghost node point to the root of out RB Tree
/* -------------------------------------------------------------------------- */
/*                           BLOCK MEMBER VARIABLES                           */
/* -------------------------------------------------------------------------- */
FreeBlockHeader* bk_free_header(hptr_t block) {
    assert(block != NULL_HPTR);
    return ((FreeBlockHeader*)((char*)mem_heap_lo() + block));
}

BlockFooter* bk_footer(hptr_t block) {
    assert(block != NULL_HPTR);
    return (BlockFooter*)((char*)mem_heap_lo() + block + sizeof(AllocBlockHeader) + bk_size(block) -
                          sizeof(BlockFooter));
}

uint32_t bk_size(hptr_t block) {
    assert(block != NULL_HPTR);
    return bk_free_header(block)->__spfc & ~0b11;
}

bool bk_prev_free(hptr_t block) {
    assert(block != NULL_HPTR);
    return bk_free_header(block)->__spfc & 0b10;
}

void bk_set_prev_free(hptr_t block, bool prev_free) {
    assert(block != NULL_HPTR);
    bk_free_header(block)->__spfc &= ~0b10;
    bk_free_header(block)->__spfc |= prev_free << 1;
    assert(bk_prev_free(block) == prev_free);
}

void bk_set_size(hptr_t block, uint32_t size) {
    assert(block != NULL_HPTR);
    assert(size >= MIN_BLOCK_SIZE);
    bk_free_header(block)->__spfc &= 0b11;
    bk_free_header(block)->__spfc |= size;
    bk_footer(block)->size = size;
    assert(bk_size(block) == size);
}

hptr_t bk_left(hptr_t block) {
    assert(block != NULL_HPTR);
    return bk_free_header(block)->left;
}

void bk_set_left(hptr_t block, hptr_t left) {
    assert(block != NULL_HPTR);
    bk_free_header(block)->left = left;
    assert(bk_left(block) == left);
}

hptr_t bk_right(hptr_t block) {
    assert(block != NULL_HPTR);
    return bk_free_header(block)->right;
}

void bk_set_right(hptr_t block, hptr_t right) {
    assert(block != NULL_HPTR);
    bk_free_header(block)->right = right;
    assert(bk_right(block) == right);
}

hptr_t bk_parent(hptr_t block) {
    assert(block != NULL_HPTR);
    return bk_free_header(block)->parent;
}

void bk_set_parent(hptr_t block, hptr_t parent) {
    assert(block != NULL_HPTR);
    bk_free_header(block)->parent = parent;
    assert(bk_parent(block) == parent);
}

Color bk_color(hptr_t block) {
    return (block == NULL_HPTR) ? BLACK : bk_free_header(block)->__spfc & 0b1;
}

void bk_set_color(hptr_t block, Color color) {
    assert(block != NULL_HPTR);
    bk_free_header(block)->__spfc &= ~0b1;
    bk_free_header(block)->__spfc |= (hptr_t)color;
    assert(bk_color(block) == color);
}

// size | prev_free | free

bool bk_is_free(hptr_t block) {
    assert(block != NULL_HPTR);
    hptr_t nblock = next_block(block) == NULL_HPTR ? rbtree.block : next_block(block);
    return bk_prev_free(nblock);
}

void bk_set_is_free(hptr_t block, bool is_free) {
    assert(block != NULL_HPTR);
    dbg_printf(is_free ? "%d block has been freed\n" : "%d block has been occupied\n", block);

    // If this is the last block, the ghost node contains its prev free
    if (next_block(block) == NULL_HPTR) {
        bk_set_prev_free(rbtree.block, is_free);
        assert(bk_is_free(block) == is_free);
        return;
    }

    bk_set_prev_free(next_block(block), is_free);
    assert(bk_is_free(block) == is_free);
}

/* ---------------------------- BLOCK NEIGHBOURS ---------------------------- */
hptr_t next_block(hptr_t block) {
    assert(block != NULL_HPTR);
    if (block + sizeof(AllocBlockHeader) + bk_size(block) >= mem_heapsize()) {
        return NULL_HPTR;
    }
    return block + sizeof(AllocBlockHeader) + bk_size(block);
}

/**
 * @brief Returns the previous block if the block is free -- otherwise, returns NULL_HPTR
 */
hptr_t prev_block_if_free(hptr_t block) {
    // If allocated, user data could've overwritten footer,
    // thus, we can only do this if the previous block is free
    // (and thus metadata has been reestablished)
    if (block == next_block(rbtree.block) || !bk_prev_free(block)) {
        return NULL_HPTR;
    }

    uint32_t prev_block_size = ((BlockFooter*)((char*)mem_heap_lo() + block - sizeof(BlockFooter)))->size;
    return block - prev_block_size - sizeof(AllocBlockHeader);
}

/* -------------------------------------------------------------------------- */
/*                                 ASSERTIONS                                 */
/* -------------------------------------------------------------------------- */
bool IS_VALID_BLOCK(hptr_t block) {
    return (block != NULL_HPTR && block + sizeof(AllocBlockHeader) + bk_size(block) - 1 < mem_heapsize() &&
            (bk_is_free(block) ? (bk_size(block) == bk_footer(block)->size) : true) &&
            bk_size(block) >= MIN_BLOCK_SIZE);
}

/* -------------------------------------------------------------------------- */
/*                                  DEBUGGING                                 */
/* -------------------------------------------------------------------------- */
void print_heap() {
    hptr_t curr_block = padding;
    // Skip ghost node
    curr_block = next_block(curr_block);
    while (curr_block != NULL_HPTR) {
        if (bk_is_free(curr_block)) {
            printf("|%d\t|", bk_size(curr_block));
        } else {
            printf("|(%d)\t|", bk_size(curr_block));
        }
        curr_block = next_block(curr_block);
    }
    printf("\n");
    curr_block = padding;
    curr_block = next_block(curr_block);
    while (curr_block != NULL_HPTR) {
        printf("%d\t", curr_block);
        curr_block = next_block(curr_block);
    }
    printf("\n");
    curr_block = padding;
    curr_block = next_block(curr_block);
    while (curr_block != NULL_HPTR) {
        printf("%d(%d)\t", bk_is_free(curr_block) ? bk_parent(curr_block) : 0, (uint8_t)bk_color(curr_block));
        curr_block = next_block(curr_block);
    }
    printf("\n");
}

/* -------------------------------------------------------------------------- */
/*                             BLOCK MANIPULATION                             */
/* -------------------------------------------------------------------------- */

/**
 * @pre Block can actually accomodate for the requested partition
 *
 * @param size_needed Size needed in "user size" (i.e., do not include metadata)
 *
 * @remark This function corrupts the rbtree metadata
 */
hptr_t partition_block(hptr_t block, uint32_t size_needed) {
    assert(IS_VALID_BLOCK(block));
    assert(size_needed >= MIN_BLOCK_SIZE);

    size_needed = ALIGN(sizeof(AllocBlockHeader) + size_needed) - sizeof(AllocBlockHeader);

    uint32_t total_space = sizeof(AllocBlockHeader) + bk_size(block);
    uint32_t total_left_space = sizeof(AllocBlockHeader) + size_needed;
    uint32_t total_right_space = total_space - total_left_space;
    dbg_printf("Partitioning block %d to left size of %d. Remaining free block of size: %lu\n", block, size_needed,
               total_right_space - sizeof(AllocBlockHeader));

    assert(total_right_space >= sizeof(FreeBlockHeader) + sizeof(BlockFooter));

    hptr_t right_bk = block + total_left_space;

    // ! Try to store the important state you need in variables because all of the block-moving
    // ! and block manipulation changes temporarily metadata (like prev_free of some block could
    // ! temporarily be pointing at the wrong thing)
    bool is_free_block = bk_is_free(block);

    bk_set_size(right_bk, total_right_space - sizeof(AllocBlockHeader));
    bk_set_prev_free(right_bk, is_free_block);
    bk_set_is_free(right_bk, true);
    // If it's allocated we don't want to override the footer
    if (!is_free_block) {
        BlockFooter user_info_in_new_footer;
        char* new_footer_ptr =
            (char*)mem_heap_lo() + block + sizeof(AllocBlockHeader) + size_needed - sizeof(BlockFooter);
        memcpy(&user_info_in_new_footer, new_footer_ptr, sizeof(BlockFooter));
        bk_set_size(block, size_needed);
        memcpy(bk_footer(block), &user_info_in_new_footer, sizeof(BlockFooter));
    } else {
        bk_set_size(block, size_needed);
    }

    assert(IS_VALID_BLOCK(block));
    assert(bk_size(block) >= size_needed);
    assert(IS_VALID_BLOCK(right_bk));

    return right_bk;
}

hptr_t partition_if_worth_it(hptr_t block, uint32_t size_needed) {
    assert(IS_VALID_BLOCK(block));

    uint32_t block_space = sizeof(AllocBlockHeader) + bk_size(block);
    size_needed = MAX(size_needed, MIN_BLOCK_SIZE);
    size_needed = ALIGN(sizeof(AllocBlockHeader) + size_needed) - sizeof(AllocBlockHeader);
    uint32_t total_left_space = sizeof(AllocBlockHeader) + size_needed;

    if (block_space - total_left_space >= PARTITION_THRESHOLD) {
        hptr_t res = partition_block(block, size_needed);

        assert(IS_VALID_BLOCK(block));
        assert(IS_VALID_BLOCK(res));
        return res;
    }

    assert(IS_VALID_BLOCK(block));
    return NULL_HPTR;
}

// ! Parts of the program depend on this thing overwriting only the header and footer of both blocks
/**
 * @pre block1 is to the left of block2, both are free blocks, and both have been removed
 * from the rbtree
 */
void coalesce_blocks(hptr_t block1, hptr_t block2) {
    assert(IS_VALID_BLOCK(block1) && IS_VALID_BLOCK(block2));
    assert(bk_is_free(block1) || bk_is_free(block2));
    assert(next_block(block1) == block2);
    uint32_t ogsize1 = bk_size(block1);
    uint32_t ogsize2 = bk_size(block2);
    /* -------------------------------------------------------------------------- */
    uint32_t new_size = bk_size(block1) + sizeof(AllocBlockHeader) + bk_size(block2);

    dbg_printf("Coalescing %d with %d -- Size of new block is %d\n", block1, block2, new_size);

    if (next_block(block2) != NULL_HPTR) {
        bk_set_prev_free(next_block(block2), bk_is_free(block1));
    } else {
        bk_set_prev_free(rbtree.block, bk_is_free(block1));
    }

    bk_set_size(block1, new_size);
    /* -------------------------------------------------------------------------- */
    assert(IS_VALID_BLOCK(block1));
    assert(bk_size(block1) >= ogsize1 + ogsize2);
}

int mm_init() {
    rbtree.block = NULL_HPTR;
    mem_reset_brk();
    return 0;
}

void* nalloc(size_t size) {
    // Lazy initialization
    if (rbtree.block == NULL_HPTR) {
        padding = ALIGN((uintptr_t)mem_heap_lo()) - (uintptr_t)mem_heap_lo();
        uint32_t ghost_node_size = ALIGN(sizeof(FreeBlockHeader) + sizeof(BlockFooter));
        void* res = mem_sbrk(padding + ghost_node_size);
        if (res == (void*)-1) {
            return NULL;
        }
        // Setup ghost node
        // ! Other parts of the program (prev_block()) rely on the ghost node being the first node
        // ! (in terms of heap arrangement)
        rbtree.block = padding;
        bk_set_left(rbtree.block, NULL_HPTR);
        bk_set_size(
            rbtree.block,
            ghost_node_size - sizeof(AllocBlockHeader));  // ! Ghost node must still be represented by a valid node
        bk_set_prev_free(rbtree.block, false);
    }

    assert(rbtree.block == padding);
    /* -------------------------------------------------------------------------- */
    size = ALIGN(sizeof(AllocBlockHeader) + size) - sizeof(AllocBlockHeader);

    dbg_printf("Allocating %zu bytes\n", size);

    hptr_t free_block = rbtree_find(rbtree, size);

    if (free_block != NULL_HPTR) {
        assert(IS_VALID_BLOCK(free_block));

        rbtree_remove(rbtree, free_block);
        bk_set_is_free(free_block, false);
        hptr_t leftover_bk = partition_if_worth_it(free_block, size);

        if (leftover_bk != NULL_HPTR) {
            assert(IS_VALID_BLOCK(leftover_bk));
            bk_set_is_free(leftover_bk, true);
            rbtree_insert(rbtree, leftover_bk);
            assert(next_block(leftover_bk) == NULL_HPTR || !bk_is_free(next_block(leftover_bk)));
            assert(prev_block_if_free(leftover_bk) == NULL_HPTR);
        }

        dbg_printf("[MALLOC]: Request for %zu bytes sufficed with block %d\n", size, free_block);
        assert(IS_VALID_BLOCK(free_block));
        assert(!bk_is_free(free_block));
        assert(bk_size(free_block) >= size);
        print_heap();
        return (char*)mem_heap_lo() + free_block + sizeof(AllocBlockHeader);
    }

    bool is_last_bk_free = bk_prev_free(rbtree.block);
    hptr_t last_bk = NULL_HPTR;
    uint32_t recyclable_space = 0;

    if (is_last_bk_free) {
        uint32_t last_bk_size = ((BlockFooter*)((char*)mem_heap_hi() - 3))->size;
        last_bk = mem_heapsize() - last_bk_size - sizeof(AllocBlockHeader);
        assert(IS_VALID_BLOCK(last_bk));
        assert(next_block(last_bk) == NULL_HPTR);
        recyclable_space = sizeof(AllocBlockHeader) + last_bk_size;

        rbtree_remove(rbtree, last_bk);
    }

    // There is no free block :(
    uint32_t expansion_size = ALIGN(
        MAX(MAX((uint32_t)(EXPANSION_FACTOR * mem_heapsize()), sizeof(AllocBlockHeader) + size - recyclable_space),
            sizeof(FreeBlockHeader) + sizeof(BlockFooter)));

    void* res = mem_sbrk(expansion_size);
    if (res == (void*)-1) {
        return NULL;
    }
    dbg_printf("[MALLOC]: Heap expanded by %d\n", expansion_size);

    if (is_last_bk_free) {
        bk_set_size(last_bk, bk_size(last_bk) + expansion_size);
    } else {
        // Convert expanded area into a block (last block, by definition)
        last_bk = mem_heapsize() - expansion_size;
        bk_set_size(last_bk, expansion_size - sizeof(AllocBlockHeader));
        bk_set_prev_free(last_bk, false);
        assert(IS_VALID_BLOCK(last_bk));
        assert(next_block(last_bk) == NULL_HPTR);
    }

    bk_set_is_free(last_bk, false);
    hptr_t leftover_bk = partition_if_worth_it(last_bk, size);
    if (leftover_bk != NULL_HPTR) {
        assert(IS_VALID_BLOCK(leftover_bk));
        bk_set_is_free(leftover_bk, true);
        rbtree_insert(rbtree, leftover_bk);
        assert(next_block(leftover_bk) == NULL_HPTR || !bk_is_free(next_block(leftover_bk)));
        assert(prev_block_if_free(leftover_bk) == NULL_HPTR);
    }

    dbg_printf("[MALLOC]: Request for %zu bytes sufficed with block %d\n", size, last_bk);
    assert(IS_VALID_BLOCK(last_bk));
    assert(!bk_is_free(last_bk));
    assert(bk_size(last_bk) >= size);
    print_heap();
    return (char*)mem_heap_lo() + last_bk + sizeof(AllocBlockHeader);
}

void nfree(void* ptr) {
    // We need to reinstate metadata
    hptr_t block = (uintptr_t)((char*)ptr - sizeof(AllocBlockHeader)) - (uintptr_t)mem_heap_lo();
    dbg_printf("Freeing block %d\n", block);
    assert(IS_VALID_BLOCK(block));
    assert(!bk_is_free(block));

    bk_set_size(block, bk_size(block));
    bk_set_is_free(block, true);
    // * No need to set prev_free since it is part of size so it hasn't been overwritten

    // Coalescing
    // ! Order matters
    // ! DO NOT USE prev_block UNLESS YOU KNOW THAT THE PREVIOUS BLOCK IS FREE
    hptr_t nblock = next_block(block);
    hptr_t pblock = prev_block_if_free(block);

    if (nblock != NULL_HPTR && bk_is_free(nblock)) {
        rbtree_remove(rbtree, nblock);
        coalesce_blocks(block, nblock);
    }

    if (pblock != NULL_HPTR) {
        rbtree_remove(rbtree, pblock);
        coalesce_blocks(pblock, block);
        block = pblock;
    }

    // Reinsert into rbtree
    bk_set_is_free(block, true);
    rbtree_insert(rbtree, block);
    /* -------------------------------------------------------------------------- */
    assert(bk_is_free(block));
    assert(next_block(block) == NULL_HPTR || !bk_is_free(next_block(block)));
    assert(!bk_prev_free(block) || prev_block_if_free(block) == NULL_HPTR);
    print_heap();
}

void* nrealloc(void* ptr, size_t size) {
    size = ALIGN(sizeof(AllocBlockHeader) + size) - sizeof(AllocBlockHeader);

    hptr_t block = ((uintptr_t)ptr - (uintptr_t)mem_heap_lo()) - sizeof(AllocBlockHeader);
    hptr_t pblock = prev_block_if_free(block);
    hptr_t nblock = next_block(block);

    assert(IS_VALID_BLOCK(block));
    dbg_printf("Reallocating %d -- looking for size %zu\n", block, size);

    uint32_t space_needed = sizeof(AllocBlockHeader) + size;

    /* -------------------------------- SHRINKING ------------------------------- */
    if (bk_size(block) >= size) {
        hptr_t leftover_bk = partition_if_worth_it(block, size);
        if (leftover_bk != NULL_HPTR) {
            assert(IS_VALID_BLOCK(leftover_bk));
            if (nblock != NULL_HPTR && bk_is_free(nblock)) {
                rbtree_remove(rbtree, nblock);
                coalesce_blocks(leftover_bk, nblock);
            }
            bk_set_is_free(leftover_bk, true);
            rbtree_insert(rbtree, leftover_bk);
            assert(next_block(leftover_bk) == NULL_HPTR || !bk_is_free(next_block(leftover_bk)));
            assert(prev_block_if_free(leftover_bk) == NULL_HPTR);
        }
        dbg_printf("[REALLOC] Shrinked %d to %d\n", block, bk_size(block));
        assert(IS_VALID_BLOCK(block));
        assert(!bk_is_free(block));
        assert(bk_size(block) >= size);
        print_heap();
        return ptr;
    }

    /* -------------------------------- EXPANDING ------------------------------- */
    if (nblock != NULL_HPTR && bk_is_free(nblock) &&
        bk_size(block) + sizeof(AllocBlockHeader) + bk_size(nblock) >= size) {
        assert(IS_VALID_BLOCK(nblock));
        /* -------------------------------------------------------------------------- */
        // Merge the two together
        rbtree_remove(rbtree, nblock);
        coalesce_blocks(block, nblock);
        // Partition if necessary
        hptr_t leftover_bk = partition_if_worth_it(block, size);
        if (leftover_bk != NULL_HPTR) {
            assert(IS_VALID_BLOCK(leftover_bk));
            bk_set_is_free(leftover_bk, true);
            rbtree_insert(rbtree, leftover_bk);
            assert(next_block(leftover_bk) == NULL_HPTR || !bk_is_free(next_block(leftover_bk)));
            assert(prev_block_if_free(leftover_bk) == NULL_HPTR);
        }

        dbg_printf("[REALLOC] Just expanded block. New size: %d\n", bk_size(block));
        assert(IS_VALID_BLOCK(block));
        assert(!bk_is_free(block));
        assert(bk_size(block) >= size);
        print_heap();
        return (char*)mem_heap_lo() + block + sizeof(AllocBlockHeader);
    }

    if (pblock != NULL_HPTR && bk_is_free(pblock) &&
        bk_size(pblock) + sizeof(AllocBlockHeader) + bk_size(block) >= size) {
        assert(IS_VALID_BLOCK(pblock));
        /* -------------------------------------------------------------------------- */
        uint32_t prev_size = bk_size(block);
        // Merge the two together
        rbtree_remove(rbtree, pblock);
        BlockFooter user_info_in_footer;
        memcpy(&user_info_in_footer, bk_footer(block), sizeof(BlockFooter));
        coalesce_blocks(pblock, block);
        bk_set_is_free(pblock, false);

        // Copy the user's info
        char* block_uptr = (char*)mem_heap_lo() + block + sizeof(AllocBlockHeader);
        char* pblock_uptr = (char*)mem_heap_lo() + pblock + sizeof(AllocBlockHeader);
        memmove(pblock_uptr, block_uptr, prev_size);
        memcpy(pblock_uptr + prev_size - sizeof(BlockFooter), &user_info_in_footer, sizeof(BlockFooter));

        // Partition if necessary
        hptr_t leftover_bk = partition_if_worth_it(pblock, size);
        if (leftover_bk != NULL_HPTR) {
            assert(IS_VALID_BLOCK(leftover_bk));
            // Check if we need to coalesce with right block
            if (bk_is_free(nblock)) {
                rbtree_remove(rbtree, nblock);
                coalesce_blocks(leftover_bk, nblock);
            }
            bk_set_is_free(leftover_bk, true);
            rbtree_insert(rbtree, leftover_bk);
            assert(next_block(leftover_bk) == NULL_HPTR || !bk_is_free(next_block(leftover_bk)));
            assert(prev_block_if_free(leftover_bk) == NULL_HPTR);
        }

        dbg_printf("[REALLOC] Merged %d with %d and reallocated -- New size is: %d\n", block, pblock, bk_size(pblock));
        assert(IS_VALID_BLOCK(pblock));
        assert(!bk_is_free(pblock));
        assert(bk_size(pblock) >= size);
        print_heap();
        return pblock_uptr;
    }

    if (pblock != NULL_HPTR && nblock != NULL_HPTR && bk_is_free(pblock) && bk_is_free(nblock) &&
        bk_size(pblock) + 2 * sizeof(AllocBlockHeader) + bk_size(block) + bk_size(nblock) >= size) {
        assert(IS_VALID_BLOCK(pblock));
        assert(IS_VALID_BLOCK(nblock));
        /* -------------------------------------------------------------------------- */
        uint32_t prev_size = bk_size(block);

        rbtree_remove(rbtree, pblock);
        rbtree_remove(rbtree, nblock);
        // ! Order matters -- allows us to avoid footer overwrite of `block`
        coalesce_blocks(block, nblock);
        coalesce_blocks(pblock, block);

        hptr_t leftover_bk = partition_if_worth_it(pblock, size);
        if (leftover_bk != NULL_HPTR) {
            assert(IS_VALID_BLOCK(leftover_bk));
            bk_set_is_free(leftover_bk, true);
            rbtree_insert(rbtree, leftover_bk);
            assert(next_block(leftover_bk) == NULL_HPTR || !bk_is_free(next_block(leftover_bk)));
            assert(prev_block_if_free(leftover_bk) == NULL_HPTR);
        }

        char* block_uptr = (char*)mem_heap_lo() + block + sizeof(AllocBlockHeader);
        char* pblock_uptr = (char*)mem_heap_lo() + pblock + sizeof(AllocBlockHeader);
        memmove(pblock_uptr, block_uptr, prev_size);
        bk_set_is_free(pblock, false);

        dbg_printf("[REALLOC] Merged %d, %d, and %d -- New size: %d\n", pblock, block, nblock, bk_size(pblock));
        assert(IS_VALID_BLOCK(pblock));
        assert(!bk_is_free(pblock));
        assert(bk_size(pblock) >= size);
        print_heap();
        return (char*)mem_heap_lo() + pblock + sizeof(AllocBlockHeader);
    }

    // If nothing worked... We just do the usual
    char* new_ptr = nalloc(size);
    if (new_ptr != NULL) {
        memcpy(new_ptr, ptr, bk_size(block));
        nfree(ptr);
    }

    dbg_printf("[REALLOC] Reallocated %d to %lu\n", block,
               (uintptr_t)new_ptr - (uintptr_t)mem_heap_lo() - sizeof(AllocBlockHeader));
    return new_ptr;
}
