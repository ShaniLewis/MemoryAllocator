#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>

#include "my_mem.h"

// This evaluates to the maximum number of bits the OS can possibly address.
// On a 32-bit system, a pointer can point to 4 bytes = 32 bits (4*8).
// On a 64-bit system a pointer can point to 8 bytes = 64 bits (8*8). 
// This allows the memory manager to use a statically allocated array to track any 
// and all memory allocations possible on this system
 
#define MAX_OS_POW2 (sizeof(void *) * 8)

// block is a doubly linked list node, and also stores which list it is in
typedef struct Block {              
    struct Block *next, *prev;      // *next and *prev point to next and prev blocks in DLL 
    size_t size_pow2;               // size of memory we want to store (not including block header pointers)
} Block;

// all data associated with the memory and its state
struct memory_manager_state {
    unsigned char *memory;  
    size_t mem_size;        
    size_t max_pow2;         
    
     // parallel arrays for storing the head of each doubly-linked pointer header list,
     // and the number of elements in each list. Each index represents the
     // power of 2 size of the block each header begins (this size includes the
     // size of the header)
    Block *free[MAX_OS_POW2];  
    Block *used[MAX_OS_POW2];  
    int num_free[MAX_OS_POW2]; // count the blocks at a specific index
    int num_used[MAX_OS_POW2]; 
} state;

// initialize a free block of memory and then push it to the proper free list
static void push_new_block(Block *block, size_t size_pow2) {
    block->prev = NULL;                     // set prev pointer to null
    block->next = state.free[size_pow2];    // set next pointer to *free list of corresponding pow2
    block->size_pow2 = size_pow2;           // set block size to level it is on free list

    if (block->next != NULL)                
        block->next->prev = block;          

    state.free[size_pow2] = block;           
    ++state.num_free[size_pow2];            
};

// Removes the head of a free list and returns it. 
// Ensure there is a block available before using this 
static Block *pop_free_block(size_t size_pow2) {
    Block *block = state.free[size_pow2]; 

    state.free[size_pow2] = block->next;
    --state.num_free[size_pow2];

    if (block->next != NULL)        // if next is not pointing to null b/c we are inserting the block a
        block->next->prev = NULL;   // set next and prev to null to make it free

    return block;
}

// removes a free block from its list
static void remove_free_block(Block *block) {
    if (block->prev != NULL) {                  
        block->prev->next = block->next;        
    } else {                                    
        // block is list head
        state.free[block->size_pow2] = block->next;  
    }                                                                      

    if (block->next != NULL)
        block->next->prev = block->prev;

    --state.num_free[block->size_pow2];
}

// splits a block of size_pow2 into 2 blocks of half the size
static void split_free_block(size_t size_pow2) {
    Block *block = pop_free_block(size_pow2);
    size_t next_size = size_pow2 - 1;
    Block *block2 = (Block *)(((unsigned char *)block) + (1 << next_size));

    push_new_block(block, next_size);
    push_new_block(block2, next_size);
}


// pops a block from free list to used list and returns
static Block *alloc_block(size_t size_pow2) {
    Block *block = pop_free_block(size_pow2);

    block->prev = NULL;
    block->next = state.used[size_pow2];

    if (block->next != NULL)
        block->next->prev = block;

    state.used[size_pow2] = block;
    ++state.num_used[size_pow2];

    return block;
}


// given two blocks, attempts to merge them. returns if it was successful.
static bool try_merge(Block *a, Block *b) {
    // ensure a is before b
    if (b < a) {
        Block *tmp = b;
        b = a;
        a = tmp;
    }

    // if a is aligned to the boundary of the next higher block size, 
    // meaning it could be the first block of a split block
    size_t diff = (unsigned char *)a - (unsigned char *)state.memory;
    bool properly_aligned = diff % (1 << (a->size_pow2 + 1)) == 0;

    // if a is directly before b
    bool adjacent = (Block *)((unsigned char *)a + (1 << a->size_pow2)) == b;

    if (properly_aligned && adjacent) {
        // blocks are mergeable, so merge them
        remove_free_block(a);
        remove_free_block(b);
        push_new_block(a, a->size_pow2 + 1);

        return true;
    }

    return false;
}

// given a free list that has recently had a block pushed, attempt to defragment it
static void try_defrag(size_t size_pow2) {
    if (state.num_free[size_pow2] >= 2) {
        Block *head = state.free[size_pow2];

        for (Block *trav = head->next; trav; trav = trav->next) {
            if (try_merge(trav, head)) {
                // if the merge was successful, try to defrag the next list up
                // and stop iterating
                try_defrag(size_pow2 + 1);
                break;
            }
        }
    }
}


// given an allocated block on a used list, deletes it from its list and push
// it back on a free list for later usage
static void free_block(Block *block) {
    // remove this block from its used list
    if (block->prev != NULL) {
        block->prev->next = block->next;
    } else {
        // block is the head of the used list, replace it
        state.used[block->size_pow2] = block->next;
    }

    if (block->next != NULL)
        block->next->prev = block->prev;

    --state.num_used[block->size_pow2];

    // push it to the free list
    push_new_block(block, block->size_pow2);
    try_defrag(block->size_pow2);
}


