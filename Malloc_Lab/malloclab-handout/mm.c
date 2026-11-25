/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

team_t team = {
    "lvokic",
    "lvokic",
    "2792779840@qq.com",
    "",
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))
#define HEADER_SIZE (ALIGN(sizeof(size_t)))
#define FOOTER_SIZE (ALIGN(sizeof(size_t)))

static char *rover = NULL;

static char *header_of(void *ptr)
{
    return (char *)ptr - HEADER_SIZE;
}

static size_t block_size(void *ptr)
{
    uint64_t header = *(uint64_t *)header_of(ptr);
    return (size_t)(header & ~(uint64_t)0x7);
}

static size_t block_size_from_ptr(void *ptr)
{
    uint64_t header = *(uint64_t *)(ptr);
    return (size_t)(header & ~(uint64_t)0x7);
}

static int block_allocated(void *ptr)
{
    uint64_t header = *(uint64_t *)header_of(ptr);
    return (header & 1) != 0;
}

static int allocated(void *ptr) {
    uint64_t header = *(uint64_t *)(ptr);
    return (header & 1) != 0;
}

static char *footer_of(void *ptr)
{
    return header_of(ptr) + block_size(ptr) - FOOTER_SIZE;
}

/* Merge a free block with preceding free blocks; return the new payload. */
static void *coalesce_forward(void *cur_ptr)
{
    char *heap_start = mem_heap_lo();
    for (;;) {
        char *cur_header = header_of(cur_ptr);
        /* A footer before the heap is not a valid previous block. */
        if ((size_t)(cur_header - heap_start) < FOOTER_SIZE) break;
        char *prev_footer = cur_header - FOOTER_SIZE;

        if (allocated(prev_footer)) break;
        size_t prev_size = block_size_from_ptr(prev_footer);
        if (prev_size < HEADER_SIZE + FOOTER_SIZE ||
            prev_size % ALIGNMENT != 0 ||
            prev_size > (size_t)(cur_header - heap_start))
            break;
            
        char *prev_header = cur_header - prev_size;
        char *cur_footer = footer_of(cur_ptr);
        size_t new_size = prev_size + block_size(cur_ptr);

        *(uint64_t *)prev_header = (uint64_t)new_size;
        *(uint64_t *)cur_footer = (uint64_t)new_size;
        cur_ptr = prev_header + HEADER_SIZE;
    }
    return cur_ptr;
}

static void *coalesce_afterward(void *cur_ptr)
{
    char *heap_end = mem_heap_hi() + 1;
    for (;;) {
        char *cur_footer = footer_of(cur_ptr);
        char *next_header = cur_footer + FOOTER_SIZE;
        if (next_header >= heap_end) break;

        size_t remaining = (size_t)(heap_end - next_header);
        if (remaining < HEADER_SIZE) break;

        if (allocated(next_header)) break;
        size_t next_size = block_size_from_ptr(next_header);
        if (next_size < HEADER_SIZE + FOOTER_SIZE ||
            next_size % ALIGNMENT != 0 ||
            next_size > remaining)
            break;

        char *next_footer = next_header + next_size - FOOTER_SIZE;
        char *cur_header = header_of(cur_ptr);
        size_t new_size = next_size + block_size(cur_ptr);

        *(uint64_t *)next_footer = (uint64_t)new_size;
        *(uint64_t *)cur_header = (uint64_t)new_size;
    }
    return cur_ptr;
}

static void* find_fit(size_t required_size) {
    char* heap_start = mem_heap_lo();
    char* heap_end = heap_start + mem_heapsize();
    
    char* header = heap_start;
    while (header < heap_end) {
        char* ptr = header + HEADER_SIZE;
        if (!block_allocated(ptr) && block_size(ptr) >= required_size) {
            return ptr;
        }
        header += block_size(ptr);
    }
    return NULL;
}

