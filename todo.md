## TODO

- [X] Add logs to events (i.e., tree rotations, insertions, free's, coalescings, etc.)
- [X] Add a bunch of asserts -- also to rbtree?
- [X] Get rid of magic numbers
- [ ] Create crazy tests
- [ ] Beautify the code :)
- [ ] Fix mem leaks in fuzzer
- [ ] Change uint32_t for something more general
- [ ] Ghost node allows us to go branchless

### Postponed
- Create heapchecker:
    - There's never 2+ free blocks next to eachother
