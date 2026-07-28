#include <unistd.h>

#define MAX_HEAP (20 << 20)

void mem_init(char* heap, uint32_t og_size);               
void mem_deinit(void);
void *mem_sbrk(int incr);
void mem_reset_brk(void); 
void *mem_heap_lo(void);
void *mem_heap_hi(void);
size_t mem_heapsize(void);
size_t mem_pagesize(void);
