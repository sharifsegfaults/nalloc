## `nalloc` - design

This is a walk through some of the decisions and problems that led to `nalloc`'s design, and I hope it is helpful to anyone who is curious, or wants to know how this learning journey unfolded. Let's begin!

We first need to pick an allocation policy. In this case, it'll be best fit (i.e., to suffice a request, we try to find the block that fits it the "tightest").

To support this policy, we need to know, for a given size, what the best block would be, and thus, this becomes a search problem. When it comes to search, one data structure shines: Binary Search Trees (BSTs).

### How did we end up with red-black trees?

The ideal scenario for a BST is for it to be always balanced, as it guarantees its operations stay, in practice, within their asymptotic bounds (`O(log n)` for insertion, removal, and search). Making a BST balanced is a problem solved by AVL trees and Red-Black trees -- Binary Search Trees that impose some extra conditions in order to guarantee a notion of balance.

Personally, I have been more familiar with AVL trees (did not know about Red-Black trees at all before this project), but there's an interesting trade-off to consider:
- AVL trees require storing the height of each subtree in our tree. That is, for every node, we would need `sizeof(size_t)` bytes of space apart from any other information we want to store

- Red-Black trees require storing the color of a node, and thus, require us to store only 1 more bit per block.

It is clear the Red-Black trees make use of waaaay less memory that AVL trees while maintaining very similar performance in practice, so they became the data structure used for `nalloc`.

### Designing a Block of Memory

