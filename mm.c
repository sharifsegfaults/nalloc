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

// Have ghost node point to the root of out RB Tree
/* -------------------------------------------------------------------------- */
/*                           BLOCK MEMBER VARIABLES                           */
/* -------------------------------------------------------------------------- */
FreeBlockHeader* bk_free_header(hptr_t block) {
    return((FreeBlockHeader*)((char*)mem_heap_lo() + block));
}

BlockFooter* bk_footer(hptr_t block) {
    return (BlockFooter*)((char*)mem_heap_lo() + block + sizeof(AllocBlockHeader) + bk_size(block)
                          - sizeof(BlockFooter));
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
}

void bk_set_size(hptr_t block, uint32_t size) {
    assert(block != NULL_HPTR);
    bk_free_header(block)->__spfc &= 0b11;
    bk_free_header(block)->__spfc |= size;
    bk_footer(block)->size = size;
}

hptr_t bk_left(hptr_t block) {
    assert(block != NULL_HPTR);
    return bk_free_header(block)->left;
}

void bk_set_left(hptr_t block, hptr_t left) {
    assert(block != NULL_HPTR);
    bk_free_header(block)->left = left;
}

hptr_t bk_right(hptr_t block) {
    assert(block != NULL_HPTR);
    return bk_free_header(block)->right;
}

void bk_set_right(hptr_t block, hptr_t right) {
    assert(block != NULL_HPTR);
    bk_free_header(block)->right = right;
}

hptr_t bk_parent(hptr_t block) {
    assert(block != NULL_HPTR);
    return bk_free_header(block)->parent;
}

void bk_set_parent(hptr_t block, hptr_t parent) {
    assert(block != NULL_HPTR);
    bk_free_header(block)->parent = parent;
}

Color bk_color(hptr_t block) {
    return (block == NULL_HPTR) ? BLACK : bk_free_header(block)->__spfc & 0b1;
}

void bk_set_color(hptr_t block, Color color) {
    assert(block != NULL_HPTR);
    bk_free_header(block)->__spfc &= ~0b1;
    bk_free_header(block)->__spfc |= (hptr_t)color;
}

// size | prev_free | free

bool bk_is_free(hptr_t block) {
    assert(block != NULL_HPTR);
    hptr_t nblock = next_block(block) == NULL_HPTR ? rbtree.block : next_block(block);
    return bk_prev_free(nblock);
}

void bk_set_is_free(hptr_t block, bool is_free) {
    assert(block != NULL_HPTR);
    dbg_printf(is_free ? "%d block has been freed" : "%d block has been occupied");
    
    // If this is the last block, the ghost node contains its prev free
    if (next_block(block) == NULL_HPTR) {
        bk_set_prev_free(rbtree.block, is_free);
        return;
    }

    bk_set_prev_free(next_block(block), is_free);
}

/* ---------------------------- BLOCK NEIGHBOURS ---------------------------- */
hptr_t next_block(hptr_t block){
    if (block + sizeof(AllocBlockHeader) + bk_size(block) >= mem_heapsize()) {
        return NULL_HPTR;
    }
    return block + sizeof(AllocBlockHeader) + bk_size(block);
}

hptr_t prev_block(hptr_t block) {
    if (block == next_block(rbtree.block)) {
        return NULL_HPTR;
    }

    uint32_t prev_block_size = ((BlockFooter*)((char*)mem_heap_lo() + block - sizeof(BlockFooter)))->size;
    return block - prev_block_size - sizeof(AllocBlockHeader);
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
    
    assert(size_needed >= 16);
    size_needed = ALIGN(sizeof(AllocBlockHeader) + size_needed) - sizeof(AllocBlockHeader);
    
    uint32_t total_space = sizeof(AllocBlockHeader) + bk_size(block);
    uint32_t total_left_space = sizeof(AllocBlockHeader) + size_needed;
    uint32_t total_right_space = total_space - total_left_space;
    dbg_printf("Partitioning block %d to left size of %d. Remaining free block of size: %d",
                block, size_needed, total_right_space - sizeof(AllocBlockHeader));
    
    assert(total_right_space >= sizeof(FreeBlockHeader) + sizeof(BlockFooter));

    hptr_t right_bk = block + total_left_space;\

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
        char* new_footer_ptr = (char*)mem_heap_lo() + block + sizeof(AllocBlockHeader) + size_needed - sizeof(BlockFooter);
        memcpy(&user_info_in_new_footer, new_footer_ptr, sizeof(BlockFooter));
        bk_set_size(block, size_needed);
        memcpy(bk_footer(block), &user_info_in_new_footer, sizeof(BlockFooter));
    } else {
        bk_set_size(block, size_needed);
    }

    return right_bk;
}

