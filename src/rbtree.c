#include <assert.h>
#include <stddef.h>

#include "rbtree.h"

/* -------------------------------------------------------------------------- */
/*                            NODE MEMBER VARIABLES                           */
/* -------------------------------------------------------------------------- */
node_t* nd_left(node_t* node) {
    assert(node != NULL);
    return node->left;
}

void nd_set_left(node_t* node, node_t* left) {
    assert(node != NULL);
    node->left = left;
}

node_t* nd_right(node_t* node) {
    assert(node != NULL);
    return node->right;
}

void nd_set_right(node_t* node, node_t* right) {
    assert(node != NULL);
    node->right = right;
}

node_t* nd_parent(node_t* node) {
    assert(node != NULL);
    return (node_t*)((uintptr_t)(node->__pc) & ~0b1UL);
}

void nd_set_parent(node_t* node, node_t* parent) {
    assert(node != NULL);
    node->__pc = (node_t*)((uintptr_t)(node->__pc) & 0b1);
    node->__pc = (node_t*)((uintptr_t)(node->__pc) | (uintptr_t)parent);
}

Color nd_color(node_t* node) {
    return (node != NULL) ? (uintptr_t)node->__pc & 0b1 : BLACK;
}

void nd_set_color(node_t* node, Color color) {
    assert(node != NULL);
    node->__pc = (node_t*)((uintptr_t)node->__pc & ~0b1);
    node->__pc = (node_t*)((uintptr_t)node->__pc | color);
}

/* -------------------------------------------------------------------------- */
/*                               RBTREE METHODS                               */
/* -------------------------------------------------------------------------- */
rbtree_t create_rbtree() {
    return (rbtree_t){ NULL };
}

/**
 * @pre Assumes `node` is linked to the correct place as if this were a binary search tree
 */
void rbtree_insert_fix(rbtree_t* rbtree, node_t* node) {
    // Navigate down the tree until you find the insertion spot
    // Starting metadata
    nd_set_left(node, NULL);
    nd_set_right(node, NULL);
    nd_set_color(node, RED);
    node_t* curr = nd_parent(node);

    if (rbtree->root == node) {
        nd_set_color(node, BLACK);
        return;
    }

    if (nd_color(curr) == BLACK) {
        return;
    }

    // Every time we solve an error, we make the current node
    // pose as the newly inserted node, and make our parent
    // be the current node. We re-run this until we reach the root.

    while (node != NULL) {
        // Case 0: new node is root --> make it black
        if (rbtree->root == node) {
            nd_set_color(node, BLACK);
            break;
        }

        // If there are no problems, we just continue :)
        if (nd_color(node) != RED || nd_color(curr) != RED) {
            node = curr;
            curr = nd_parent(curr);
            continue;
        }

        // Case 1: node's uncle is red
        if (nd_color(uncle(node)) == RED) {
            nd_set_color(grandpa(node), RED);
            nd_set_color(uncle(node), BLACK);
            nd_set_color(nd_parent(node), BLACK);
            // Move up the tree
            node = curr;
            curr = nd_parent(curr);
            continue;
        }

        // Case 2: uncle is black, and node forms a triangle with its grandpa
        char tridir = check_if_triangle(node);
        if (tridir != 0) {
            if (tridir < 0)
                left_rotate(rbtree, nd_parent(node));
            else
                right_rotate(rbtree, nd_parent(node));
            // Move up the tree
            node_t* tmp = curr;
            curr = node;
            node = tmp;
            continue;
        }

        // Case 3: uncle is black, and node forms a line with its grandpa
        char linedir = check_if_line(node);
        if (linedir != 0) {
            nd_set_color(grandpa(node), RED);
            nd_set_color(nd_parent(node), BLACK);

            if (linedir < 0)
                right_rotate(rbtree, grandpa(node));
            else
                left_rotate(rbtree, grandpa(node));

            break;
        }
    }
}