// split starting memory into viable power-of-2 sized chunks for usage, and
//push them onto the free list
static void split_initial_memory(void) {
    unsigned char *mem = state.memory;

    // pow2 cannot be larger than the max_pow2 free, and must accommodate a
    // Block plus extra space. iterate through these bits
    for (size_t i = state.max_pow2; (1 << i) > sizeof(Block); --i) {
        // if memory can be split by this power of 2, split it
        if (state.mem_size & (1 << i)) {
            push_new_block((Block *)mem, i);
            mem += (1 << i);
        }
    }
}

void mem_init(unsigned char *my_memory, unsigned int my_mem_size) {
    state = (struct memory_manager_state){0};

    state.memory = my_memory;
    state.mem_size = my_mem_size;

    // calculate maximum pow2 
    while ((my_mem_size >>= 1))  // ex: 1024
        ++state.max_pow2;        // max_pow2 = 10 =(1,2,4,8,16,32,64,128,256,512)

    split_initial_memory();
}

// resets the memory state for testing
void mem_reset(void) {
    unsigned char *memory = state.memory;
    unsigned int mem_size = state.mem_size;

    state = (struct memory_manager_state){0};

    mem_init(memory, mem_size);
}

// the maximum possible allocation
unsigned mem_max_alloc(void) {
    return state.mem_size - sizeof(Block); 
}

 // this returns the minimum power of 2 required to store
 // (n + sizeof(Block)) bytes, AKA the smallest block needed
static size_t smallest_block(size_t n) {
    n += sizeof(Block);

    // find max power of 2 n requires
    size_t res = 0;

    while ((n >> (res + 1)) > 0) // 
        ++res;

    // if n also requires extra space, give it
    if (n > (1 << res))
        ++res;

    return res;
}

void *my_malloc(unsigned size) {
    // special case, return NULL for size == 0
    if (size == 0)
        return NULL;

    // find smallest pow2 required for this size
    size_t req_pow2 = smallest_block(size);

    // find smallest block actually available that accommodates this size
    size_t best_free = req_pow2;

    while (state.num_free[best_free] == 0) {
        // if no blocks are available, return NULL
        if (++best_free > state.max_pow2)
            return NULL;
    }

    // split blocks until one is available that equals the required size
    while (best_free > req_pow2)
        split_free_block(best_free--);

    // allocated block and return a pointer to the memory located after the
    // block header
    return (void *)(alloc_block(req_pow2) + 1);
}

// if pointer isn't null, free it from the used list it is on
void my_free(void *mem_pointer) {
    if (mem_pointer != NULL) {
        // subtract the size of one block to get the header
        free_block(((Block *)mem_pointer) - 1);
    }
}

// size of a pow2 block for stats
static int stats_block_size(size_t size_pow2) {
    return (1 << size_pow2) - sizeof(Block);
}

void mem_get_stats(mem_stats_ptr mem_stats_ptr) {
    *mem_stats_ptr = (mem_stats_struct){0};

    // count blocks
    for (size_t i = 0; i <= state.max_pow2; ++i) {
        mem_stats_ptr->num_blocks_used += state.num_used[i];
        mem_stats_ptr->num_blocks_free += state.num_free[i];
    }

    // used blocks
    if (mem_stats_ptr->num_blocks_used > 0) {
        for (size_t i = 0; i <= state.max_pow2; ++i) {
            if (state.num_used[i] > 0) {
                mem_stats_ptr->smallest_block_used = stats_block_size(i);
                break;
            }
        }

        for (size_t i = state.max_pow2; i >= 0; --i) {
            if (state.num_used[i] > 0) {
                mem_stats_ptr->largest_block_used = stats_block_size(i);
                break;
            }
        }
    }

    // free blocks
    if (mem_stats_ptr->num_blocks_free > 0) {
        for (size_t i = 0; i <= state.max_pow2; ++i) {
            if (state.num_free[i] > 0) {
                mem_stats_ptr->smallest_block_free = stats_block_size(i);
                break;
            }
        }

        for (size_t i = state.max_pow2; i >= 0; --i) {
            if (state.num_free[i] > 0) {
                mem_stats_ptr->largest_block_free = stats_block_size(i);
                break;
            }
        }
    }
}

void mem_print(void) {
    puts("--- memory ---");

    printf(
        "%-2s | %-12s | %-6s | %-6s\n",
        "n", "true size", "free", "used"
    );

    for (size_t i = 0; i <= state.max_pow2; ++i) {
        printf(
            "%2zu | %12d | %6d | %6d\n",
            i, (1 << i), state.num_free[i], state.num_used[i]
        );
    }

    // assumes allocated blocks have valid strings in them
    puts("--- used ---");

    for (size_t i = 0; i <= state.max_pow2; ++i)
        for (Block *trav = state.used[i]; trav; trav = trav->next)
            printf("%2zu: %s\n", i, (char *)(trav + 1));
}
