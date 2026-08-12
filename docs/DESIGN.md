## `nalloc` - design

This is a walk through the process of designing `nalloc`

We first need to pick an allocation policy. In this case, it'll be best fit (i.e., to suffice a request, we try to find the block that fits it the "tightest").

To support this policy, we need to know, for a given size, what the best block would be, and thus, this becomes a search problem. When it comes to search, one data structure shines: Binary Search Trees (BSTs).

### How did we end up with red-black trees?

The ideal scenario for a BST is for it to be always balanced, as it guarantees its operations stay, in practice, within their asymptotic bounds (`O(log n)` for insertion, removal, and search). Making a BST balanced is a problem solved by AVL trees and Red-Black trees -- Binary Search Trees that impose some extra conditions in order to guarantee a notion of balance.

Personally, I have been more familiar with AVL trees (did not know about Red-Black trees at all before this project), but there's an interesting trade-off to consider:
- AVL trees require storing the height of each subtree in our tree. That is, for every node, we would need `sizeof(size_t)` bytes of space apart from any other information we want to store

- Red-Black trees require storing the color of a node, and thus, require us to store only 1 more bit per block.

It is clear the Red-Black trees make use of waaaay less memory that AVL trees, so they became the data structure used for `nalloc`.

### Designing a Block of Memory

Let's for now imagine that our heap is 48 bytes long and is completely free.

We have in our hands a big block of continguous memory. When the user asks us for X amout of bytes (say, 12), we can give them a **block of memory** of that size (out of the free space we have available, give them a small section that is 12 bytes long).

Heap Before:
|            48            |
|   12   |       36        |

This is completely fine... if you're only trying to perform a single allocation throughout the programs lifetime, because the moment a second allocation mcomes, we get in trouble: Suppose the user now asks us to allocate 24 bytes. How could we tell where this soon-to-be-given block should start? Notice that for us, it is very clear: it should start in the 12th byte, but remember: we haven't stored any information related to any previous allocation, so the program doesn't know this.

Thus, we can write the size of a block of memory in its first bytes:

| size |        user info       |

Notice that we still need to know whether a given block is free or not (we don't want to reuse a block that's currently in use), and thus, we must also store this information in our block's header:

| size | is_free |   user info   |

Perfect! Thus, a call to `nalloc` to allocate 12 bytes would look something like this:

Initial state of the heap (why 43 instead of 48? keep reading):
[ 43 | true |               (43 bytes)              ]

- `nalloc(12)`:
[ 12 | false | (12 bytes) ][ 31 | true | (31 bytes) ]

But Sharif, didn't you say that the heap was 48 bytes long?... it is! to discover why the initial state was a 

