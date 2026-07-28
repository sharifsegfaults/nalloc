#include <assert.h>

#include "rbtree.h"

void rbtree_insert(rbtree_t rbtree, hptr_t block) {
    assert(bk_is_free(block));

    // Starting metadata
    bk_set_left(block, NULL_HPTR);
    bk_set_right(block, NULL_HPTR);
    bk_set_parent(block, NULL_HPTR);
    bk_set_color(block, RED);

    hptr_t ghost_node = rbtree.block;
    hptr_t curr = root(rbtree);

    // If the tree is empty...
    if (root(rbtree) == NULL_HPTR) {
        tlink(ghost_node, block, true);
        bk_set_color(root(rbtree), BLACK);
        return;
    }

    // Navigate down the tree until you find the insertion spot
    while (true) {
        if (bk_size(block) < bk_size(curr)) {
            if (bk_left(curr) == NULL_HPTR) {
                tlink(curr, block, true);
                break;
            }
            curr = bk_left(curr);
        } else {
            if (bk_right(curr) == NULL_HPTR) {
                tlink(curr, block, false);
                break;
            }
            curr = bk_right(curr);
        }
    }

    if (bk_color(curr) == BLACK) {
        return;
    }

    // Every time we solve an error, we make the current node
    // pose as the newly inserted node, and make our parent
    // be the current node. We re-run this until we reach the root.

    while (block != ghost_node) {
        // Case 0: new node is root --> make it black
        if (root(rbtree) == block) {
            bk_set_color(block, BLACK);
            break;
        }

        // If there are no problems, we just continue :)
        if (bk_color(block) != RED || bk_color(curr) != RED) {
            block = curr;
            curr = bk_parent(curr);
            continue;
        }

        // Case 1: block's uncle is red
        if (bk_color(uncle(block)) == RED) {
            bk_set_color(grandpa(block), RED);
            bk_set_color(uncle(block), BLACK);
            bk_set_color(bk_parent(block), BLACK);
            // Move up the tree
            block = curr;
            curr = bk_parent(curr);
            continue;
        }

        // Case 2: uncle is black, and block forms a triangle with its grandpa
        char tridir = check_if_triangle(block);
        if (tridir != 0) {
            if (tridir < 0) left_rotate(bk_parent(block));
            else right_rotate(bk_parent(block));
            // Move up the tree
            hptr_t tmp = curr;
            curr = block;
            block = tmp;
            continue;
        }

        // Case 3: uncle is black, and block forms a line with its grandpa
        char linedir = check_if_line(block);
        if (linedir != 0) {
            bk_set_color(grandpa(block), RED);
            bk_set_color(bk_parent(block), BLACK);

            if (linedir < 0) right_rotate(grandpa(block));
            else left_rotate(grandpa(block));

            break;
        }
    }
}

hptr_t rbtree_find(rbtree_t rbtree, uint32_t size) {
    hptr_t curr = root(rbtree);
    hptr_t ub = NULL_HPTR;

    while (curr != NULL_HPTR) {
        if (size <= bk_size(curr)) {
            ub = curr;
            curr = bk_left(curr);
        } else if (size > bk_size(curr)) {
            curr = bk_right(curr);
        }
    }

    return ub;
}

