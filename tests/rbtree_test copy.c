#include <assert.h>
#include <stdlib.h>

#include "rbtree.h"

#define S(x) ALIGN(x)

static char* heap;
hptr_t nb = 0;

bool rbtree_eq_vec(rbtree_t rbtree, Node expected[], int size) {
    Node vectree[size];
    rbtree_to_vec(root(rbtree), vectree);

    for (int i = 0; i < size; ++i) {
        if (vectree[i].size != expected[i].size || vectree[i].color != expected[i].color) {
            return false;
        }
    }

    return true;
}

hptr_t create_block(uint32_t size) {
    assert(size >= 16);
    hptr_t old_nb = nb;
    bk_set_size(nb, size);
    bk_set_left(nb, NULL_HPTR);
    bk_set_right(nb, NULL_HPTR);
    bk_set_parent(nb, NULL_HPTR);
    bk_set_color(nb, RED);
    bk_set_is_free(nb, true);
    nb += sizeof(AllocBlockHeader) + size;
    return old_nb;
}

rbtree_t create_rbtree() {
    rbtree_t rbtree;
    uint32_t padding = ALIGN((uintptr_t)mem_heap_lo()) - (uintptr_t)mem_heap_lo();
    uint32_t ghost_node_size = ALIGN(sizeof(FreeBlockHeader) + sizeof(BlockFooter));
    mem_sbrk(padding + ghost_node_size);
    // Setup ghost node
    rbtree.block = padding;

    bk_set_left(rbtree.block, NULL_HPTR);
    bk_set_prev_free(rbtree.block, false);

    return rbtree;
}

bool TEST_LEFT_ROTATION() {
    hptr_t five = create_block(S(50));
    hptr_t two = create_block(S(20));
    hptr_t ten = create_block(S(100));
    hptr_t eight = create_block(S(80));
    hptr_t twelve = create_block(S(120));
    hptr_t six = create_block(S(60));
    hptr_t nine = create_block(S(90));

    rbtree_link(five, two, true);
    rbtree_link(five, ten, false);
    rbtree_link(ten, eight, true);
    rbtree_link(ten, twelve, false);
    rbtree_link(eight, six, true);
    rbtree_link(eight, nine, false);

    hptr_t res = left_rotate(five);

    Node vectree[7] = {};
    rbtree_to_vec(res, vectree);
    int expected[] = {S(100), S(50), S(20), S(80), S(60), S(90), S(120)};

    for (int i = 0; i < 7; ++i) {
        assert(vectree[i].size == expected[i]);
    }

    return true;
}

bool TEST_RIGHT_ROTATION() {
    hptr_t five = create_block(S(50));
    hptr_t two = create_block(S(20));
    hptr_t ten = create_block(S(100));
    hptr_t eight = create_block(S(80));
    hptr_t twelve = create_block(S(120));
    hptr_t six = create_block(S(60));
    hptr_t nine = create_block(S(90));

    rbtree_link(ten, five, true);
    rbtree_link(ten, twelve, false);
    rbtree_link(five, two, true);
    rbtree_link(five, eight, false);
    rbtree_link(eight, six, true);
    rbtree_link(eight, nine, false);

    hptr_t res = right_rotate(ten);

    Node vectree[7] = {};
    rbtree_to_vec(res, vectree);
    int expected[] = {S(50), S(20), S(100), S(80), S(60), S(90), S(120)};

    for (int i = 0; i < 7; ++i) {
        assert(vectree[i].size == expected[i]);
    }

    return true;
}

