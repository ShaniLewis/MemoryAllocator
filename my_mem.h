#ifndef MY_MEM_H
#define MY_MEM_H

typedef struct {
    int num_blocks_used;
    int num_blocks_free;
    int smallest_block_free;
    int smallest_block_used;
    int largest_block_free;
    int largest_block_used;
} mem_stats_struct, *mem_stats_ptr;

void mem_init(unsigned char *my_memory, unsigned int my_mem_size);

// returns valid pointer, or NULL if allocation failed or size is zero
void *my_malloc(unsigned size);


// given a valid pointer allocated using my_malloc, frees the memory associated
// with it. if the pointer was not allocated using my_malloc, this will likely
// produce a segmentation fault. if the pointer is NULL, it will do nothing
void my_free(void *mem_pointer);

// fills in stats struct
void mem_get_stats(mem_stats_ptr mem_stats_ptr);

// debugging + testing functions
void mem_reset(void);
unsigned mem_max_alloc(void);
void mem_print(void);

#endif