void rbtree_remove(rbtree_t rbtree, hptr_t block) {
    assert(bk_is_free(block));
    // Remove element as if in a BST
    hptr_t succ = min_node(bk_right(block));
    hptr_t pred = max_node(bk_left(block));
    hptr_t heir = (succ != NULL_HPTR) ? succ : pred;

    // If deleting the only node left...
    if (heir == NULL_HPTR && block == root(rbtree)) {
        set_root(rbtree, NULL_HPTR);
        return;
    }

    if (heir != NULL_HPTR) {
        swap(block, heir);
        Color tmp = bk_color(block);
        bk_set_color(block, bk_color(heir));
        bk_set_color(heir, tmp);
    }

    assert(bk_left(block) == NULL_HPTR || bk_right(block) == NULL_HPTR);
    
    // If the node to delete is NOT a leaf node, then we just do normal BST deletion
    if (bk_left(block) != NULL_HPTR) {
        assert(bk_color(block) != RED || bk_color(bk_left(block)) != RED);
        bk_set_color(bk_left(block), BLACK);
        tlink(bk_parent(block), bk_left(block), is_lc(block));
        return;
    }

    if (bk_right(block) != NULL_HPTR) {
        assert(bk_color(block) != RED || bk_color(bk_right(block)) != RED);
        bk_set_color(bk_right(block), BLACK);
        tlink(bk_parent(block), bk_right(block), is_lc(block));
        return;
    }

    // The node is a leaf node -- we fight to get rid of double black
    // ! block is now thought of as the double black node
    bool is_db_nil = true;
    while (true) {
        // Case 1
        if (bk_color(block) == RED) {
            tlink(bk_parent(block), NULL_HPTR, is_lc(block));
            break;
        }

        // Case 2
        if (block == root(rbtree)) {
            bk_set_color(block, BLACK);
            break;
        }

        // Case 3
        if (bk_color(sibling(block)) == BLACK
        && bk_color(bk_left(sibling(block))) == BLACK
        && bk_color(bk_right(sibling(block))) == BLACK) {
            bk_set_color(sibling(block), RED);
            hptr_t new_db = NULL_HPTR;
            
            if (bk_color(bk_parent(block)) == BLACK) {
                new_db = bk_parent(block);
            } else {
                bk_set_color(bk_parent(block), BLACK);
            }

            if (is_db_nil) {
                tlink(bk_parent(block), NULL_HPTR, is_lc(block));
                is_db_nil = false;
            }

            if (new_db != NULL_HPTR) block = new_db;
            else break;
            continue;
        }

        // Case 4
        if (bk_color(sibling(block)) == RED) {
            Color tmp = bk_color(bk_parent(block));
            bk_set_color(bk_parent(block), bk_color(sibling(block)));
            bk_set_color(sibling(block), tmp);

            if (is_lc(block)) left_rotate(bk_parent(block));
            else right_rotate(bk_parent(block));
            continue;
        }

        // Case 5
        hptr_t far_nephew = is_lc(block) ? bk_right(sibling(block)) : bk_left(sibling(block));
        hptr_t near_nephew = is_lc(block) ? bk_left(sibling(block)) : bk_right(sibling(block));

        if (bk_color(sibling(block)) == BLACK
        && bk_color(far_nephew) == BLACK
        && bk_color(near_nephew) == RED) {
            bk_set_color(sibling(block), RED);
            bk_set_color(near_nephew, BLACK);

            if (is_lc(block)) right_rotate(sibling(block));
            else left_rotate(sibling(block));
            // Apply case 6
        }

        // Re-compute these
        far_nephew = is_lc(block) ? bk_right(sibling(block)) : bk_left(sibling(block));
        near_nephew = is_lc(block) ? bk_left(sibling(block)) : bk_right(sibling(block));

        // Case 6
        Color tmp = bk_color(bk_parent(block));
        bk_set_color(bk_parent(block), bk_color(sibling(block)));
        bk_set_color(sibling(block), tmp);

        if (is_lc(block)) left_rotate(bk_parent(block));
        else right_rotate(bk_parent(block));
        
        if (is_db_nil) {
            tlink(bk_parent(block), NULL_HPTR, is_lc(block));
            is_db_nil = false;
        }
        bk_set_color(far_nephew, BLACK);
        break;
    }
}
/* -------------------------------------------------------------------------- */
/*                                   HELPERS                                  */
/* -------------------------------------------------------------------------- */
/* --------------------------- TREE MODIFICATIONS --------------------------- */
void tlink(hptr_t to_be_parent, hptr_t to_be_child, bool left) {
    if (left) {
        if (bk_left(to_be_parent) != NULL_HPTR) {
            bk_set_parent(bk_left(to_be_parent), NULL_HPTR);
        }
        bk_set_left(to_be_parent, to_be_child);
    } else {
        if (bk_right(to_be_parent) != NULL_HPTR) {
            bk_set_parent(bk_right(to_be_parent), NULL_HPTR);
        }
        bk_set_right(to_be_parent, to_be_child);
    }
    if (to_be_child != NULL_HPTR) bk_set_parent(to_be_child, to_be_parent);
}

void swap(hptr_t a, hptr_t b) {
    // Parent exchange
    if (is_lc(a)) bk_set_left(bk_parent(a), b);
    else bk_set_right(bk_parent(a), b);

    if (is_lc(b)) bk_set_left(bk_parent(b), a);
    else bk_set_right(bk_parent(b), a);

    hptr_t tmp = bk_parent(a);
    bk_set_parent(a, bk_parent(b));
    bk_set_parent(b, tmp);

    // Children exchange
    // Left child
    if (bk_left(a) != NULL_HPTR) bk_set_parent(bk_left(a), b);
    if (bk_left(b) != NULL_HPTR) bk_set_parent(bk_left(b), a);

    tmp = bk_left(a);
    bk_set_left(a, bk_left(b));
    bk_set_left(b, tmp);
    
    // Right child
    if (bk_right(a) != NULL_HPTR) bk_set_parent(bk_right(a), b);
    if (bk_right(b) != NULL_HPTR) bk_set_parent(bk_right(b), a);

    tmp = bk_right(a);
    bk_set_right(a, bk_right(b));
    bk_set_right(b, tmp);
}

