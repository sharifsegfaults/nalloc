#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "utils.h"

#include "rbtree.h"

static char* heap;

typedef struct {
    int data;
    node_t rbtree_node;
} Data;

static node_t* rbtree_find(rbtree_t* rbtree, uint32_t size) {
    node_t* curr_nd = rbtree->root;
    node_t* ub = NULL;

    while (curr_nd != NULL) {
        Data* curr_data = container_of(curr_nd, Data, rbtree_node);
        if (size <= curr_data->data) {
            ub = curr_nd;
            curr_nd = nd_left(curr_nd);
        } else if (size > curr_data->data) {
            curr_nd = nd_right(curr_nd);
        }
    }

    return ub;
}

static void rbtree_insert(rbtree_t* rbtree, node_t* node) {
    Data* data = container_of(node, Data, rbtree_node);
    if (rbtree->root == NULL) {
        rbtree->root = node;
    } else {
        node_t* curr_nd = rbtree->root;

        while (true) {
            Data* curr_data = container_of(curr_nd, Data, rbtree_node);
            if (data->data < curr_data->data) {
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

typedef struct {
    Color color;
    int data;
} NodeExp;

bool rbtree_eq_vec(rbtree_t* rbtree, NodeExp expected[], int size) {
    node_t* vectree[size];
    rbtree_to_vec(rbtree->root, vectree);

    for (int i = 0; i < size; ++i) {
        Data* actual_data = container_of(vectree[i], Data, rbtree_node);
        if (actual_data->data != expected[i].data || nd_color(vectree[i]) != expected[i].color) {
            return false;
        }
    }

    return true;
}

bool TEST_LEFT_ROTATION() {
    Data five = { 50, {} };
    Data two = {20, {}};
    Data ten = {100, {}};
    Data eight = {80, {}};
    Data twelve = {120, {}};
    Data six = {60, {}};
    Data nine = {90, {}};

    rbtree_link(&five.rbtree_node, &two.rbtree_node, true);
    rbtree_link(&five.rbtree_node, &ten.rbtree_node, false);
    rbtree_link(&ten.rbtree_node, &eight.rbtree_node, true);
    rbtree_link(&ten.rbtree_node, &twelve.rbtree_node, false);
    rbtree_link(&eight.rbtree_node, &six.rbtree_node, true);
    rbtree_link(&eight.rbtree_node, &nine.rbtree_node, false);

    rbtree_t rbtree = create_rbtree();
    node_t* res = left_rotate(&rbtree, &five.rbtree_node);

    node_t* vectree[7] = {};
    rbtree_to_vec(res, vectree);
    int expected[] = { ten.data, five.data, two.data, eight.data, six.data, nine.data, twelve.data };

    for (int i = 0; i < 7; ++i) {
        Data* datanode = container_of(vectree[i], Data, rbtree_node);
        assert(datanode->data == expected[i]);
    }

    return true;
}

bool TEST_RIGHT_ROTATION() {
    Data five = { 50, {} };
    Data two = {20, {}};
    Data ten = {100, {}};
    Data eight = {80, {}};
    Data twelve = {120, {}};
    Data six = {60, {}};
    Data nine = {90, {}};

    rbtree_link(&ten.rbtree_node, &five.rbtree_node, true);
    rbtree_link(&ten.rbtree_node, &twelve.rbtree_node, false);
    rbtree_link(&five.rbtree_node, &two.rbtree_node, true);
    rbtree_link(&five.rbtree_node, &eight.rbtree_node, false);
    rbtree_link(&eight.rbtree_node, &six.rbtree_node, true);
    rbtree_link(&eight.rbtree_node, &nine.rbtree_node, false);

    rbtree_t rbtree = create_rbtree();
    node_t* res = right_rotate(&rbtree, &ten.rbtree_node);

    node_t* vectree[7] = {};
    rbtree_to_vec(res, vectree);
    int expected[] = { five.data, two.data, ten.data, eight.data, six.data, nine.data, twelve.data };

    for (int i = 0; i < 7; ++i) {
        assert(container_of(vectree[i], Data, rbtree_node)->data == expected[i]);
    }

    return true;
}

bool TEST_INSERTION() {
    rbtree_t rbtree = create_rbtree();

    Data fifteen = {150, {}};
    Data five = {50, {}};
    Data one = {10, {}};
    Data twelve = {120, {}};
    Data thirteen = {130, {}};
    Data seventeen = {170, {}};
    Data nineteen = {190, {}};
    Data twentyone = {210, {}};

    // clang-format off
    rbtree_insert(&rbtree, &fifteen.rbtree_node);
    assert(rbtree_eq_vec(&rbtree, (NodeExp[1]){
        { BLACK, 150 }
    }, 1));
    rbtree_insert(&rbtree, &five.rbtree_node);
    assert(rbtree_eq_vec(&rbtree, (NodeExp[2]){
        { BLACK, 150 },
        { RED, 50 }
    }, 2));
    rbtree_insert(&rbtree, &one.rbtree_node);
    assert(rbtree_eq_vec(&rbtree, (NodeExp[3]){
        { BLACK, 50 },
        { RED, 10 },
        { RED, 150 }
    }, 3));
    rbtree_insert(&rbtree, &twelve.rbtree_node);
    assert(rbtree_eq_vec(&rbtree, (NodeExp[4]){
        { BLACK, 50 },
        { BLACK, 10 },
        { BLACK, 150 },
        { RED, 120 }
    }, 4));
    rbtree_insert(&rbtree, &thirteen.rbtree_node);
    assert(rbtree_eq_vec(&rbtree, (NodeExp[5]){
        { BLACK, 50 },
        { BLACK, 10 },
        { BLACK, 130 },
        { RED, 120 },
        { RED, 150 }
    }, 5));
    rbtree_insert(&rbtree, &seventeen.rbtree_node);
    assert(rbtree_eq_vec(&rbtree, (NodeExp[6]){
        { BLACK, 50 },
        { BLACK, 10 },
        { RED, 130 },
        { BLACK, 120 },
        { BLACK, 150 },
        { RED, 170 }
    }, 6));
    rbtree_insert(&rbtree, &nineteen.rbtree_node);
    assert(rbtree_eq_vec(&rbtree, (NodeExp[7]){
        { BLACK, 50 },
        { BLACK, 10 },
        { RED, 130 },
        { BLACK, 120 },
        { BLACK, 170 },
        { RED, 150 },
        { RED, 190 }
    }, 7));
    rbtree_insert(&rbtree, &twentyone.rbtree_node);
    assert(rbtree_eq_vec(&rbtree, (NodeExp[8]){
        { BLACK, 130 },
        { RED, 50 },
        { BLACK, 10 },
        { BLACK, 120 },
        { RED, 170 },
        { BLACK, 150 },
        { BLACK, 190 },
        { RED, 210 }
    }, 8));
    // clang-format off
    return true;
}

bool TEST_FIND() {
    rbtree_t rbtree = create_rbtree();

    Data fifteen = {150, {}};
    Data five = {50, {}};
    Data one = {10, {}};
    Data twelve = {120, {}};
    Data thirteen = {130, {}};
    Data seventeen = {170, {}};
    Data nineteen = {190, {}};
    Data twentyone = {210, {}};

    rbtree_insert(&rbtree, &fifteen.rbtree_node);
    rbtree_insert(&rbtree, &five.rbtree_node);
    rbtree_insert(&rbtree, &one.rbtree_node);
    rbtree_insert(&rbtree, &twelve.rbtree_node);
    rbtree_insert(&rbtree, &thirteen.rbtree_node);
    rbtree_insert(&rbtree, &seventeen.rbtree_node);
    rbtree_insert(&rbtree, &nineteen.rbtree_node);
    rbtree_insert(&rbtree, &twentyone.rbtree_node);

    // Check exact matches
    int nums[8] = {150, 50, 10, 120, 130, 170, 190, 210};
    for (int i = 0; i < 8; ++i) {
        Data* res = container_of(rbtree_find(&rbtree, nums[i]), Data, rbtree_node);
        assert(res->data == nums[i]);
    }

    // Check if it returns lower bound when not an exact match
    assert(container_of(rbtree_find(&rbtree, 110), Data, rbtree_node)->data == 120);
    assert(container_of(rbtree_find(&rbtree, 30), Data, rbtree_node)->data == 50);
    assert(container_of(rbtree_find(&rbtree, 140), Data, rbtree_node)->data == 150);
    assert(rbtree_find(&rbtree, 140000) == NULL);
    assert(container_of(rbtree_find(&rbtree, 0), Data, rbtree_node)->data == 10);
    assert(container_of(rbtree_find(&rbtree, 180), Data, rbtree_node)->data == 190);
    assert(container_of(rbtree_find(&rbtree, 200), Data, rbtree_node)->data == 210);

    return true;
}

bool TEST_REMOVE() {
    rbtree_t rbtree = create_rbtree();

    Data fifteen = {150, {}};
    Data five = {50, {}};
    Data one = {10, {}};
    Data twelve = {120, {}};
    Data thirteen = {130, {}};
    Data seventeen = {170, {}};
    Data nineteen = {190, {}};
    Data twentyone = {210, {}};

    rbtree_insert(&rbtree, &fifteen.rbtree_node);
    rbtree_insert(&rbtree, &five.rbtree_node);
    rbtree_insert(&rbtree, &one.rbtree_node);
    rbtree_insert(&rbtree, &twelve.rbtree_node);
    rbtree_insert(&rbtree, &thirteen.rbtree_node);
    rbtree_insert(&rbtree, &seventeen.rbtree_node);
    rbtree_insert(&rbtree, &nineteen.rbtree_node);
    rbtree_insert(&rbtree, &twentyone.rbtree_node);

    // clang-format off
    rbtree_remove(&rbtree, &fifteen.rbtree_node);
    assert(rbtree_eq_vec(&rbtree, (NodeExp[7]){
        { BLACK, 130 },
        { RED, 50 },
        { BLACK, 10 },
        { BLACK, 120 },
        { RED, 190 },
        { BLACK, 170 },
        { BLACK, 210 }
    }, 7));

    rbtree_remove(&rbtree, &nineteen.rbtree_node);
    assert(rbtree_eq_vec(&rbtree, (NodeExp[6]){
        { BLACK, 130 },
        { RED, 50 },
        { BLACK, 10 },
        { BLACK, 120 },
        { BLACK, 210 },
        { RED, 170 }
    }, 6));

    rbtree_remove(&rbtree, &thirteen.rbtree_node);
    assert(rbtree_eq_vec(&rbtree, (NodeExp[5]){
        { BLACK, 170 },
        { RED, 50 },
        { BLACK, 10 },
        { BLACK, 120 },
        { BLACK, 210 }
    }, 5));

    rbtree_remove(&rbtree, &one.rbtree_node);
    assert(rbtree_eq_vec(&rbtree, (NodeExp[4]){
        { BLACK, 170 },
        { BLACK, 50 },
        { RED, 120 },
        { BLACK, 210 }
    }, 4));

    rbtree_remove(&rbtree, &seventeen.rbtree_node);
    assert(rbtree_eq_vec(&rbtree, (NodeExp[3]){
        { BLACK, 120 },
        { BLACK, 50 },
        { BLACK, 210 }
    }, 3));

    rbtree_remove(&rbtree, &twelve.rbtree_node);
    assert(rbtree_eq_vec(&rbtree, (NodeExp[2]){
        { BLACK, 210 },
        { RED, 50 }
    }, 2));

    rbtree_remove(&rbtree, &five.rbtree_node);
    assert(rbtree_eq_vec(&rbtree, (NodeExp[1]){
        { BLACK, 210 }
    }, 1));

    rbtree_remove(&rbtree, &twentyone.rbtree_node);
    assert(rbtree.root == NULL);
    //clang-format on

    return true;
}

bool TEST_FROM_HELL() {
    rbtree_t rbtree = create_rbtree();

    Data p32 = { 32, {} };
    Data p104 = { 104, {} };
    Data p152 = { 152, {} };
    Data p264 = { 264, {} };
    Data p24 = { 24, {} };

    rbtree_insert(&rbtree, &p32.rbtree_node);
    assert(rbtree_eq_vec(&rbtree, (NodeExp[1]){
        { BLACK, 32 },
    }, 1));

    rbtree_insert(&rbtree, &p104.rbtree_node);
    assert(rbtree_eq_vec(&rbtree, (NodeExp[2]){
        { BLACK, 32 },
        { RED, 104 }
    }, 2));
    rbtree_insert(&rbtree, &p152.rbtree_node);
    assert(rbtree_eq_vec(&rbtree, (NodeExp[3]){
        { BLACK, 104 },
        { RED, 32 },
        { RED, 152 }
    }, 3));
    rbtree_insert(&rbtree, &p264.rbtree_node);
    assert(rbtree_eq_vec(&rbtree, (NodeExp[4]){
        { BLACK, 104 },
        { BLACK, 32 },
        { BLACK, 152 },
        { RED, 264 }
    }, 4));
    rbtree_insert(&rbtree, &p24.rbtree_node);
    assert(rbtree_eq_vec(&rbtree, (NodeExp[5]){
        { BLACK, 104 },
        { BLACK, 32 },
        { RED, 24 },
        { BLACK, 152 },
        { RED, 264 }
    }, 5));

    rbtree_remove(&rbtree, &p104.rbtree_node);
    assert(rbtree_eq_vec(&rbtree, (NodeExp[4]){
        { BLACK, 152 },
        { BLACK, 32 },
        { RED, 24 },
        { BLACK, 264 }
    }, 4));

    return true;
}

int main() {
    TEST_LEFT_ROTATION();
    TEST_RIGHT_ROTATION();
    TEST_INSERTION();
    TEST_FIND();
    TEST_REMOVE();

    TEST_FROM_HELL();

    printf("Success B)\n");
}
