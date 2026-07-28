#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef enum : uint8_t {
    RED = 0,
    BLACK = 1
} Color;

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
extern node_t* nd_left(node_t* node);
extern void nd_set_left(node_t* node, node_t* left);

extern node_t* nd_right(node_t* node);
extern void nd_set_right(node_t* node, node_t* right);

extern node_t* nd_parent(node_t* node);
extern void nd_set_parent(node_t* node, node_t* parent);

extern Color nd_color(node_t* node);
extern void nd_set_color(node_t* node, Color color);

/* -------------------------------------------------------------------------- */
/*                               RBTREE METHODS                               */
/* -------------------------------------------------------------------------- */
/**
 * @param node This node already exists
 */
void rbtree_insert(rbtree_t* rbtree, node_t* node);

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
void tlink(node_t* to_be_parent, node_t* to_be_child, bool left);

/**
 * @brief Swaps two nodes
 */
void swap(node_t* a, node_t* b);

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

/* ----------------------------- FAMILY MEMBERS ----------------------------- */
node_t* grandpa(node_t* node);
node_t* uncle(node_t* node);
node_t* sibling(node_t* node);

/* ---------------------------- NODE INFORMATION ---------------------------- */
/**
 * @brief Returns whether or not `node` is a left child
 */
bool is_lc(node_t* node);

/**
 * @returns Smallest node in tree starting at `node`
 */
node_t* min_node(node_t* node);

/**
 * @returns Biggest node in tree starting at `node`
 */
node_t* max_node(node_t* node);

/* ---------------------------------- SHAPE --------------------------------- */
char check_if_triangle(node_t* node);
char check_if_line(node_t* node);

/* -------------------------------- DEBUGGING ------------------------------- */
extern uint32_t rbtree_to_vec(rbtree_t* rbtree, node_t result[]);
