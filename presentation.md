# Content
## What did you learn?

This assignment required me to become familiar with pointers and how to use them to manipulate data structures. My program made heavy use of doubly linked lists, which contain a lot of edge cases and pointer manipulation.

This assignment also required me to think hard about the memory layout of my program. In languages that manage memory for you, like Python or Java, you don't have to understand what the stack or the heap is to write useful programs. In order to avoid using heap allocation, I had to think in new ways to solve problems. For example, using static arrays to store the data for all of the memory blocks instead of using Python or Java's lists.

## What was the most fun part?

The most fun part of writing this was figuring out how the memory manager would work before I even wrote any of the code. When I started this assignment I had absolutely no idea how anyone would make this kind of program in the first place, and I knew that I would need to solve a lot of smaller problems in order to have a working product.

One of these problems I faced was figuring out how to manage data associated with each individual block. By reading about how other people have implemented their own memory managers, I learned about a trick where you can place data right before the address of a pointer. This way, I could store data for the `my_free` function to use while anyone using `my_malloc` doesn't see anything besides a raw pointer. Very cool!

## What was the hardest part?

The hardest part was definitely wrapping my head around pointers and pointer math. Before this assignment, pointers seemed incredibly mysterious and hard to understand. It was really hard to get doubly linked lists to work at first, and I had a lot of trouble visualizing what was going on in my code. After spending enough time working with these things, I am definetly more comfortable with pointers. 

# Performance Analysis

My memory manager implementation splits memory blocks in half until it can't anymore, in order to allocate the smallest possible chunk it can. When freeing blocks, it attempts to merge them together into the largest possible chunk it can. These procedures of splitting and merging are the slowest. 

## Best Case Performance

- `my_malloc` is fastest when there is a block already available that fits the size of bytes requested, this is a constant time operation.
- `my_free` is fastest when a block that is freed cannot be merged. This time is linear, relative to the size of the free list the block is merging into.

## Worst Case Performance

- `my_malloc` is slowest when a very small block is requested, but only a very large block exists. This requires the large block to be split multiple times, which requires logarithmic time relative to the difference in block sizes.
- `my_free` is slowest when a very small block was allocated and a very large block was split many times to form it. This requires logarithmic time to merge blocks, relative to the difference in block sizes, multiplied by the linear time for the size of the free lists.

## Average Case Performance

- `my_malloc` typically requires a large number of block splitting to be performed at the first call, and then not many splits are typically required.
- `my_free` typically can only merge many blocks after all of the blocks have been freed. The last call is typically the slowest, and not many merges are typically required.
