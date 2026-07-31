FLAGS := -Ilib -Iinclude -Isrc -g

test: fuzzer rbtree_test

dylib: src/memlib.c src/mm.c src/lib/containers/rbtree.c
	gcc -dynamiclib $^ -o libnalloc.dylib ${FLAGS}

fuzzer: src/memlib.c src/mm.c src/lib/containers/rbtree.c src/lib/containers/stivec.c tests/fuzzer.c
	mkdir -p build/tests
	gcc $^ -o build/tests/$@ ${FLAGS}

rbtree_test: src/memlib.c src/mm.c src/lib/containers/rbtree.c tests/rbtree_test.c
	mkdir -p build/tests
	gcc $^ -o build/tests/$@ ${FLAGS}

clean:
	rm -rf build/*
