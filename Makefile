FLAGS := -g

all: nalloc

mm_test: memlib.c mm.c rbtree.c mm_test.c
	gcc $^ -o bins/$@ ${FLAGS}

mm_test_random: memlib.c mm.c rbtree.c mm_test_random.c stivec.c
	gcc $^ -o bins/$@ ${FLAGS}

rbtree_test: memlib.c mm.c rbtree.c rbtree_test.c
	gcc $^ -o bins/$@ ${FLAGS}

clean:
	rm -rf bins/mm_test bins/rbtree_test
