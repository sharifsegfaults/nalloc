FLAGS := -Ilib -Iinclude -Isrc
RFLAGS := ${FLAGS} -DNDEBUG
DFLAGS := ${FLAGS} -g

test: fuzzer rbtree_test

dylib: src/memlib.c src/mm.c src/lib/containers/rbtree.c
	gcc -dynamiclib $^ -o libnalloc.dylib ${RFLAGS}

dylib-override: src/m
emlib.c src/mm.c src/lib/containers/rbtree.c src/mm_override.c
	gcc -dynamiclib $^ -o libnalloc-override.dylib ${RFLAGS}

fuzzer: src/memlib.c src/mm.c src/lib/containers/rbtree.c src/lib/containers/stivec.c tests/fuzzer.c
	mkdir -p build/tests
	gcc $^ -o build/tests/$@ ${DFLAGS}

rbtree_test: src/memlib.c src/mm.c src/lib/containers/rbtree.c tests/rbtree_test.c
	mkdir -p build/tests
	gcc $^ -o build/tests/$@ ${DFLAGS}

clean:
	rm -rf build/*
