#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>

typedef enum : uint8_t { RED = 0, BLACK = 1 } Color;

typedef struct node_t {
    struct node_t* left;
    struct node_t* right;
    struct node_t* __pc;
} node_t;

typedef struct {
    node_t* root;
} rbtree_t;

/* -------------------------------------------------------------------------- */
/*                            NODE MEMBER VARIABLES                           */
/* -------------------------------------------------------------------------- */
static inline node_t* nd_left(node_t* node) {
    assert(node != NULL);
    return node->left;
}

static inline void nd_set_left(node_t* node, node_t* left) {
    assert(node != NULL);
    node->left = left;
}

static inline node_t* nd_right(node_t* node) {
    assert(node != NULL);
    return node->right;
}

static inline void nd_set_right(node_t* node, node_t* right) {
    assert(node != NULL);
    node->right = right;
}

static inline node_t* nd_parent(node_t* node) {
    assert(node != NULL);
    return (node_t*)((uintptr_t)(node->__pc) & ~0b1UL);
}

static inline void nd_set_parent(node_t* node, node_t* parent) {
    assert(node != NULL);
    node->__pc = (node_t*)((uintptr_t)(node->__pc) & 0b1);
    node->__pc = (node_t*)((uintptr_t)(node->__pc) | (uintptr_t)parent);
}

static inline Color nd_color(node_t* node) {
    return (node != NULL) ? (uintptr_t)node->__pc & 0b1 : BLACK;
}

static inline void nd_set_color(node_t* node, Color color) {
    assert(node != NULL);
    node->__pc = (node_t*)((uintptr_t)node->__pc & ~0b1);
    node->__pc = (node_t*)((uintptr_t)node->__pc | color);
}

/* -------------------------------------------------------------------------- */
/*                               RBTREE METHODS                               */
/* -------------------------------------------------------------------------- */
static inline rbtree_t create_rbtree() {
    return (rbtree_t){ NULL };
}

/**
 * @pre Assumes `node` is linked to the correct place if this were a binary search tree
 */
void rbtree_insert_fix(rbtree_t* rbtree, node_t* node);

/**
 * @brief Removes the node pointed to by `node` *using successor replacement*
 */
void rbtree_remove(rbtree_t* rbtree, node_t* node);
/* -------------------------------------------------------------------------- */
/*                                   HELPERS                                  */
/* -------------------------------------------------------------------------- */
/* --------------------------- TREE MODIFICATIONS --------------------------- */
/**
 * Links `to_be_child` as a left/right child (according to `left`) of `to_be_parent`
 * @remark Nullifies the `parent` pointer of the left/right child (according to `left`) of `to_be_parent`
 */
void rbtree_link(node_t* to_be_parent, node_t* to_be_child, bool left);

/**
 * @brief Swaps two nodes
 */
void swap(rbtree_t* rbtree, node_t* a, node_t* b);

/**
 * @brief Left rotates the tree starting at node
 * @returns Pointer to the node that now occupies `node`'s position
 */
node_t* left_rotate(rbtree_t* rbtree, node_t* node);

/**
 * @brief Right rotates the tree starting at node
 * @returns Pointer to the node that now occupies `node`'s position
 */
node_t* right_rotate(rbtree_t* rbtree, node_t* node);

/* ---------------------------- NODE INFORMATION ---------------------------- */
/**
 * @brief Returns whether or not `node` is a left child
 */
static inline bool is_lc(node_t* node) {
    assert(nd_parent(node) != NULL);
    return nd_left(nd_parent(node)) == node;
}

/**
 * @returns Smallest node in tree starting at `node`
 */
node_t* min_node(node_t* node);

/**
 * @returns Biggest node in tree starting at `node`
 */
node_t* max_node(node_t* node);

/* ----------------------------- FAMILY MEMBERS ----------------------------- */
static inline node_t* grandpa(node_t* node) {
    return nd_parent(nd_parent(node));
}

static inline node_t* uncle(node_t* node) {
    return is_lc(nd_parent(node)) ? nd_right(grandpa(node)) : nd_left(grandpa(node));
}

static inline node_t* sibling(node_t* node) {
    return is_lc(node) ? nd_right(nd_parent(node)) : nd_left(nd_parent(node));
}

/* ---------------------------------- SHAPE --------------------------------- */
char check_if_triangle(node_t* node);
char check_if_line(node_t* node);

/* -------------------------------- DEBUGGING ------------------------------- */
size_t rbtree_to_vec(node_t* rbtree, node_t* result[]);
