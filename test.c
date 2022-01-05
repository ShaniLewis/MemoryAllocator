#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/time.h>

#include "my_mem.h"

typedef struct test_function {
    bool (*func)(void);
    const char *name;
} test_function;

static double now(void) {
    struct timeval tv;

    gettimeofday(&tv, NULL);

    return (double)tv.tv_sec + (double)tv.tv_usec / 100000.0;
}

// try to allocate all the memory at once
static bool test_max_alloc(void) {
    mem_stats_struct stats;
    void *mem = my_malloc(mem_max_alloc());

    mem_get_stats(&stats);

    return mem != NULL && stats.num_blocks_used == 1;
}

// try to allocate too much memory
static bool test_too_much_memory(void) {
    void *mem = my_malloc(mem_max_alloc() + 1);

    return mem == NULL;
}

// allocate 20 strings and then free them
static bool test_many_allocs(void) {
#define NUM_ALLOCS 10000
    mem_stats_struct stats;

    void *ptrs[NUM_ALLOCS];

    for (size_t i = 0; i < NUM_ALLOCS; ++i) {
        ptrs[i] = my_malloc(1);

        if (ptrs[i] == NULL)
            return false;
    }

    mem_get_stats(&stats);

    if (stats.num_blocks_used != NUM_ALLOCS)
        return false;

    for (size_t i = 0; i < NUM_ALLOCS; ++i)
        my_free(ptrs[i]);

    mem_get_stats(&stats);

    return stats.num_blocks_used == 0 && stats.num_blocks_free == 1;
#undef NUM_ALLOCS
}

// my_malloc should fail eventually
static bool test_too_many_allocs(void) {
    for (size_t i = 0; i < 1000; ++i)
        if (my_malloc(10000) == NULL)
            return true;

    return false;
}

// allocate the same block over and over again
static bool test_one_block(void) {
    mem_stats_struct stats;

    for (size_t i = 0; i < 100; ++i) {
        void *test = my_malloc(1000);

        if (test == NULL)
            return false;

        my_free(test);
    }

    mem_get_stats(&stats);

    return stats.num_blocks_used == 0;
}

static bool test_malloc_zero(void) {
    mem_stats_struct stats;
    void *test = my_malloc(0);

    mem_get_stats(&stats);

    return test == NULL && stats.num_blocks_used == 0;
}

static bool test_free_null(void) {
    mem_stats_struct stats;
    mem_get_stats(&stats);

    int prev_free = stats.num_blocks_free;

    my_free(NULL);
    mem_get_stats(&stats);

    return stats.num_blocks_free == prev_free;
}

static bool test_random_sizes(void) {
    mem_stats_struct stats;
    srand(now());

    for (size_t i = 0; i < 10000; ++i) {
        unsigned size = rand() % mem_max_alloc();

        void *test = my_malloc(size);

        mem_get_stats(&stats);

        if (test == NULL || stats.num_blocks_used != 1)
            return false;

        my_free(test);
    }

    mem_get_stats(&stats);

    return stats.num_blocks_used == 0 && stats.num_blocks_free == 1;
}

int main() {
    // init
    unsigned int mem_size = 1024 * 1024;
    unsigned char *memory = malloc(mem_size);

    mem_init(memory, mem_size);

    // run tests
    test_function tests[] = {
        { test_max_alloc, "maximum allocation" },
        { test_too_much_memory, "too much memory" },
        { test_many_allocs, "many allocs" },
        { test_too_many_allocs, "too many allocs" },
        { test_one_block, "one block" },
        { test_malloc_zero, "malloc zero" },
        { test_free_null, "free null" },
        { test_random_sizes, "random sizes" },

        { NULL, "end of test array" }
    };

    for (test_function *trav = tests; trav->func; ++trav) {
        double test_start = now();
        bool success = trav->func();
        double time_taken = now() - test_start;

        printf(
            "[ %s ] - %s in %fs\n",
            trav->name,
            success ? "success" : "failure",
            time_taken
        );

        mem_reset();
    }

    // cleanup
    free(memory);

    return 0;
}
