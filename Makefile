FLAGS := -g

all: nalloc

mm_test: memlib.c mm.c rbtree.c mm_test.c
	gcc $^ -o $@.o

clean:
	rm -rf nalloc.o mm_test.o