hptr_t partition_if_worth_it(hptr_t block, uint32_t size_needed) {
    uint32_t block_space = sizeof(AllocBlockHeader) + bk_size(block);
    size_needed = MAX(size_needed, 16);
    size_needed = ALIGN(sizeof(AllocBlockHeader) + size_needed) - sizeof(AllocBlockHeader);
    uint32_t total_left_space = sizeof(AllocBlockHeader) + size_needed;

    // If the remaining block can host a 16-byte allocation, let it live
    if (block_space - total_left_space >= PARTITION_THRESHOLD) {
        hptr_t res = partition_block(block, size_needed);
        return res;
    }

    return NULL_HPTR;
}

// ! Parts of the program depend on this thing overwriting only the header and footer of both blocks
/**
 * @pre block1 is to the left of block2, both are free blocks, and both have been removed
 * from the rbtree
 */
void coalesce_blocks(hptr_t block1, hptr_t block2) {
    assert(bk_is_free(block1) || bk_is_free(block2));
    assert(next_block(block1) == block2);

    uint32_t new_size = bk_size(block1) + sizeof(AllocBlockHeader) + bk_size(block2);

    dbg_printf("Coalescing %d with %d -- Size of new block is %d", block1, block2, new_size);

    if (next_block(block2) != NULL_HPTR) {
        bk_set_prev_free(next_block(block2), bk_is_free(block1));
    } else {
        bk_set_prev_free(rbtree.block, bk_is_free(block1));
    }

    bk_set_size(block1, new_size);
}

int mm_init() {
    rbtree.block = NULL_HPTR;
    mem_reset_brk();
    return 0;
}

void* nalloc(size_t size) {
    // Lazy initialization
    if (rbtree.block == NULL_HPTR) {
        uint32_t padding = ALIGN((uintptr_t)mem_heap_lo()) - (uintptr_t)mem_heap_lo();
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
        bk_set_size(rbtree.block, ghost_node_size - sizeof(AllocBlockHeader)); // ! Ghost node must still be represented by a valid node
        bk_set_prev_free(rbtree.block, false);
    }

    size = ALIGN(sizeof(AllocBlockHeader) + size) - sizeof(AllocBlockHeader);

    dbg_printf("Allocating %d bytes", size);

    hptr_t free_block = rbtree_find(rbtree, size);

    if (free_block != NULL_HPTR) {
        rbtree_remove(rbtree, free_block);
        bk_set_is_free(free_block, false);
        hptr_t leftover_bk = partition_if_worth_it(free_block, size);
        if (leftover_bk != NULL_HPTR) {
            bk_set_is_free(leftover_bk, true);
            rbtree_insert(rbtree, leftover_bk);
        }
        dbg_printf("[MALLOC]: Request for %d bytes sufficed with block %d", size, free_block);
        return (char*)mem_heap_lo() + free_block + sizeof(AllocBlockHeader);
    }

    bool is_last_bk_free = bk_prev_free(rbtree.block);
    hptr_t last_bk = NULL_HPTR;
    uint32_t recyclable_space = 0;

    if (is_last_bk_free) {
        uint32_t last_bk_size = ((BlockFooter*)((char*)mem_heap_hi() - 3))->size;
        last_bk = mem_heapsize() - last_bk_size - sizeof(AllocBlockHeader);
        recyclable_space = sizeof(AllocBlockHeader) + last_bk_size;
        
        rbtree_remove(rbtree, last_bk);
    }

    // There is no free block :(
    uint32_t expansion_size = ALIGN(MAX(
        MAX((uint32_t)(EXPANSION_FACTOR * mem_heapsize()), sizeof(AllocBlockHeader) + size - recyclable_space),
        sizeof(FreeBlockHeader) + sizeof(BlockFooter)
    ));

    void* res = mem_sbrk(expansion_size);
    if (res == (void*)-1) {
        return NULL;
    }
    dbg_printf("[MALLOC]: Heap expanded by %d", expansion_size);

    if (is_last_bk_free) {
        bk_set_size(last_bk, bk_size(last_bk) + expansion_size);
    } else {
        // Convert expanded area into a block (last block, by definition)
        last_bk = mem_heapsize() - expansion_size;
        bk_set_size(last_bk, expansion_size - sizeof(AllocBlockHeader));
        bk_set_prev_free(last_bk, false);
    }

    bk_set_is_free(last_bk, false);
    hptr_t leftover_bk = partition_if_worth_it(last_bk, size);
    if (leftover_bk != NULL_HPTR) {
        bk_set_is_free(leftover_bk, true);
        rbtree_insert(rbtree, leftover_bk);
    }

    dbg_printf("[MALLOC]: Request for %d bytes sufficed with block %d", size, last_bk);
    return (char*)mem_heap_lo() + last_bk + sizeof(AllocBlockHeader);
}

