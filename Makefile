# TODO: Use Unity
FLAGS := -g -I ./vendor/unity/src

all: mm_test

mm_test: memlib.c mm_test.c mm.c rbtree.c vendor/unity/src/unity.c
	gcc $^ -o bins/$@ ${FLAGS}

clean:
	rm -rf mm_test