static void* next_fit(size_t required_size) {
    char* heap_start = mem_heap_lo();
    char* heap_end = heap_start + mem_heapsize();
    if (heap_start == heap_end)
        return NULL;
    if (rover == NULL || rover >= heap_end)
        rover = heap_start;
    char* start = rover;
    while (1) {
        char* ptr = rover + HEADER_SIZE;
        if (!block_allocated(ptr) && block_size(ptr) >= required_size) {
            return ptr;
        }
        rover += block_size(ptr);
        if (rover >= heap_end)
            rover = heap_start;
        if (rover == start)
            break;
    }
    return NULL;
}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    rover = NULL;
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    if (size == 0 ||
        size > SIZE_MAX - HEADER_SIZE - FOOTER_SIZE - (ALIGNMENT - 1))
        return NULL;

    size_t total_size = (size + HEADER_SIZE + FOOTER_SIZE + (ALIGNMENT - 1))
                          & ~(size_t)(ALIGNMENT - 1);
    if (total_size > INT_MAX)
        return NULL;
    void* block = next_fit(total_size);
    if (block != NULL) {
        size_t original_size = block_size(block);
        size_t remaining     = original_size - total_size;
        size_t allocated_size = original_size;
        if (remaining >= HEADER_SIZE + FOOTER_SIZE + ALIGNMENT) {
            char *remaining_ptr = (char *)block + total_size;
            uint64_t remaining_value = (uint64_t)remaining;
            *(uint64_t *)header_of(remaining_ptr) = remaining_value;
            *(uint64_t *)footer_of(remaining_ptr) = remaining_value;
            allocated_size = total_size;
        }
        uint64_t header_value = (uint64_t)allocated_size | 1;
        *(uint64_t *)header_of(block) = header_value;
        *(uint64_t *)footer_of(block) = header_value;
        return block;
    } else {
        block = mem_sbrk((int)total_size);
        if (block == (void *)-1)
            return NULL;
        uint64_t header_value = (uint64_t)total_size | 1;
        *(uint64_t *)block = header_value;
        *(uint64_t *)(block + total_size - FOOTER_SIZE) = header_value;
        return block + HEADER_SIZE;
    }
}

/*
 * mm_free - Mark a block free and merge adjacent free blocks.
 */
void mm_free(void *ptr)
{
    if (ptr == NULL)
        return;
    rover = NULL;
    char* header = header_of(ptr);
    char* footer = footer_of(ptr);
    uint64_t free_value = (uint64_t)block_size(ptr);
    *(uint64_t *)header = free_value;
    *(uint64_t *)footer = free_value;
    ptr = coalesce_forward(ptr);
    coalesce_afterward(ptr);
}

/*
 * mm_realloc - Grow in place when possible, otherwise allocate and copy.
 */
void *mm_realloc(void *ptr, size_t size)
{
    if (ptr == NULL)
        return mm_malloc(size);
    if (size == 0) {
        mm_free(ptr);
        return NULL;
    }

    size_t old_size = block_size(ptr);
    size_t old_capacity = old_size - HEADER_SIZE - FOOTER_SIZE;
    if (size <= old_capacity)
        return ptr;

    if (size > SIZE_MAX - HEADER_SIZE - FOOTER_SIZE - (ALIGNMENT - 1))
        return NULL;
    size_t required_size = (size + HEADER_SIZE + FOOTER_SIZE + (ALIGNMENT - 1))
                           & ~(size_t)(ALIGNMENT - 1);
    if (required_size > INT_MAX)
        return NULL;

    char *header = header_of(ptr);
    char *heap_end = (char *)mem_heap_lo() + mem_heapsize();
    char *next_header = header + old_size;
    size_t available_size = old_size;

    /* Include the next physical block when it is free. */
    if (next_header < heap_end && !allocated(next_header))
        available_size += block_size_from_ptr(next_header);

    /* If the combined space reaches the heap end, grow it in place. */
    if (available_size < required_size && header + available_size == heap_end) {
        size_t extra = required_size - available_size;
        if (extra <= INT_MAX && mem_sbrk((int)extra) != (void *)-1)
            available_size = required_size;
    }

    if (available_size >= required_size) {
        size_t allocated_size = available_size;
        size_t remaining = available_size - required_size;

        if (remaining >= HEADER_SIZE + FOOTER_SIZE + ALIGNMENT) {
            char *remaining_header = header + required_size;
            *(uint64_t *)remaining_header = (uint64_t)remaining;
            *(uint64_t *)(remaining_header + remaining - FOOTER_SIZE) =
                (uint64_t)remaining;
            allocated_size = required_size;
        }

        uint64_t header_value = (uint64_t)allocated_size | 1;
        *(uint64_t *)header = header_value;
        *(uint64_t *)(header + allocated_size - FOOTER_SIZE) = header_value;

        /* The next-fit cursor may have pointed inside the merged block. */
        rover = NULL;
        return ptr;
    }

    void *new_ptr = mm_malloc(size);
    if (new_ptr == NULL) return NULL;

    size_t copy_size = old_capacity;
    memcpy(new_ptr, ptr, copy_size);
    mm_free(ptr);
    
    return new_ptr;
}














