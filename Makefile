FLAGS := -Ilib -Iinclude -Isrc -g

test: mm_test_random rbtree_test

mm_test: src/memlib.c src/mm.c src/rbtree.c tests/mm_test.c
	mkdir -p build/tests
	gcc $^ -o build/tests/$@ ${FLAGS}

mm_test_random: src/memlib.c src/mm.c src/rbtree.c tests/mm_test_random.c src/lib/containers/stivec.c
	mkdir -p build/tests
	gcc $^ -o build/tests/$@ ${FLAGS}

rbtree_test: src/memlib.c src/mm.c src/rbtree.c tests/rbtree_test.c
	mkdir -p build/tests
	gcc $^ -o build/tests/$@ ${FLAGS}

clean:
	rm -rf build/*