void rbtree_remove(rbtree_t* rbtree, node_t* node) {
    // Remove element as if in a BST
    node_t* succ = min_node(nd_right(node));
    node_t* pred = max_node(nd_left(node));
    node_t* heir = (succ != NULL) ? succ : pred;

    // If deleting the only node left...
    if (heir == NULL && node == rbtree->root) {
        rbtree->root = NULL;
        return;
    }

    if (heir != NULL) {
        swap(rbtree, node, heir);
        Color tmp = nd_color(node);
        nd_set_color(node, nd_color(heir));
        nd_set_color(heir, tmp);
    }

    assert(nd_left(node) == NULL || nd_right(node) == NULL);

    // If the node to delete is NOT a leaf node, then we just do normal BST deletion
    if (nd_left(node) != NULL) {
        assert(nd_color(node) != RED || nd_color(nd_left(node)) != RED);
        nd_set_color(nd_left(node), BLACK);
        rbtree_link(nd_parent(node), nd_left(node), is_lc(node));
        return;
    }

    if (nd_right(node) != NULL) {
        assert(nd_color(node) != RED || nd_color(nd_right(node)) != RED);
        nd_set_color(nd_right(node), BLACK);
        rbtree_link(nd_parent(node), nd_right(node), is_lc(node));
        return;
    }

    // The node is a leaf node -- we fight to get rid of double black
    // ! node is now thought of as the double black node
    bool is_db_nil = true;
    while (true) {
        // Case 1
        if (nd_color(node) == RED) {
            rbtree_link(nd_parent(node), NULL, is_lc(node));
            break;
        }

        // Case 2
        if (node == rbtree->root) {
            nd_set_color(node, BLACK);
            break;
        }

        // Case 3
        if (nd_color(sibling(node)) == BLACK && nd_color(nd_left(sibling(node))) == BLACK &&
            nd_color(nd_right(sibling(node))) == BLACK) {
            nd_set_color(sibling(node), RED);
            node_t* new_db = NULL;

            if (nd_color(nd_parent(node)) == BLACK) {
                new_db = nd_parent(node);
            } else {
                nd_set_color(nd_parent(node), BLACK);
            }

            if (is_db_nil) {
                rbtree_link(nd_parent(node), NULL, is_lc(node));
                is_db_nil = false;
            }

            if (new_db != NULL)
                node = new_db;
            else
                break;
            continue;
        }

        // Case 4
        if (nd_color(sibling(node)) == RED) {
            Color tmp = nd_color(nd_parent(node));
            nd_set_color(nd_parent(node), nd_color(sibling(node)));
            nd_set_color(sibling(node), tmp);

            if (is_lc(node))
                left_rotate(rbtree, nd_parent(node));
            else
                right_rotate(rbtree, nd_parent(node));
            continue;
        }

        // Case 5
        node_t* far_nephew = is_lc(node) ? nd_right(sibling(node)) : nd_left(sibling(node));
        node_t* near_nephew = is_lc(node) ? nd_left(sibling(node)) : nd_right(sibling(node));

        if (nd_color(sibling(node)) == BLACK && nd_color(far_nephew) == BLACK && nd_color(near_nephew) == RED) {
            nd_set_color(sibling(node), RED);
            nd_set_color(near_nephew, BLACK);

            if (is_lc(node))
                right_rotate(rbtree, sibling(node));
            else
                left_rotate(rbtree, sibling(node));
            // Apply case 6
        }

        // Re-compute these
        far_nephew = is_lc(node) ? nd_right(sibling(node)) : nd_left(sibling(node));
        near_nephew = is_lc(node) ? nd_left(sibling(node)) : nd_right(sibling(node));

        // Case 6
        Color tmp = nd_color(nd_parent(node));
        nd_set_color(nd_parent(node), nd_color(sibling(node)));
        nd_set_color(sibling(node), tmp);

        if (is_lc(node))
            left_rotate(rbtree, nd_parent(node));
        else
            right_rotate(rbtree, nd_parent(node));

        if (is_db_nil) {
            rbtree_link(nd_parent(node), NULL, is_lc(node));
            is_db_nil = false;
        }
        nd_set_color(far_nephew, BLACK);
        break;
    }
}
/* -------------------------------------------------------------------------- */
/*                                   HELPERS                                  */
/* -------------------------------------------------------------------------- */
/* --------------------------- TREE MODIFICATIONS --------------------------- */
void rbtree_link(node_t* to_be_parent, node_t* to_be_child, bool left) {
    if (left) {
        if (nd_left(to_be_parent) != NULL) {
            nd_set_parent(nd_left(to_be_parent), NULL);
        }
        nd_set_left(to_be_parent, to_be_child);
    } else {
        if (nd_right(to_be_parent) != NULL) {
            nd_set_parent(nd_right(to_be_parent), NULL);
        }
        nd_set_right(to_be_parent, to_be_child);
    }
    if (to_be_child != NULL)
        nd_set_parent(to_be_child, to_be_parent);
}