bool TEST_INSERTION() {
    rbtree_t rbtree = create_rbtree();

    rbtree_insert(rbtree, create_block(S(150)));
    assert(rbtree_eq_vec(rbtree, (Node[1]){csnode(BLACK, S(150))}, 1));
    rbtree_insert(rbtree, create_block(S(50)));
    assert(rbtree_eq_vec(rbtree, (Node[2]){csnode(BLACK, S(150)), csnode(RED, S(50))}, 2));
    rbtree_insert(rbtree, create_block(S(10)));
    assert(rbtree_eq_vec(rbtree, (Node[3]){csnode(BLACK, S(50)), csnode(RED, S(10)), csnode(RED, S(150))}, 3));
    rbtree_insert(rbtree, create_block(S(120)));
    assert(rbtree_eq_vec(
        rbtree, (Node[4]){csnode(BLACK, S(50)), csnode(BLACK, S(10)), csnode(BLACK, S(150)), csnode(RED, S(120))}, 4));
    rbtree_insert(rbtree, create_block(S(130)));
    assert(rbtree_eq_vec(rbtree,
                         (Node[5]){csnode(BLACK, S(50)), csnode(BLACK, S(10)), csnode(BLACK, S(130)),
                                   csnode(RED, S(120)), csnode(RED, S(150))},
                         5));
    rbtree_insert(rbtree, create_block(S(170)));
    assert(rbtree_eq_vec(rbtree,
                         (Node[6]){csnode(BLACK, S(50)), csnode(BLACK, S(10)), csnode(RED, S(130)),
                                   csnode(BLACK, S(120)), csnode(BLACK, S(150)), csnode(RED, S(170))},
                         6));
    rbtree_insert(rbtree, create_block(S(190)));
    assert(
        rbtree_eq_vec(rbtree,
                      (Node[7]){csnode(BLACK, S(50)), csnode(BLACK, S(10)), csnode(RED, S(130)), csnode(BLACK, S(120)),
                                csnode(BLACK, S(170)), csnode(RED, S(150)), csnode(RED, S(190))},
                      7));
    rbtree_insert(rbtree, create_block(S(210)));
    assert(
        rbtree_eq_vec(rbtree,
                      (Node[8]){csnode(BLACK, S(130)), csnode(RED, S(50)), csnode(BLACK, S(10)), csnode(BLACK, S(120)),
                                csnode(RED, S(170)), csnode(BLACK, S(150)), csnode(BLACK, S(190)), csnode(RED, S(210))},
                      8));

    return true;
}

bool TEST_FIND() {
    rbtree_t rbtree = create_rbtree();

    rbtree_insert(rbtree, create_block(S(150)));
    rbtree_insert(rbtree, create_block(S(50)));
    rbtree_insert(rbtree, create_block(S(10)));
    rbtree_insert(rbtree, create_block(S(120)));
    rbtree_insert(rbtree, create_block(S(130)));
    rbtree_insert(rbtree, create_block(S(170)));
    rbtree_insert(rbtree, create_block(S(190)));
    rbtree_insert(rbtree, create_block(S(210)));

    // Check exact matches
    int nums[8] = {S(150), S(50), S(10), S(120), S(130), S(170), S(190), S(210)};
    for (int i = 0; i < 8; ++i) {
        assert(bk_size(rbtree_find(rbtree, nums[i])) == nums[i]);
    }

    // Check if it returns lower bound when not an exact match
    assert(bk_size(rbtree_find(rbtree, S(110))) == S(120));
    assert(bk_size(rbtree_find(rbtree, S(30))) == S(50));
    assert(bk_size(rbtree_find(rbtree, S(140))) == S(150));
    assert(rbtree_find(rbtree, S(140000)) == NULL_HPTR);
    assert(bk_size(rbtree_find(rbtree, S(0))) == S(10));
    assert(bk_size(rbtree_find(rbtree, S(180))) == S(190));
    assert(bk_size(rbtree_find(rbtree, S(200))) == S(210));

    return true;
}