void mm_free(void* ptr) {
    
    // We need to reinstate metadata
    hptr_t block = (uintptr_t)((char*)ptr - sizeof(AllocBlockHeader)) - (uintptr_t)mem_heap_lo();
    dbg_printf("Freeing block %d", block);

    assert(!bk_is_free(block));
    bk_set_size(block, bk_size(block));
    bk_set_is_free(block, true);
    // * No need to set prev_free since it is part of size

    // Coalescing
    // ! Order matters
    hptr_t nblock = next_block(block);

    if (nblock != NULL_HPTR && bk_is_free(nblock)) {
        rbtree_remove(rbtree, nblock);
        coalesce_blocks(block, nblock);
    }
    if (bk_prev_free(block)) {
        hptr_t pblock = prev_block(block);
        rbtree_remove(rbtree, pblock);
        coalesce_blocks(pblock, block);
        block = pblock;
    }

    // Reinsert into rbtree
    bk_set_is_free(block, true);
    rbtree_insert(rbtree, block);
}

void* mm_realloc(void* ptr, size_t size) {
    size = ALIGN(sizeof(AllocBlockHeader) + size) - sizeof(AllocBlockHeader);

    hptr_t block = ((uintptr_t)ptr - (uintptr_t)mem_heap_lo()) - sizeof(AllocBlockHeader);
    hptr_t pblock = prev_block(block);
    hptr_t nblock = next_block(block);

    dbg_printf("Reallocating %d -- looking for size %d", block, size);

    uint32_t space_needed = sizeof(AllocBlockHeader) + size;

    /* -------------------------------- SHRINKING ------------------------------- */
    if (bk_size(block) >= size) {
        hptr_t leftover_bk = partition_if_worth_it(block, size);
        if (leftover_bk != NULL_HPTR) {
            if (nblock != NULL_HPTR && bk_is_free(nblock)) {
                rbtree_remove(rbtree, nblock);
                coalesce_blocks(leftover_bk, nblock);
            }
            bk_set_is_free(leftover_bk, true);
            rbtree_insert(rbtree, leftover_bk);
        }
        dbg_printf("[REALLOC] Shrinked %d to %d", block, bk_size(block));
        return ptr;
    }

    /* -------------------------------- EXPANDING ------------------------------- */
    if (nblock != NULL_HPTR && bk_is_free(nblock) && bk_size(block) + sizeof(AllocBlockHeader) + bk_size(nblock) >= size) {
        // Merge the two together
        rbtree_remove(rbtree, nblock);
        coalesce_blocks(block, nblock);
        // Partition if necessary
        hptr_t leftover_bk =  partition_if_worth_it(block, size);
        if (leftover_bk != NULL_HPTR) {
            bk_set_is_free(leftover_bk, true);
            rbtree_insert(rbtree, leftover_bk);
        }

        dbg_printf("[REALLOC] Just expanded block. New size: %d", bk_size(block));
        return (char*)mem_heap_lo() + block + sizeof(AllocBlockHeader);
    }

    if (pblock != NULL_HPTR && bk_is_free(pblock) && bk_size(pblock) + sizeof(AllocBlockHeader) + bk_size(block) >= size) {
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
            // Check if we need to coalesce with right block
            if (bk_is_free(nblock)) {
                rbtree_remove(rbtree, nblock);
                bk_set_is_free(block, true);
                coalesce_blocks(leftover_bk, nblock);
            }
            bk_set_is_free(leftover_bk, true);
            rbtree_insert(rbtree, leftover_bk);
        }

        dbg_printf("[REALLOC] Merged %d with %d and reallocated -- New size is: %d", block, pblock, bk_size(pblock));
        return pblock_uptr;
    }

    if (pblock != NULL_HPTR && nblock != NULL_HPTR
    && bk_is_free(pblock) && bk_is_free(nblock)
    && bk_size(pblock) + 2*sizeof(AllocBlockHeader) + bk_size(block) + bk_size(nblock) >= size) {
        uint32_t prev_size = bk_size(block);

        rbtree_remove(rbtree, pblock);
        rbtree_remove(rbtree, nblock);
        // ! Order matters -- allows us to avoid footer overwrite of `block`
        coalesce_blocks(block, nblock);
        coalesce_blocks(pblock, block);

        hptr_t leftover_bk = partition_if_worth_it(pblock, size);
        if (leftover_bk != NULL_HPTR) {
            bk_set_is_free(leftover_bk, true);
            rbtree_insert(rbtree, leftover_bk);
        }

        char* block_uptr = (char*)mem_heap_lo() + block + sizeof(AllocBlockHeader);
        char* pblock_uptr = (char*)mem_heap_lo() + pblock + sizeof(AllocBlockHeader);
        memmove(pblock_uptr, block_uptr, prev_size);
        bk_set_is_free(pblock, false);

        dbg_printf("[REALLOC] Merged %d, %d, and %d -- New size: %d", pblock, block, nblock, bk_size(pblock));
        return (char*)mem_heap_lo() + pblock + sizeof(AllocBlockHeader);
    }

    // If nothing worked... We just do the usual
    char* new_ptr = nalloc(size);
    if (new_ptr != NULL) {
        memcpy(new_ptr, ptr, bk_size(block));
        mm_free(ptr);
    }

    dbg_printf("[REALLOC] Reallocated %d to %d",
                block, (uintptr_t)mem_heap_lo() - (uintptr_t)new_ptr - sizeof(AllocBlockHeader));
    return new_ptr;
}
