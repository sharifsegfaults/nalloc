## Setup

Using `nalloc` is very simple:

### MacOS

1. Clone this repo
2. Once inside the repo, you have two options:
    - To use `nalloc` with the original function names (`nalloc`, `nfree`, and `nrealloc`), run
    ```bash
    make dylib
    ```
    Which generates `libnalloc.dylib` in the top-level directory.
    - To use `nalloc` by replacing calls to `malloc` (and `free` and `realloc`) in your program, run
    ```bash
    make dylib-override
    ```
    Which generates `libnalloc-override.dylib` in the top-level directory.
3. A ".dylib" file will be generated in the repo's folder (top-most folder). You can now use this file and dynamically link it against any programs you want to run. A command you can use for this purpose is:
```bash
DYLD_INSERT_LIBRARIES=/absolute/path/to/libnallocORlibnalloc-override.dylib /path/to/your_program
```

You can also use this file while you compile another C/C++ program. Just include it in the compilation files:

```bash
gcc /path/to/libnallocORlibnalloc-override.dylib /path/to/your_program -o your_program
```

### Linux

Coming Soon!
