#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "mm.h"

typedef struct {
    hptr_t block;
} rbtree_t;

/**
 * @param block This block already exists
 */
void rbtree_insert(rbtree_t rbtree, hptr_t block);
/**
 * @brief Returns the element whose key is a upper bound to the passed key
 * @param size Size of user data to find a fit for
 */
hptr_t rbtree_find(rbtree_t rbtree, uint32_t size);

/**
 * @brief Removes the node pointed to by `node` *using successor replacement*
 */
void rbtree_remove(rbtree_t rbtree, hptr_t block);
/* -------------------------------------------------------------------------- */
/*                                   HELPERS                                  */
/* -------------------------------------------------------------------------- */
/* --------------------------- TREE MODIFICATIONS --------------------------- */
/**
 * Links `to_be_child` as a left/right child (according to `left`) of `to_be_parent`
 * @remark Nullifies the `parent` pointer of the left/right child (according to `left`) of `to_be_parent`
 */
void tlink(hptr_t to_be_parent, hptr_t to_be_child, bool left);

/**
 * @brief Swaps two nodes
 */
void swap(hptr_t a, hptr_t b);

/**
 * @brief Left rotates the tree starting at node
 * @returns Pointer to the node that now occupies `node`'s position
 */
hptr_t left_rotate(hptr_t block);

/**
 * @brief Right rotates the tree starting at node
 * @returns Pointer to the node that now occupies `node`'s position
 */
hptr_t right_rotate(hptr_t block);

/* ---------------------------- TREE DATA ACCESS ---------------------------- */
hptr_t root(rbtree_t rbtree);
void set_root(rbtree_t rbtree, hptr_t new_root);

/* ----------------------------- FAMILY MEMBERS ----------------------------- */
hptr_t grandpa(hptr_t block);
hptr_t uncle(hptr_t block);
hptr_t sibling(hptr_t block);

/* ---------------------------- NODE INFORMATION ---------------------------- */
/**
 * @brief Returns whether or not `node` is a left child
 */
bool is_lc(hptr_t block);

/**
 * @returns Smallest node in tree starting at `node`
 */
hptr_t min_node(hptr_t block);

/**
 * @returns Biggest node in tree starting at `block`
 */
hptr_t max_node(hptr_t block);

/* ---------------------------------- SHAPE --------------------------------- */
char check_if_triangle(hptr_t block);
char check_if_line(hptr_t block);

/* -------------------------------- DEBUGGING ------------------------------- */
typedef struct {
    Color color;
    hptr_t left;
    hptr_t right;
    hptr_t parent;
    uint32_t size;
} Node;

extern Node snode(uint32_t size);
extern Node csnode(Color color, uint32_t size);
extern Node node(Color color, uint32_t size, hptr_t left, hptr_t right, hptr_t parent);
extern uint32_t rbtree_to_vec(hptr_t block, Node* result);