bool TEST_REMOVE() {
    rbtree_t rbtree = create_rbtree();

    hptr_t fifteen = create_block(S(150));
    hptr_t five = create_block(S(50));
    hptr_t one = create_block(S(10));
    hptr_t twelve = create_block(S(120));
    hptr_t thirteen = create_block(S(130));
    hptr_t seventeen = create_block(S(170));
    hptr_t nineteen = create_block(S(190));
    hptr_t twentyone = create_block(S(210));

    rbtree_insert(rbtree, fifteen);
    rbtree_insert(rbtree, five);
    rbtree_insert(rbtree, one);
    rbtree_insert(rbtree, twelve);
    rbtree_insert(rbtree, thirteen);
    rbtree_insert(rbtree, seventeen);
    rbtree_insert(rbtree, nineteen);
    rbtree_insert(rbtree, twentyone);

    rbtree_remove(rbtree, fifteen);
    assert(
        rbtree_eq_vec(rbtree,
                      (Node[7]){csnode(BLACK, S(130)), csnode(RED, S(50)), csnode(BLACK, S(10)), csnode(BLACK, S(120)),
                                csnode(RED, S(190)), csnode(BLACK, S(170)), csnode(BLACK, S(210))},
                      7));

    rbtree_remove(rbtree, nineteen);
    assert(rbtree_eq_vec(rbtree,
                         (Node[6]){csnode(BLACK, S(130)), csnode(RED, S(50)), csnode(BLACK, S(10)),
                                   csnode(BLACK, S(120)), csnode(BLACK, S(210)), csnode(RED, S(170))},
                         6));

    rbtree_remove(rbtree, thirteen);
    assert(rbtree_eq_vec(rbtree,
                         (Node[5]){csnode(BLACK, S(170)), csnode(RED, S(50)), csnode(BLACK, S(10)),
                                   csnode(BLACK, S(120)), csnode(BLACK, S(210))},
                         5));

    rbtree_remove(rbtree, one);
    assert(rbtree_eq_vec(
        rbtree, (Node[4]){csnode(BLACK, S(170)), csnode(BLACK, S(50)), csnode(RED, S(120)), csnode(BLACK, S(210))}, 4));

    rbtree_remove(rbtree, seventeen);
    assert(rbtree_eq_vec(rbtree, (Node[3]){csnode(BLACK, S(120)), csnode(BLACK, S(50)), csnode(BLACK, S(210))}, 3));

    rbtree_remove(rbtree, twelve);
    assert(rbtree_eq_vec(rbtree, (Node[2]){csnode(BLACK, S(210)), csnode(RED, S(50))}, 2));

    rbtree_remove(rbtree, five);
    assert(rbtree_eq_vec(rbtree, (Node[1]){csnode(BLACK, S(210))}, 1));

    rbtree_remove(rbtree, twentyone);
    assert(root(rbtree) == NULL_HPTR);

    return true;
}

bool TEST_FROM_HELL() {
    rbtree_t rbtree = create_rbtree();

    hptr_t p32 = create_block(S(32));
    hptr_t p104 = create_block(S(104));
    hptr_t p152 = create_block(S(152));
    hptr_t p264 = create_block(S(264));
    hptr_t p24 = create_block(S(24));

    rbtree_insert(rbtree, p32);
    assert(rbtree_eq_vec(rbtree,
                         (Node[1]){
                             csnode(BLACK, S(32)),
                         },
                         1));
    rbtree_insert(rbtree, p104);
    assert(rbtree_eq_vec(rbtree, (Node[2]){csnode(BLACK, S(32)), csnode(RED, S(104))}, 2));
    rbtree_insert(rbtree, p152);
    assert(rbtree_eq_vec(rbtree, (Node[3]){csnode(BLACK, S(104)), csnode(RED, S(32)), csnode(RED, S(152))}, 3));
    rbtree_insert(rbtree, p264);
    assert(rbtree_eq_vec(
        rbtree, (Node[4]){csnode(BLACK, S(104)), csnode(BLACK, S(32)), csnode(BLACK, S(152)), csnode(RED, S(264))}, 4));
    rbtree_insert(rbtree, p24);
    assert(rbtree_eq_vec(rbtree,
                         (Node[5]){csnode(BLACK, S(104)), csnode(BLACK, S(32)), csnode(RED, S(24)),
                                   csnode(BLACK, S(152)), csnode(RED, S(264))},
                         5));

    rbtree_remove(rbtree, p104);
    assert(rbtree_eq_vec(
        rbtree, (Node[4]){csnode(BLACK, S(152)), csnode(BLACK, S(32)), csnode(RED, S(24)), csnode(BLACK, S(264))}, 4));
    return true;
}

int main() {
    const int HEAP_SIZE = 1 * 1024 * 1024;
    heap = malloc(HEAP_SIZE);
    mem_init(heap, HEAP_SIZE);

    TEST_LEFT_ROTATION();
    TEST_RIGHT_ROTATION();
    TEST_INSERTION();
    TEST_FIND();
    TEST_REMOVE();

    TEST_FROM_HELL();

    printf("Success B)\n");
}
