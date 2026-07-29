FLAGS := -Ilib -Iinclude -Isrc -g

test: fuzzer rbtree_test

fuzzer: src/memlib.c src/mm.c src/rbtree.c src/lib/containers/stivec.c tests/fuzzer.c
	mkdir -p build/tests
	gcc $^ -o build/tests/$@ ${FLAGS}

rbtree_test: src/memlib.c src/mm.c src/rbtree.c tests/rbtree_test.c
	mkdir -p build/tests
	gcc $^ -o build/tests/$@ ${FLAGS}

clean:
	rm -rf build/*