void swap(rbtree_t* rbtree, node_t* a, node_t* b) {
    // Parent exchange
    if (rbtree->root != a) {
        if (is_lc(a))
            nd_set_left(nd_parent(a), b);
        else
            nd_set_right(nd_parent(a), b);
    }

    if (rbtree->root != b) {
        if (is_lc(b))
            nd_set_left(nd_parent(b), a);
        else
            nd_set_right(nd_parent(b), a);
    }

    node_t* tmp = nd_parent(a);
    nd_set_parent(a, nd_parent(b));
    nd_set_parent(b, tmp);
    if (rbtree->root == a) rbtree->root = b;
    else if (rbtree->root == b) rbtree->root = a;

    // Children exchange
    // Left child
    if (nd_left(a) != NULL)
        nd_set_parent(nd_left(a), b);
    if (nd_left(b) != NULL)
        nd_set_parent(nd_left(b), a);

    tmp = nd_left(a);
    nd_set_left(a, nd_left(b));
    nd_set_left(b, tmp);

    // Right child
    if (nd_right(a) != NULL)
        nd_set_parent(nd_right(a), b);
    if (nd_right(b) != NULL)
        nd_set_parent(nd_right(b), a);

    tmp = nd_right(a);
    nd_set_right(a, nd_right(b));
    nd_set_right(b, tmp);
}

node_t* left_rotate(rbtree_t* rbtree, node_t* node) {
    assert(nd_right(node) != NULL);

    nd_set_parent(nd_right(node), NULL);

    if (nd_parent(node) != NULL) {
        rbtree_link(nd_parent(node), nd_right(node), is_lc(node));
    }

    node_t* rl_gc = nd_left(nd_right(node));

    rbtree_link(nd_right(node), node, true);
    nd_set_right(node, NULL);
    rbtree_link(node, rl_gc, false);

    if (rbtree->root == node) {
        rbtree->root = nd_parent(node);
        nd_set_parent(rbtree->root, NULL); // ! Shouldn't be necessary with ghost node
    }
    return nd_parent(node);
}

node_t* right_rotate(rbtree_t* rbtree, node_t* node) {
    assert(nd_left(node) != NULL);

    nd_set_parent(nd_left(node), NULL);

    if (nd_parent(node) != NULL) {
        rbtree_link(nd_parent(node), nd_left(node), is_lc(node));
    }

    node_t* lr_gc = nd_right(nd_left(node));

    rbtree_link(nd_left(node), node, false);
    nd_set_left(node, NULL);
    rbtree_link(node, lr_gc, true);

    if (rbtree->root == node) {
        rbtree->root = nd_parent(node);
        nd_set_parent(rbtree->root, NULL); // ! Shouldn't be necessary with ghost node
    }
    return nd_parent(node);
}

/* ----------------------------- FAMILY MEMBERS ----------------------------- */
node_t* grandpa(node_t* node) {
    return nd_parent(nd_parent(node));
}

node_t* uncle(node_t* node) {
    return is_lc(nd_parent(node)) ? nd_right(grandpa(node)) : nd_left(grandpa(node));
}

node_t* sibling(node_t* node) {
    return is_lc(node) ? nd_right(nd_parent(node)) : nd_left(nd_parent(node));
}

/* ---------------------------- NODE INFORMATION ---------------------------- */
bool is_lc(node_t* node) {
    assert(nd_parent(node) != NULL);
    return nd_left(nd_parent(node)) == node;
}

node_t* min_node(node_t* node) {
    if (node == NULL)
        return NULL;
    while (nd_left(node) != NULL) {
        node = nd_left(node);
    }
    return node;
}

node_t* max_node(node_t* node) {
    if (node == NULL)
        return NULL;
    while (nd_right(node) != NULL) {
        node = nd_right(node);
    }
    return node;
}

/* ---------------------------------- SHAPE --------------------------------- */
char check_if_triangle(node_t* node) {
    if (nd_parent(node) == NULL || grandpa(node) == NULL)
        return 0;

    if (!is_lc(node) && is_lc(nd_parent(node)))
        return -1;
    if (is_lc(node) && !is_lc(nd_parent(node)))
        return 1;
    return 0;
}

char check_if_line(node_t* node) {
    if (nd_parent(node) == NULL || grandpa(node) == NULL)
        return 0;

    if (is_lc(node) && is_lc(nd_parent(node)))
        return -1;
    if (!is_lc(node) && !is_lc(nd_parent(node)))
        return 1;
    return 0;
}

/* -------------------------------- DEBUGGING ------------------------------- */
size_t rbtree_to_vec(node_t* node, node_t* result[]) {
    if (node == NULL) return 0;
    result[0] = node;

    size_t l_elems = rbtree_to_vec(nd_left(node), result + 1);
    size_t r_elems = rbtree_to_vec(nd_right(node), result + 1 + l_elems);
    return 1 + l_elems + r_elems;
}