Let's for now imagine that our heap is 48 bytes long and is completely free. Let's also assume we are in a 32-bit architecture (this is just for the sake of fixing the size of the data types we'll be using).

We have in our hands a big block of continguous memory. When the user asks us for X amout of bytes (say, 12), we can give them a **block of memory** of that size (out of the free space we have available, give them a small section that is 12 bytes long).

Heap before (this is an oversimplification -- keep reading):
```
+----------------------------------------+
|                  48 B                  |
+----------------------------------------+
```
Heap after (this is an oversimplification -- keep reading):
```
+----------------------------------------+
|    12 B   |            36 B            |
+----------------------------------------+
```

This is completely fine... if you're only trying to perform a single allocation throughout the programs lifetime, because the moment a second allocation mcomes, we get in trouble: Suppose the user now asks us to allocate 24 bytes. How could we tell where this soon-to-be-given block should start? Notice that for us, it is very clear: it should start in the 13th byte, but remember: we haven't stored any information related to any previous allocation, so the program doesn't know this.

#### Our first pieces of metadata

Thus, we can write the size of a block of memory in its first bytes:
```
MEMORY BLOCK DESIGN (ASSUMING 32-bit ARCH.)
+----------------------------------------+
|    SIZE   |            DATA            |
| (4 Bytes) |          (X Bytes)         |
+----------------------------------------+
METADATA SIZE: 4 Bytes
```

Notice that we still need to know whether a given block is free or not (we don't want to reuse a block that's currently in use), and thus, we must also store this information in our block's header:

```
MEMORY BLOCK DESIGN (ASSUMING 32-bit ARCH.)
+--------------------------------------------------+
|   SIZE   | IS_FREE |            DATA             |
|  (4 B)   |  (1 B)  |            (X B)            |
+--------------------------------------------------+
METADATA SIZE: 5 Bytes
```

Perfect! Thus, a call to `nalloc` to allocate 12 bytes would look something like this:

Initial state of a 48 byte heap (we'll label the blocks with letters so that we can refer to them easily):

```
[                       A                          ]
+--------------------------------------------------+
|   43   | true |                                  |
+--------------------------------------------------+
```

> **NOTE:** But Sharif, didn't you say that the heap was 48 bytes long?... it is! but remember that now we are storing some metadata related to the block, in this case, we are using 4 bytes to store the size of the block, and another byte to store whether or not that block is free. Thus, our **block header** is 5 bytes long, so 48 B - 5 B = 43 B, which is why the size of the block is 43 bytes instead of 48.

- `nalloc(12)`:
```
+--------------------------------------------------+
[            A            ][          B            ]
[  12  | false |  (12 B)  ][  26  | true | (26 B)  ]
+--------------------------------------------------+
```

> Notice that, taking into account the size of the headers, all of the sizes now make sense

Great! Now, if the user calls something like `nalloc(16)`, our program would visit the first byte of the heap, see that said block is 12 bytes long and not free, so it would be able to skip to the next block, a 26-bytes block that's free, and thus can be used to satisfy the request.

#### Freedom unites us

Now imagine the user calls `nfree(A)`. What should happen? You may think that we just need to set the `is_free` value of `A` to be `true` and that's it... well... let's see what happens:

- `nfree(A)`
```
+--------------------------------------------------+
[            A            ][          B            ]
[  12  |  true |  (12 B)  ][  26  | true | (26 B)  ]
+--------------------------------------------------+
```

We now call `nalloc(30)` and we see it returns with an error: `Not enough memory`... why? There's clearly enough memory: 12 + 26 = 38, so we can definitely accomodate 30 bytes, no?

Well... think of things from the perspective of `nalloc`: the program sees two blocks, one free of size 12, and another one that's also free of size 26... there is no block of size +30 that's free.

You may right now be thinking: "Everything would be so easy if `nalloc` could just realize that these free blocks are next to each other, and thus the entire region is free!", and I agree! In fact, what we can do is we can **coalesce** these blocks together: merge them into a single, big block. Thus, we can modify the code so that, whenever a block is freed, it takes a look at the block to its right, and if it is also free, merge with it (why not look at the block on the left? -- keep reading)

Let's see how everything would look after this modification:

- `Initial State`:
```
+--------------------------------------------------+
[  A                      ][  B                    ]
[  12  | false |  (12 B)  ][  26  | true | (26 B)  ]
+--------------------------------------------------+
```
- `nfree(A)`
```
+--------------------------------------------------+
[  A                                               ]
[  43  | true |               (43 B)               ]
+--------------------------------------------------+
```
> **Note**: Why is this new block of size 43 if 12 + 26 = 38? the reason is simple: when the two blocks coalesce, we no longer need to keep track of the metadata of B, and thus, we can include those 5 bytes for use in the new block. 12 + 26 + 5 = 43.

Yay :D -- or well, not so fast. Consider this scenario:

- `Initial State`:
```
+--------------------------------------------------+
[  A                      ][  B                    ]
[  12  | false |  (12 B)  ][  26  | false | (26 B) ]
+--------------------------------------------------+
```

We have the same setup, but both blocks are allocated. Now consider this:

- `nfree(A)`:
```
+--------------------------------------------------+
[  A                      ][  B                    ]
[  12  | true  |  (12 B)  ][  26  | false | (26 B) ]
+--------------------------------------------------+
```
> No coalescing occurs yet, as expected

- `nfree(B)`:
```
+--------------------------------------------------+
[  A                      ][  B                    ]
[  12  | true  |  (12 B)  ][  26  | true | (26 B)  ]
+--------------------------------------------------+
```

Wow, why did the blocks not coalesce? Well, remember that we modified the code so that, whenever a block is freed, it tries to see if the block to its right is free, and if so, merges with it. We never look to our left.

Now, I deliberately mentioned that a just-freed block would check its right neighbour for coalescing, and not its left, and the reason is that looking at a block's left is, weirdly enough, impossible with our current design.

To see this, consider that on the call to `nfree(B)`, all the information the `nfree` function has is a pointer to the beginning of `B`. The function is not at all aware that there is a 12 byte block before `B`, nor does it know its size or anything like that (we can definitely see `B` has a block behind it, as we can see the entire heap, but remember: think from the perspective of the program).

You may say "well, can't we just go back 12 bytes (skipping the `data` section of `A`) and another 5 bytes (skipping the header of `A`), land at the beginning of block `A`, and then merge the blocks together?" and... yes, we could... if we knew what the size of `A`'s `data` section is, but, just by having a pointer to the beginning of `B`, it is impossible to know this: as far as `nfree` knows, the previous block could have a `data` section of 12, 32, or 1'000'000 bytes.

This is precisely what motivates the next change in the design of our memory block -- at the end of a block, we can store the block's size. That way, a block can travel 4 bytes back, read the size of its left block's `data` section, skip it, and then skip another 5 bytes (the header) and land at the beginning of its left block, and then perform coalescing.

```
MEMORY BLOCK DESIGN (ASSUMING 32-bit ARCH.)
+-------------------------------------------------------------+
|   SIZE   | IS_FREE |            DATA             |   SIZE   |
|  (4 B)   |  (1 B)  |           (X B)             |  (4 B)   |
+-------------------------------------------------------------+
METADATA SIZE: 9 Bytes
```

Great! This design, in fact, is enough to implement a simple memory allocator: whenever we try to satisfy a `nalloc` call, we can just jump from block to block until we find a block that's big enough to satisfy our request.

#### Speed. I am speed. One winner, 42 losers. I eat losers for breakfast.

Nonetheless, notice that this approach involves an `O(n)` search, where `n` is the number of blocks curently in our heap. We want something better and faster, and this is where the first section ("How did we end up with red-black trees?") comes into play: we can use a red-black tree to speed-up the search process. We thus can think of a given block of memory as our node in our red-black tree, and whenever we want to satisfy a request, we can travel down the red-black tree until we find the ideal block.

Now, that's beautiful and all, but there's something we need to worry about: every red-black tree node needs to store some information (left child, right child, parent, and color)... where do we put that information?

As you may already be suspecting, we can simply include it in the block's metadata, which gives us a new design for our block of memory shape:

```
MEMORY BLOCK DESIGN (ASSUMING 32-bit ARCH.)
+--------------------------------------------------------------------------+
|   SIZE   | IS_FREE | LEFT  | RIGHT | PARENT | COLOR |  DATA   |   SIZE   |
|  (4 B)   |  (1 B)  | (4 B) | (4 B) |  (4 B) | (1 B) | (X B)   |  (4 B)   |
+--------------------------------------------------------------------------+
METADATA SIZE: 22 Bytes
```

Great! This is, in fact, all of the data used by `nalloc` in each memory block.

...

Did you think it was over?

#### Decreasing the size of metadata

As I mentioned, this is all the data we need, but if you're an avid observer, you'll notice that the metadata size is extremely big in comparison to what we had before! As the ones in charge of managing the user's memory, we don't want to use too much memory in the process, and this is the complete opposite of that! For a call like `nalloc(16)` we wouldn't be using 16 bytes, but 16 + 22 = 38 bytes! More than double of what the user requested! Not only that, but as the amount of blocks increases, the amount of metadata increases too, so we want to keep metadata as lean as possible.

Thus, we now focus on reducing the size of metadata without losing any data.

##### The 1-bit fields

One of the first things we can easily notice is that there are some fields for which we could use a single bit (`is_free` --> a yes/no answer, or `color` --> either red or black) instead of 1 byte. We could make these fields be 1-bit long, but then we would be messing up the block's alignment, which we do not want (what do I mean by alignment? See [this](https://medium.com/@pawanwagh/understanding-memory-alignment-for-better-performance-3075787cfd3b)).

One of the requirements of our memory allocator is for it to return pointers that are aligned to a given value, which is usually 16 bytes (to make SIMD easier). This is a behavior also replicated by `malloc`, which returns pointers aligned to a given value (usually 8 bytes for 32-bit and 16 for 64-bit). In this example, let's assume we need to return 4-byte aligned pointers (everything we say will also apply to 8, 16, 32, etc., as you'll see).

Now that we have clarified this, notice that this restriction actually gives us a key insight: the size of a block must be a multiple of 4, otherwise, the next block will start at a non 4-byte aligned address. Because of being a multiple of 4, we know that the two least significant bits of a block's size field will be 0s, and thus, we can store `is_free` in the least significant bit! Applying the same logic allows us to store `color` in the least significant bit opf `parent`.

```
MEMORY BLOCK DESIGN (ASSUMING 32-bit ARCH.)
+-----------------------------------------------------------------------------------------------+
|   SIZE (31b) + IS_FREE (1b)   | LEFT  | RIGHT | PARENT (31b) + COLOR (1b) |   DATA  |   SIZE  |
|            (4 B)              | (4 B) | (4 B) |           (4 B)           |  (X B)  |  (4 B)  |
+-----------------------------------------------------------------------------------------------+
METADATA SIZE: 20 Bytes
```

Yay! We've decreased the metadata size by 2 bytes, *which, I know, is a very small amount*, but at least it's something! Nonetheless, we can take the savings further.

##### Do we really need all of this?

There are some metadata fields we brushed over: those related to the red-black tree. Remember that we need them to keep track of this block's node in the red-black tree that is storing all of our free blocks... and that is the key word: *free*. Notice that when a block is given to the user, we remove it from our red-black tree, and thus, that metadata is no longer useful or begin used -- thus, when giving the block back to the user, we can allow the user to override these metadata fields. This means that the metadata of a free block will differ from that of an allocated block, and thus we list both here:

```
IN-USE MEMORY BLOCK DESIGN (ASSUMING 32-bit ARCH.)
+-------------------------------------------------------------------------------------------+
|  SIZE (31b) + IS_FREE (1b)  |                        DATA                       |   SIZE  |
|            (4 B)            |                       (X B)                       |  (4 B)  |
+-------------------------------------------------------------------------------------------+
METADATA SIZE: 8 Bytes
```

```
FREE MEMORY BLOCK DESIGN (ASSUMING 32-bit ARCH.)
+-------------------------------------------------------------------------------------------+
|  SIZE (31b) + IS_FREE (1b)  | LEFT  | RIGHT | PARENT (31b) + COLOR (1b) | DATA  |   SIZE  |
|            (4 B)            | (4 B) | (4 B) |           (4 B)           | (X B) |  (4 B)  |
+-------------------------------------------------------------------------------------------+
METADATA SIZE: 20 Bytes
```

Wonderful! We have reduced the metadata size to 8 bytes, which is a massive improvement. Notice nonetheless that this imposes a limit: the minimum size a block can be. Since, once a block is freed, the first 12 bytes of its allocated`data` section will be used to reinstate the `left`, `right`, and `parent + color` fields, we need the `data` section of an allocated block to be at least 12 bytes.

>This value (the minimum size of a block's `data` section) will change as we optimize more. To make the reading easier, I will include this information under an allocated block's design. Notice also that this number is fundamentally dictated by the metadata size of a free block.

8 bytes is nice, but we can make things better...

##### Why do we need the size in the back anyways?

Remember that all of our metadata fields have a reason of being. In the case of the last metadata field, the `size` that is present in the block's footer, the reason was that we need the size of a block to be at its footer so that whenever the right block gets freed, it can take a look to its left, and, if we are free, merge with us. Notice the key phrase: ***if** we are free*.

Coalescing with the left block only occurs when the left block is also free, thus, we only need to take a look at the size of the block to our left whenever it is free (for the purpose of coalescing) since, if the block is allocated, we cannot even merge with it in the first place. Thus, we could make the last bit of our block represent whether or not the block is free, and allow the user to override the first 3 bytes of the footer, thus making the design of an in-use memory block become:

```
IN-USE MEMORY BLOCK DESIGN (ASSUMING 32-bit ARCH.)
+--------------------------------------------------------------------------------+
|   SIZE (31b) + IS_FREE (1b)  |                DATA              | IS_FREE (1b) |
|            (4 B)             |               (X B)              |    (1 B)     |
+--------------------------------------------------------------------------------+
METADATA SIZE: 5 Bytes
MINIMUM DATA SECTION SIZE: 15 Bytes
```

> It is important to notice that the "IS_FREE" flag on the footer occupies only 1 bit (represented by the "(1b)" to its side), it's just that we cannot give 0.875 bytes to the user, so we simply preserve the whole byte.

"But Sharif, we still need the size to be at the block's footer whenever the block is free so that others can coalesce with it" -- you're right, and that's why, whenever the user frees a given block, we simply reinstate the size in the footer (we use the last 4 bytes of the block -- last byte is where `IS_FREE` lives, and the 3 remaining needed bytes come from the last 3 bytes of the `data` section. Since the user just freed the block, they're not gonna use the `data` section anymore, so we can overwrite it with no trouble). Thus, for a free block, we have:

```
FREE MEMORY BLOCK DESIGN (ASSUMING 32-bit ARCH.)
+-----------------------------------------------------------------------------------------------------------+
| SIZE (31b) + IS_FREE (1b) | LEFT  | RIGHT | PARENT (31b) + COLOR (1b) | DATA  | SIZE (31b) + IS_FREE (1b) |
|           (4 B)           | (4 B) | (4 B) |          (4 B)            | (X B) |          (4 B)            |
+-----------------------------------------------------------------------------------------------------------+
METADATA SIZE: 20 Bytes
```
> Notice that we still need "IS_FREE" to be in the last bit, which is possible thanks to the observation that size must be a multiple of 4, and thus the last 2 bits of the size field will always be 0 (so we can use them to store other information)

5 bytes is an amazing quantity, but that "1 Byte" at the end of the block breaks all symmetry... can we somehow make it disappear?

##### Using symmetry as an excuse to improve metadata size

Life would be wonderful if we could just get rid of that last byte in an in-use block that is being used to store whether or not the block is free. You may think we could make it occupy a single bit, and although it is possible, it's just annoying, plus we cannot give to the user spare bits (as mentioned before) -- anything we give to the user needs to be in terms of bytes (so those 31 bits would still be there -- just occupying space and doing nothing).

This is where the last optimization comes into play: how about, in the header, instead of storing `IS_FREE` (which tells us whether or not the current block is free) we store `IS_PREV_FREE` which tells us whether or not the *previous* block is free -- this way, whenever we need to check if we can merge with the left block whenever we get freed, we can just look at out own header, without relying on the previous block's footer at all. We can still know whether or not a given block is free just by getting to its right block (which we've already established can be done solely with the information from the current block's header) and checking the right block's `IS_PREV_FREE`.

This, dear reader, is what allows us to have this beautiful, final memory block design:

```
IN-USE MEMORY BLOCK DESIGN (ASSUMING 32-bit ARCH.)
+----------------------------------------------------------------------------------------+
| SIZE(31b) + IS_PREV_FREE(1b) |                           DATA                          |
|            (4 B)             |                          (X B)                          |
+----------------------------------------------------------------------------------------+
METADATA SIZE: 4 Bytes
MINIMUM DATA SECTION SIZE: 16 Bytes
```

```
FREE MEMORY BLOCK DESIGN (ASSUMING 32-bit ARCH.)
+----------------------------------------------------------------------------------------+
| SIZE(31b) + IS_PREV_FREE(1b) | LEFT  | RIGHT | PARENT(31b) + COLOR(1b) | DATA  |  SIZE |
|          (4 B)               | (4 B) | (4 B) |         (4 B)           | (X B) | (4 B) |
+----------------------------------------------------------------------------------------+
METADATA SIZE: 20 Bytes
```

> If you are wondering "Who stores the prev_free of the last block?"... Easy! the first block!

That's it! we've done it! We transitioned from 22 bytes to 4 bytes! ISN'T THAT AMAIZING!??!?

### Conclusion

Thanks for reading this! I wanted to make clear all of the circumstances, problems, and decisions made in order to arrive at the current design of `nalloc`, as it is always interesting to see the "why?" behind these things, as well as a good resource and reference for some of you! It is also especially helpful for me, as this way I can improve my writing and explanations slowly but surely (I have a lot to improve, but hope that, by writing more and more, this ability will also improve).

Thank you for reading :D!
