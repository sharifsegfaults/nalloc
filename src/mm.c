#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include "mm.h"
#include "rbtree.h"
#include "utils.h"

/* -------------------------------------------------------------------------- */
/*                              STATIC VARIABLES                              */
/* -------------------------------------------------------------------------- */
rbtree_t rbtree;
size_t padding;

/* -------------------------------------------------------------------------- */
/*                           BLOCK MEMBER VARIABLES                           */
/* -------------------------------------------------------------------------- */
FreeBlockHeader* bk_free_header(bptr_t block) {
    assert(block != NULL);
    return (FreeBlockHeader*)block;
}

BlockFooter* bk_footer(bptr_t block) {
    assert(block != NULL);
    // clang-format off
    return (BlockFooter*)(
        (char*)block + sizeof(AllocBlockHeader) + bk_size(block) - sizeof(BlockFooter)
    );
    // clang-format on
}

size_t bk_size(bptr_t block) {
    assert(block != NULL);
    return bk_free_header(block)->__size_prevfree & ~0b11;
}

void bk_set_size(bptr_t block, size_t size) {
    assert(block != NULL);
    assert(size >= MIN_BLOCK_SIZE);
    bk_free_header(block)->__size_prevfree &= 0b11;
    bk_free_header(block)->__size_prevfree |= size;
    bk_footer(block)->size = size;
    assert(bk_size(block) == size);
}

bool bk_prev_free(bptr_t block) {
    assert(block != NULL);
    return bk_free_header(block)->__size_prevfree & 0b10;
}

void bk_set_prev_free(bptr_t block, bool prev_free) {
    assert(block != NULL);
    bk_free_header(block)->__size_prevfree &= ~0b10;
    bk_free_header(block)->__size_prevfree |= prev_free << 1;
    assert(bk_prev_free(block) == prev_free);
}

// size | prev_free | free
bool bk_is_free(bptr_t block) {
    assert(block != NULL);

    // If this is the last block, the first node contains its prev free
    if (next_block(block) == NULL) {
        return bk_prev_free(mem_heap_lo() + padding);
    }
    return bk_prev_free(next_block(block));
}

void bk_set_is_free(bptr_t block, bool is_free) {
    assert(block != NULL);
    dbg_printf(is_free ? "%d block has been freed\n" : "%d block has been occupied\n", block);

    // If this is the last block, the first node contains its prev free
    if (next_block(block) == NULL) {
        bk_set_prev_free(mem_heap_lo() + padding, is_free);
        assert(bk_is_free(block) == is_free);
        return;
    }

    bk_set_prev_free(next_block(block), is_free);
    assert(bk_is_free(block) == is_free);
}

/* ---------------------------- BLOCK NEIGHBOURS ---------------------------- */
bptr_t next_block(bptr_t block) {
    assert(block != NULL);
    if ((uintptr_t)block + sizeof(AllocBlockHeader) + bk_size(block) >= (uintptr_t)mem_heap_hi()) {
        return NULL;
    }
    return (bptr_t)((char*)block + sizeof(AllocBlockHeader) + bk_size(block));
}

/**
 * @brief Returns the previous block if the block is free -- otherwise, returns NULL
 */
bptr_t prev_block_if_free(bptr_t block) {
    // If allocated, user data could've overwritten footer, thus, we can only do this if the
    // previous block is free (and thus metadata has been reestablished)
    if (block == mem_heap_lo() + padding || !bk_prev_free(block)) {
        return NULL;
    }

    size_t prev_block_size = ((BlockFooter*)((char*)block - sizeof(BlockFooter)))->size;
    return (bptr_t)((char*)block - prev_block_size - sizeof(AllocBlockHeader));
}

/* -------------------------------------------------------------------------- */
/*                                 ASSERTIONS                                 */
/* -------------------------------------------------------------------------- */
bool IS_VALID_BLOCK(bptr_t block) {
    // clang-format off
    return (
        block != NULL
        && (uintptr_t)block >= (uintptr_t)mem_heap_lo()
        // Last byte of block is within the heap
        && ((uintptr_t)block + sizeof(AllocBlockHeader) + bk_size(block) - 1) <= (uintptr_t)mem_heap_hi()
        // Whenever block is free, footer is present
        && (bk_is_free(block) ? (bk_size(block) == bk_footer(block)->size) : true)
        && bk_size(block) >= MIN_BLOCK_SIZE
    );
    // clang-format on
}

