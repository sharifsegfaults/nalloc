FLAGS := -g -DDEBUG

all: nalloc

mm_test: memlib.c mm.c rbtree.c mm_test.c
	gcc $^ -o bins/$@ ${FLAGS}

rbtree_test: memlib.c mm.c rbtree.c rbtree_test.c
	gcc $^ -o bins/$@ ${FLAGS}

clean:
	rm -rf bins/mm_test bins/rbtree_test