hptr_t left_rotate(hptr_t block) {
    assert(bk_right(block) != NULL_HPTR);

    bk_set_parent(bk_right(block), NULL_HPTR);

    if (bk_parent(block) != NULL_HPTR) {
        tlink(bk_parent(block), bk_right(block), is_lc(block));
    }

    hptr_t rl_gc = bk_left(bk_right(block));

    tlink(bk_right(block), block, true);
    bk_set_right(block, NULL_HPTR);
    tlink(block, rl_gc, false);

    return bk_parent(block);
}

hptr_t right_rotate(hptr_t block) {
    assert(bk_left(block) != NULL_HPTR);

    bk_set_parent(bk_left(block), NULL_HPTR);
    
    if (bk_parent(block) != NULL_HPTR) {
        tlink(bk_parent(block), bk_left(block), is_lc(block));
    }

    hptr_t lr_gc = bk_right(bk_left(block));

    tlink(bk_left(block), block, false);
    bk_set_left(block, NULL_HPTR);
    tlink(block, lr_gc, true);

    return bk_parent(block);
}

/* ---------------------------- TREE DATA ACCESS ---------------------------- */
hptr_t root(rbtree_t rbtree) {
    return bk_left(rbtree.block);
}

void set_root(rbtree_t rbtree, hptr_t new_root) {
    bk_set_left(rbtree.block, new_root);
}

/* ----------------------------- FAMILY MEMBERS ----------------------------- */
hptr_t grandpa(hptr_t block) {
    return bk_parent(bk_parent(block));
}

hptr_t uncle(hptr_t block) {    
    return is_lc(bk_parent(block)) ? bk_right(grandpa(block)) : bk_left(grandpa(block));
}

hptr_t sibling(hptr_t block) {
    return is_lc(block) ? bk_right(bk_parent(block)) : bk_left(bk_parent(block));
}

/* ---------------------------- NODE INFORMATION ---------------------------- */
bool is_lc(hptr_t block) {
    assert(bk_parent(block) != NULL_HPTR);
    return bk_left(bk_parent(block)) == block;
}

hptr_t min_node(hptr_t block) {
    if (block == NULL_HPTR) return NULL_HPTR;
    while (bk_left(block) != NULL_HPTR) {
        block = bk_left(block);
    }
    return block;
}

hptr_t max_node(hptr_t block) {
    if (block == NULL_HPTR) return NULL_HPTR;
    while (bk_right(block) != NULL_HPTR) {
        block = bk_right(block);
    }
    return block;
}

/* ---------------------------------- SHAPE --------------------------------- */
char check_if_triangle(hptr_t block) {
    if (bk_parent(block) == NULL_HPTR || grandpa(block) == NULL_HPTR) return 0;

    if (!is_lc(block) && is_lc(bk_parent(block))) return -1;
    if (is_lc(block) && !is_lc(bk_parent(block))) return 1;
    return 0;
}

char check_if_line(hptr_t block) {
    if (bk_parent(block) == NULL_HPTR || grandpa(block) == NULL_HPTR) return 0;

    if (is_lc(block) && is_lc(bk_parent(block))) return -1;
    if (!is_lc(block) && !is_lc(bk_parent(block))) return 1;
    return 0;
}

/* -------------------------------- DEBUGGING ------------------------------- */
Node snode(uint32_t size) {
    Node nody = { RED, NULL_HPTR, NULL_HPTR, NULL_HPTR, size };
    return nody;
}

Node csnode(Color color, uint32_t size) {
    Node nody = { color, NULL_HPTR, NULL_HPTR, NULL_HPTR, size };
    return nody;
}

Node node(Color color, uint32_t size, hptr_t left, hptr_t right, hptr_t parent) {
    Node nody = { color, left, right, parent, size };
    return nody;
}

uint32_t rbtree_to_vec(hptr_t block, Node* result) {
    if (block == NULL_HPTR) return 0;
    result->color = bk_color(block);
    result->left = bk_left(block);
    result->right = bk_right(block);
    result->parent = bk_parent(block);
    result->size = bk_size(block);
    
    uint32_t l_elems = rbtree_to_vec(bk_left(block), result + 1);
    uint32_t r_elems = rbtree_to_vec(bk_right(block), result + 1 + l_elems);
    return 1 + l_elems + r_elems;
}