/* -------------------------------------------------------------------------- */
/*                                  DEBUGGING                                 */
/* -------------------------------------------------------------------------- */
void print_heap() {
    bptr_t curr_block = mem_heap_lo() + padding;
    while (curr_block != NULL) {
        if (bk_is_free(curr_block)) {
            printf("|%zu\t|", bk_size(curr_block));
        } else {
            printf("|(%zu)\t|", bk_size(curr_block));
        }
        curr_block = next_block(curr_block);
    }
    printf("\n");
    curr_block = mem_heap_lo() + padding;
    while (curr_block != NULL) {
        printf("%zu\t", (uintptr_t)curr_block - (uintptr_t)mem_heap_lo());
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
static bptr_t partition_block(bptr_t block, size_t size_needed) {
    assert(IS_VALID_BLOCK(block));
    assert(size_needed >= MIN_BLOCK_SIZE);

    size_needed = ALIGN(sizeof(AllocBlockHeader) + size_needed) - sizeof(AllocBlockHeader);
    size_t total_space = sizeof(AllocBlockHeader) + bk_size(block);
    size_t total_left_space = sizeof(AllocBlockHeader) + size_needed;
    size_t total_right_space = total_space - total_left_space;
    assert(total_right_space >= sizeof(AllocBlockHeader) + MIN_BLOCK_SIZE);

    bptr_t right_bk = (bptr_t)((char*)block + total_left_space);
    /* -------------------------------------------------------------------------- */
    // ! Store the important state you need in variables -- metadata will be corrupted temporarily
    bool is_free_block = bk_is_free(block);

    bk_set_size(right_bk, total_right_space - sizeof(AllocBlockHeader));
    bk_set_prev_free(right_bk, is_free_block);
    bk_set_is_free(right_bk, true);

    // If it's allocated we don't want to override the footer
    if (!is_free_block) {
        BlockFooter user_info_in_new_footer;
        char* new_footer_ptr = (char*)block + sizeof(AllocBlockHeader) + size_needed - sizeof(BlockFooter);
        memcpy(&user_info_in_new_footer, new_footer_ptr, sizeof(BlockFooter));
        bk_set_size(block, size_needed);
        memcpy(bk_footer(block), &user_info_in_new_footer, sizeof(BlockFooter));
    } else {
        bk_set_size(block, size_needed);
    }
    /* -------------------------------------------------------------------------- */
    assert(IS_VALID_BLOCK(block));
    assert(bk_size(block) >= size_needed);
    assert(IS_VALID_BLOCK(right_bk));
    return right_bk;
}

/**
 * @brief Partitions block only if the leftover block is at least PARTITION_THRESHOLD bytes long
 *
 * @returns leftover block. NULL if none.
 */
static bptr_t partition_if_worth_it(bptr_t block, size_t size_needed) {
    assert(IS_VALID_BLOCK(block));

    size_needed = ALIGN(MAX(size_needed, MIN_BLOCK_SIZE));
    size_t block_space = sizeof(AllocBlockHeader) + bk_size(block);
    size_t total_left_space = sizeof(AllocBlockHeader) + size_needed;
    /* -------------------------------------------------------------------------- */
    if (block_space - total_left_space >= PARTITION_THRESHOLD) {
        bptr_t res = partition_block(block, size_needed);
        assert(IS_VALID_BLOCK(block));
        assert(IS_VALID_BLOCK(res));
        return res;
    }
    /* -------------------------------------------------------------------------- */
    assert(IS_VALID_BLOCK(block));
    return NULL;
}

/**
 * @pre block1 is to the left of block2, both are free blocks, and both have been removed
 * from the rbtree
 *
 * @post Only the allocated header of block1 and header and footer of block2 will be modified
 */
void coalesce_blocks(bptr_t block1, bptr_t block2) {
    assert(IS_VALID_BLOCK(block1) && IS_VALID_BLOCK(block2));
    assert(bk_is_free(block1) || bk_is_free(block2));
    assert(next_block(block1) == block2);
    size_t ogsize1 = bk_size(block1);
    size_t ogsize2 = bk_size(block2);
    size_t new_size = bk_size(block1) + sizeof(AllocBlockHeader) + bk_size(block2);
    /* -------------------------------------------------------------------------- */
    dbg_printf("Coalescing %d with %d -- Size of new block is %d\n", block1, block2, new_size);

    bptr_t block_with_my_prev_free = (next_block(block2) != NULL) ? next_block(block2) : mem_heap_lo() + padding;
    bk_set_prev_free(block_with_my_prev_free, bk_is_free(block1));
    bk_set_size(block1, new_size);
    /* -------------------------------------------------------------------------- */
    assert(IS_VALID_BLOCK(block1));
    assert(bk_size(block1) >= ogsize1 + ogsize2);
}

/* -------------------------------------------------------------------------- */
/*                               RED-BLACK TREE                               */
/* -------------------------------------------------------------------------- */
static node_t* rbtree_find(rbtree_t* rbtree, size_t size) {
    node_t* curr_nd = rbtree->root;
    node_t* ub = NULL;

    while (curr_nd != NULL) {
        bptr_t curr_bk = container_of(curr_nd, FreeBlockHeader, rbtree_node);
        if (size <= bk_size(curr_bk)) {
            ub = curr_nd;
            curr_nd = nd_left(curr_nd);
        } else if (size > bk_size(curr_bk)) {
            curr_nd = nd_right(curr_nd);
        }
    }

    return ub;
}

static void rbtree_insert(rbtree_t* rbtree, node_t* node) {
    bptr_t bk = container_of(node, FreeBlockHeader, rbtree_node);

    // Starting metadata
    nd_set_parent(node, NULL);
    nd_set_left(node, NULL);
    nd_set_right(node, NULL);

    if (rbtree->root == NULL) {
        rbtree->root = node;
    } else {
        node_t* curr_nd = rbtree->root;

        while (true) {
            bptr_t curr_bk = container_of(curr_nd, FreeBlockHeader, rbtree_node);
            if (bk_size(bk) < bk_size(curr_bk)) {
                if (nd_left(curr_nd) == NULL) {
                    rbtree_link(curr_nd, node, true);
                    break;
                }
                curr_nd = nd_left(curr_nd);
            } else {
                if (nd_right(curr_nd) == NULL) {
                    rbtree_link(curr_nd, node, false);
                    break;
                }
                curr_nd = nd_right(curr_nd);
            }
        }
    }

    rbtree_insert_fix(rbtree, node);
}

int mm_init() {
    rbtree = create_rbtree();
    mem_reset_brk();
    return 0;
}

void* nalloc(size_t size) {
    // Lazy initialization
    if (rbtree.root == NULL) {
        padding = ALIGN((uintptr_t)mem_heap_lo()) - (uintptr_t)mem_heap_lo();
        void* res = mem_sbrk(padding);
        if (res == (void*)-1) {
            return NULL;
        }
    }
    /* -------------------------------------------------------------------------- */
    size = ALIGN(size);
    dbg_printf("Allocating %zu bytes\n", size);

    node_t* free_node = rbtree_find(&rbtree, size);
    bptr_t free_block = free_node != NULL ? container_of(free_node, FreeBlockHeader, rbtree_node) : NULL;

    if (free_block != NULL) {
        assert(IS_VALID_BLOCK(free_block));

        rbtree_remove(&rbtree, &free_block->rbtree_node);
        bk_set_is_free(free_block, false);

        bptr_t leftover_bk = partition_if_worth_it(free_block, size);
        if (leftover_bk != NULL) {
            assert(IS_VALID_BLOCK(leftover_bk));
            bk_set_is_free(leftover_bk, true);
            rbtree_insert(&rbtree, &leftover_bk->rbtree_node);
            assert(next_block(leftover_bk) == NULL || !bk_is_free(next_block(leftover_bk)));
            assert(prev_block_if_free(leftover_bk) == NULL);
        }

        dbg_printf("[MALLOC]: Request for %zu bytes sufficed with block %d\n", size, free_block);
        assert(IS_VALID_BLOCK(free_block));
        assert(!bk_is_free(free_block));
        assert(bk_size(free_block) >= size);
        assert((uintptr_t)((char*)(free_block) + sizeof(AllocBlockHeader)) % ALIGNMENT == 0);
        print_heap();
        return (char*)free_block + sizeof(AllocBlockHeader);
    }

    // There is no free block to accomodate this request :(
    bool is_last_bk_free = bk_prev_free(mem_heap_lo() + padding);
    bptr_t last_bk = NULL;
    size_t recyclable_space = 0;

    if (is_last_bk_free) {
        size_t last_bk_size = ((BlockFooter*)((char*)mem_heap_hi() - sizeof(BlockFooter) + 1))->size;
        last_bk = mem_heap_hi() - last_bk_size - sizeof(AllocBlockHeader) + 1;
        assert(IS_VALID_BLOCK(last_bk));
        assert(next_block(last_bk) == NULL);
        recyclable_space = sizeof(AllocBlockHeader) + last_bk_size;

        rbtree_remove(&rbtree, &last_bk->rbtree_node);
    }

    // clang-format off
    size_t expansion_size = ALIGN(
        MAX(
            (size_t)(EXPANSION_FACTOR * mem_heapsize()),
            sizeof(AllocBlockHeader) + size - recyclable_space,
            sizeof(AllocBlockHeader) + MIN_BLOCK_SIZE
        )
    );
    // clang-format on

    void* res = mem_sbrk(expansion_size);
    if (res == (void*)-1) {
        return NULL;
    }
    dbg_printf("[MALLOC]: Heap expanded by %d\n", expansion_size);

    // If last block is free, expand into new area. If not, make new area a block
    if (is_last_bk_free) {
        bk_set_size(last_bk, bk_size(last_bk) + expansion_size);
    } else {
        last_bk = mem_heap_hi() - expansion_size + 1;
        bk_set_size(last_bk, expansion_size - sizeof(AllocBlockHeader));
        bk_set_prev_free(last_bk, false);
        assert(IS_VALID_BLOCK(last_bk));
        assert(next_block(last_bk) == NULL);
    }

    bk_set_is_free(last_bk, false);

    bptr_t leftover_bk = partition_if_worth_it(last_bk, size);
    if (leftover_bk != NULL) {
        assert(IS_VALID_BLOCK(leftover_bk));
        bk_set_is_free(leftover_bk, true);
        rbtree_insert(&rbtree, &leftover_bk->rbtree_node);
        assert(next_block(leftover_bk) == NULL || !bk_is_free(next_block(leftover_bk)));
        assert(prev_block_if_free(leftover_bk) == NULL);
    }
    /* -------------------------------------------------------------------------- */
    dbg_printf("[MALLOC]: Request for %zu bytes sufficed with block %d\n", size, last_bk);
    assert(IS_VALID_BLOCK(last_bk));
    assert(!bk_is_free(last_bk));
    assert(bk_size(last_bk) >= size);
    assert((uintptr_t)((char*)(last_bk) + sizeof(AllocBlockHeader)) % ALIGNMENT == 0);
    print_heap();
    return (char*)(last_bk) + sizeof(AllocBlockHeader);
}

void nfree(void* ptr) {
    bptr_t block = (bptr_t)((char*)ptr - sizeof(AllocBlockHeader));
    dbg_printf("Freeing block %d\n", block);
    assert(IS_VALID_BLOCK(block));
    assert(!bk_is_free(block));
    /* -------------------------------------------------------------------------- */
    // Reinstate metadata
    bk_set_size(block, bk_size(block));
    bk_set_is_free(block, true);
    // No need to set prev_free since it is part of size so it hasn't been overwritten

    // Coalescing
    bptr_t nblock = next_block(block);
    bptr_t pblock = prev_block_if_free(block);

    if (nblock != NULL && bk_is_free(nblock)) {
        rbtree_remove(&rbtree, &nblock->rbtree_node);
        coalesce_blocks(block, nblock);
    }

    if (pblock != NULL) {
        rbtree_remove(&rbtree, &pblock->rbtree_node);
        coalesce_blocks(pblock, block);
        block = pblock;
    }

    // Reinsert into rbtree
    bk_set_is_free(block, true);
    rbtree_insert(&rbtree, &block->rbtree_node);
    /* -------------------------------------------------------------------------- */
    assert(bk_is_free(block));
    assert(next_block(block) == NULL || !bk_is_free(next_block(block)));
    assert(!bk_prev_free(block) || prev_block_if_free(block) == NULL);
    print_heap();
}

void* nrealloc(void* ptr, size_t size) {
    size = ALIGN(sizeof(AllocBlockHeader) + size) - sizeof(AllocBlockHeader);

    bptr_t block = (bptr_t)((char*)ptr - sizeof(AllocBlockHeader));
    bptr_t pblock = prev_block_if_free(block);
    bptr_t nblock = next_block(block);
    assert(IS_VALID_BLOCK(block));

    dbg_printf("Reallocating %d -- looking for size %zu\n", block, size);

    size_t space_needed = sizeof(AllocBlockHeader) + size;
    /* -------------------------------------------------------------------------- */
    // Shrinking
    if (bk_size(block) >= size) {
        bptr_t leftover_bk = partition_if_worth_it(block, size);
        if (leftover_bk != NULL) {
            assert(IS_VALID_BLOCK(leftover_bk));
            if (nblock != NULL && bk_is_free(nblock)) {
                rbtree_remove(&rbtree, &nblock->rbtree_node);
                coalesce_blocks(leftover_bk, nblock);
            }
            bk_set_is_free(leftover_bk, true);
            rbtree_insert(&rbtree, &leftover_bk->rbtree_node);
            assert(next_block(leftover_bk) == NULL || !bk_is_free(next_block(leftover_bk)));
            assert(prev_block_if_free(leftover_bk) == NULL);
        }
        /* -------------------------------------------------------------------------- */
        dbg_printf("[REALLOC] Shrinked %d to %d\n", block, bk_size(block));
        assert(IS_VALID_BLOCK(block));
        assert(!bk_is_free(block));
        assert(bk_size(block) >= size);
        assert((uintptr_t)ptr % ALIGNMENT == 0);
        print_heap();
        return ptr;
    }

    // If merging with right is enough...
    if (nblock != NULL && bk_is_free(nblock) && bk_size(block) + sizeof(AllocBlockHeader) + bk_size(nblock) >= size) {
        assert(IS_VALID_BLOCK(nblock));
        /* -------------------------------------------------------------------------- */
        rbtree_remove(&rbtree, &nblock->rbtree_node);
        coalesce_blocks(block, nblock);
        // Partition if necessary
        bptr_t leftover_bk = partition_if_worth_it(block, size);
        if (leftover_bk != NULL) {
            assert(IS_VALID_BLOCK(leftover_bk));
            bk_set_is_free(leftover_bk, true);
            rbtree_insert(&rbtree, &leftover_bk->rbtree_node);
            assert(next_block(leftover_bk) == NULL || !bk_is_free(next_block(leftover_bk)));
            assert(prev_block_if_free(leftover_bk) == NULL);
        }
        /* -------------------------------------------------------------------------- */
        dbg_printf("[REALLOC] Just expanded block. New size: %d\n", bk_size(block));
        assert(IS_VALID_BLOCK(block));
        assert(!bk_is_free(block));
        assert(bk_size(block) >= size);
        assert((uintptr_t)((char*)(block) + sizeof(AllocBlockHeader)) % ALIGNMENT == 0);
        print_heap();
        return (char*)(block) + sizeof(AllocBlockHeader);
    }

    // If merging with left is enough...
    if (pblock != NULL && bk_is_free(pblock) && bk_size(pblock) + sizeof(AllocBlockHeader) + bk_size(block) >= size) {
        assert(IS_VALID_BLOCK(pblock));
        /* -------------------------------------------------------------------------- */
        size_t prev_size = bk_size(block);

        // Merge the two together -- coalescing may overwrite footer bytes, so store them
        rbtree_remove(&rbtree, &pblock->rbtree_node);
        BlockFooter user_info_in_footer;
        memcpy(&user_info_in_footer, bk_footer(block), sizeof(BlockFooter));
        coalesce_blocks(pblock, block);
        bk_set_is_free(pblock, false);

        // Copy the user's info
        char* block_uptr = (char*)(block) + sizeof(AllocBlockHeader);
        char* pblock_uptr = (char*)(pblock) + sizeof(AllocBlockHeader);
        memmove(pblock_uptr, block_uptr, prev_size);
        memcpy(pblock_uptr + prev_size - sizeof(BlockFooter), &user_info_in_footer, sizeof(BlockFooter));

        // Partition if necessary
        bptr_t leftover_bk = partition_if_worth_it(pblock, size);
        if (leftover_bk != NULL) {
            assert(IS_VALID_BLOCK(leftover_bk));
            // Check if we need to coalesce with right block
            if (nblock != NULL && bk_is_free(nblock)) {
                rbtree_remove(&rbtree, &nblock->rbtree_node);
                coalesce_blocks(leftover_bk, nblock);
            }
            bk_set_is_free(leftover_bk, true);
            rbtree_insert(&rbtree, &leftover_bk->rbtree_node);
            assert(next_block(leftover_bk) == NULL || !bk_is_free(next_block(leftover_bk)));
            assert(prev_block_if_free(leftover_bk) == NULL);
        }
        /* -------------------------------------------------------------------------- */
        dbg_printf("[REALLOC] Merged %d with %d and reallocated -- New size is: %d\n", block, pblock, bk_size(pblock));
        assert(IS_VALID_BLOCK(pblock));
        assert(!bk_is_free(pblock));
        assert(bk_size(pblock) >= size);
        assert((uintptr_t)pblock_uptr % ALIGNMENT == 0);
        print_heap();
        return pblock_uptr;
    }

    // If merging with both our left and our right is enough...
    // PS: This is cheaper -- we'd already know we can fit here (thus we save an rbtree search)
    if (pblock != NULL && nblock != NULL && bk_is_free(pblock) && bk_is_free(nblock) &&
        bk_size(pblock) + 2 * sizeof(AllocBlockHeader) + bk_size(block) + bk_size(nblock) >= size) {
        assert(IS_VALID_BLOCK(pblock));
        assert(IS_VALID_BLOCK(nblock));
        /* -------------------------------------------------------------------------- */
        size_t prev_size = bk_size(block);

        rbtree_remove(&rbtree, &pblock->rbtree_node);
        rbtree_remove(&rbtree, &nblock->rbtree_node);
        // ! Order matters -- allows us to avoid footer overwrite of `block`
        coalesce_blocks(block, nblock);
        coalesce_blocks(pblock, block);
        bk_set_is_free(pblock, false);

        bptr_t leftover_bk = partition_if_worth_it(pblock, size);
        if (leftover_bk != NULL) {
            assert(IS_VALID_BLOCK(leftover_bk));
            bk_set_is_free(leftover_bk, true);
            rbtree_insert(&rbtree, &leftover_bk->rbtree_node);
            assert(next_block(leftover_bk) == NULL || !bk_is_free(next_block(leftover_bk)));
            assert(prev_block_if_free(leftover_bk) == NULL);
        }

        char* block_uptr = (char*)(block) + sizeof(AllocBlockHeader);
        char* pblock_uptr = (char*)(pblock) + sizeof(AllocBlockHeader);
        memmove(pblock_uptr, block_uptr, prev_size);
        /* -------------------------------------------------------------------------- */
        dbg_printf("[REALLOC] Merged %d, %d, and %d -- New size: %d\n", pblock, block, nblock, bk_size(pblock));
        assert(IS_VALID_BLOCK(pblock));
        assert(!bk_is_free(pblock));
        assert(bk_size(pblock) >= size);
        assert((uintptr_t)((char*)(pblock) + sizeof(AllocBlockHeader)) % ALIGNMENT == 0);
        print_heap();
        return (char*)(pblock) + sizeof(AllocBlockHeader);
    }

    // If nothing worked... We just do the usual
    char* new_ptr = nalloc(size);
    if (new_ptr != NULL) {
        memcpy(new_ptr, ptr, bk_size(block));
        nfree(ptr);
    }
    /* -------------------------------------------------------------------------- */
    dbg_printf("[REALLOC] Reallocated %d to %lu\n", block,
               (uintptr_t)new_ptr - (uintptr_t)mem_heap_lo() - sizeof(AllocBlockHeader));
    assert((uintptr_t)new_ptr % ALIGNMENT == 0);
    return new_ptr;
}
