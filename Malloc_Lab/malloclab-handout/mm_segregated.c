/*
 * 分离空闲链表分配器脚手架。
 *
 * 块布局：
 *   已分配块：[header][payload / padding][footer]
 *   空闲块：  [header][prev][next][空闲空间][footer]
 *
 * header 和 footer 各占 8 字节；块大小和 payload 地址按 8 字节对齐。
 * 64 位下两个链表指针各占 8 字节，因此最小块为 32 字节。
 * 指针约定：链表中的指针指向块的 payload 起点。
 *
 * 按整个块大小分类：上界依次为 32、64、128、...、16 MiB，
 * 最后一类保存大于 16 MiB 的块。链表头数组应存放在模拟堆中，
 * 通过全局指针访问，以遵守 handout 对全局复合数据结构的限制。
 *
 * TODO：设计堆起点与链表头区域，确保块遍历和合并不会进入头指针区域。
 * TODO：size_class(size) 根据块大小选择类别。
 * TODO：insert_free_block(ptr) 和 remove_free_block(ptr) 管理链表。
 * TODO：find_fit(size)、place(ptr, size)、extend_heap(size) 和 coalesce(ptr)。
 * TODO：实现堆一致性检查，核对块和空闲链表的状态。
 */

#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

#define ALIGNMENT 8
#define HEADER_SIZE 8
#define FOOTER_SIZE 8
#define MIN_BLOCK_SIZE (HEADER_SIZE + 2 * sizeof(void *) + FOOTER_SIZE)
#define NUM_SIZE_CLASSES 21

team_t team = {
    "lvokic",
    "lvokic",
    "2792779840@qq.com",
    "",
    ""
};

/* 链表头位于模拟堆；blocks_start 指向普通块区域的起点。 */
static void **free_lists;
static char *blocks_start;

static char *header_of(void *ptr)
{
    return (char *)ptr - HEADER_SIZE;
}

static size_t block_size(void *ptr)
{
    uint64_t header = *(uint64_t *)((char *)ptr - HEADER_SIZE);
    return (size_t)(header & ~(uint64_t)(ALIGNMENT - 1));
}

static char *footer_of(void *ptr)
{
    return header_of(ptr) + block_size(ptr) - FOOTER_SIZE;
}

static size_t size_class(size_t block_size) {
    size_t index = 0;
    size_t limit = 32;
    while (index < NUM_SIZE_CLASSES - 1 && block_size > limit) {
        limit <<= 1;
        index++;
    }
    return index;
}

static void **prev_field(void *ptr) 
{
    return (void **)ptr;
}

static void **next_field(void *ptr) 
{
    return (void **)((char *)ptr + sizeof(void *));
}

static void insert_free_block(void *ptr) 
{
    size_t index = size_class(block_size(ptr));
    void *old_head = free_lists[index];
    *prev_field(ptr) = NULL;
    *next_field(ptr) = old_head;
    if (old_head != NULL) {
        *prev_field(old_head) = ptr;
    }
    free_lists[index] = ptr;
}

static void remove_free_block(void *ptr) 
{
    size_t index = size_class(block_size(ptr));
    void *prev = *prev_field(ptr);
    void *next = *next_field(ptr);
    if (prev != NULL) {
        *next_field(prev) = next;
    } else {
        free_lists[index] = next;
    }
    if (next != NULL) {
        *prev_field(next) = prev;
    }
}

static void *find_fit(size_t required_size) {
    size_t index = size_class(required_size);
    while (index < NUM_SIZE_CLASSES) {
        void *ptr = free_lists[index];
        while (ptr != NULL) {
            if (block_size(ptr) >= required_size) {
                return ptr;
            }
            ptr = *next_field(ptr);
        }
        index++;
    }
    return NULL;
}

static void *extend_heap(size_t size) {
    if (size < MIN_BLOCK_SIZE || size % ALIGNMENT != 0 || size > INT_MAX)
        return NULL;
    char *old_end = (char *)mem_heap_lo() + mem_heapsize();
    char *new_header = mem_sbrk((int)size);
    if (new_header == (void *)-1)
        return NULL;
    
    char *merged_header = new_header;
    size_t merged_size  = size;
    if (old_end > blocks_start) {
        char *prev_footer = old_end - FOOTER_SIZE;
        uint64_t prev_value = *(uint64_t *)prev_footer;

        if ((prev_value & 1) == 0) {
            size_t prev_size = (size_t)(prev_value & ~(uint64_t)(ALIGNMENT - 1));
            char *prev_header = old_end - prev_size;
            void *prev_ptr = prev_header + HEADER_SIZE;
            remove_free_block(prev_ptr);

            merged_header = prev_header;
            merged_size += prev_size;
        }
    }
    uint64_t free_value = (uint64_t)merged_size;
    *(uint64_t *)merged_header = free_value;
    *(uint64_t *)(merged_header + merged_size - FOOTER_SIZE) = free_value;

    void *ptr = merged_header + HEADER_SIZE;
    insert_free_block(ptr);
    return ptr;
}

static void place(void *ptr, size_t required_size) {
    size_t old_size = block_size(ptr);
    remove_free_block(ptr);
    size_t remaining = old_size - required_size;
    if (remaining >= MIN_BLOCK_SIZE) {
        uint64_t value = (uint64_t)required_size | 1;
        *(uint64_t *)header_of(ptr) = value;
        *(uint64_t *)footer_of(ptr) = value;
        void *new_ptr = (char *)ptr + required_size;
        *(uint64_t *)header_of(new_ptr) = (uint64_t)remaining;
        *(uint64_t *)footer_of(new_ptr) = (uint64_t)remaining;
        insert_free_block(new_ptr);
    } else {
        uint64_t value = (uint64_t)old_size | 1;
        *(uint64_t *)header_of(ptr) = value;
        *(uint64_t *)footer_of(ptr) = value;
    }
}


static size_t block_size_from_ptr(void *ptr)
{
    uint64_t header = *(uint64_t *)(ptr);
    return (size_t)(header & ~(uint64_t)(ALIGNMENT - 1));
}

static int allocated(const void *field)
{
    return (*(const uint64_t *)field & 1) != 0;
}

/* Merge a free block with preceding free blocks; return the new payload. */
static void *coalesce_forward(void *cur_ptr)
{
    char *heap_start = blocks_start;
    for (;;) {
        char *cur_header = header_of(cur_ptr);
        /* A footer before the heap is not a valid previous block. */
        if ((size_t)(cur_header - heap_start) < FOOTER_SIZE) break;
        char *prev_footer = cur_header - FOOTER_SIZE;

        if (allocated(prev_footer)) break;
        size_t prev_size = block_size_from_ptr(prev_footer);
        if (prev_size < MIN_BLOCK_SIZE ||
            prev_size % ALIGNMENT != 0 ||
            prev_size > (size_t)(cur_header - heap_start))
            break;
            
        char *prev_header = cur_header - prev_size;
        char *cur_footer = footer_of(cur_ptr);
        size_t new_size = prev_size + block_size(cur_ptr);

        remove_free_block(prev_header + HEADER_SIZE);
        *(uint64_t *)prev_header = (uint64_t)new_size;
        *(uint64_t *)cur_footer = (uint64_t)new_size;
        cur_ptr = prev_header + HEADER_SIZE;
    }
    return cur_ptr;
}

static void *coalesce_afterward(void *cur_ptr)
{
    char *heap_end = (char *)mem_heap_lo() + mem_heapsize();
    for (;;) {
        char *cur_footer = footer_of(cur_ptr);
        char *next_header = cur_footer + FOOTER_SIZE;
        if (next_header >= heap_end) break;

        size_t remaining = (size_t)(heap_end - next_header);
        if (remaining < HEADER_SIZE) break;

        if (allocated(next_header)) break;
        size_t next_size = block_size_from_ptr(next_header);
        if (next_size < MIN_BLOCK_SIZE ||
            next_size % ALIGNMENT != 0 ||
            next_size > remaining)
            break;

        char *next_footer = next_header + next_size - FOOTER_SIZE;
        char *cur_header = header_of(cur_ptr);
        size_t new_size = next_size + block_size(cur_ptr);

        remove_free_block(next_header + HEADER_SIZE);
        *(uint64_t *)next_footer = (uint64_t)new_size;
        *(uint64_t *)cur_header = (uint64_t)new_size;
    }
    return cur_ptr;
}

/* 初始化分类链表与堆；未实现时返回失败，避免误认为初始化成功。 */
int mm_init(void)
{
    size_t heads_size = NUM_SIZE_CLASSES * sizeof(*free_lists);

    free_lists = NULL;
    blocks_start = NULL;

    void *area = mem_sbrk((int)heads_size);
    if (area == (void *)-1)
        return -1;
    free_lists = area;
    for (int i = 0; i < NUM_SIZE_CLASSES; ++i)
        free_lists[i] = NULL;
    blocks_start = (char *)area + heads_size;

    return 0;
}

/* 实验策略：接近 2 的幂的请求向上取整，便于日后复用。 */
static size_t rounded_payload_size(size_t size)
{
    if (size < 64)
        return size;

    size_t upper = 64;
    while (upper < size)
        upper <<= 1;

    if (size >= upper - upper / 8 &&
        upper <= (size_t)INT_MAX - HEADER_SIZE - FOOTER_SIZE)
        return upper;
    return size;
}

/* 分配至少 size 字节；返回的 payload 地址需要 8 字节对齐。 */
void *mm_malloc(size_t size)
{
    if (size == 0 ||
        size > SIZE_MAX - HEADER_SIZE - FOOTER_SIZE - (ALIGNMENT - 1))
        return NULL;

    if (size > INT_MAX)
        return NULL;
    size = rounded_payload_size(size);
    size_t total_size = (size + HEADER_SIZE + FOOTER_SIZE + (ALIGNMENT - 1))
                          & ~(size_t)(ALIGNMENT - 1);
    if (total_size < MIN_BLOCK_SIZE) total_size = MIN_BLOCK_SIZE;
    if (total_size > INT_MAX) return NULL;

    void *block = find_fit(total_size);
    if (block == NULL) {
        block = extend_heap(total_size);
        if (block == NULL) {
            return NULL;
        }
    }
    place(block, total_size);

    return block;
}

/* 释放块，合并相邻空闲块，并将结果插入对应类别。 */
void mm_free(void *ptr)
{
    if (ptr == NULL)
        return;
    char* header = header_of(ptr);
    char* footer = footer_of(ptr);
    uint64_t free_value = (uint64_t)block_size(ptr);
    *(uint64_t *)header = free_value;
    *(uint64_t *)footer = free_value;
    ptr = coalesce_forward(ptr);
    ptr = coalesce_afterward(ptr);
    insert_free_block(ptr);
}

/* 调整块大小，并保留旧数据。 */
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

    char *next_header = header_of(ptr) + old_size;
    char *heap_end = (char *)mem_heap_lo() + mem_heapsize();
    void *next_ptr = NULL;
    size_t available_size = old_size;
    if (next_header < heap_end && !allocated(next_header)) {
        next_ptr = next_header + HEADER_SIZE;
        available_size += block_size(next_ptr);
    }

    /* 块或其后相邻的空闲块位于堆尾时，只申请缺少的空间。 */
    if (available_size < required_size &&
        header_of(ptr) + available_size == heap_end) {
        size_t extra = required_size - available_size;
        if (mem_sbrk((int)extra) != (void *)-1)
            available_size = required_size;
    }

    if (available_size >= required_size) {
        size_t remaining = available_size - required_size;
        size_t allocated_size = available_size;

        /* 扩展成功后，修改大小前，再移除原类别中的空闲块。 */
        if (next_ptr != NULL)
            remove_free_block(next_ptr);

        if (remaining >= MIN_BLOCK_SIZE)
            allocated_size = required_size;

        uint64_t allocated_value = (uint64_t)allocated_size | 1;
        *(uint64_t *)header_of(ptr) = allocated_value;
        *(uint64_t *)footer_of(ptr) = allocated_value;

        if (remaining >= MIN_BLOCK_SIZE) {
            void *remainder_ptr = (char *)ptr + allocated_size;
            *(uint64_t *)header_of(remainder_ptr) = (uint64_t)remaining;
            *(uint64_t *)footer_of(remainder_ptr) = (uint64_t)remaining;
            insert_free_block(remainder_ptr);
        }

        return ptr;
    }

    void *block = mm_malloc(size);
    if (block == NULL) return NULL;
    size_t copy_size = old_capacity;
    memcpy(block, ptr, copy_size);
    mm_free(ptr);

    return block;
}

/* 检查堆和分类链表的一致性；未实现时返回不通过。 */
int mm_check(void)
{
    return 0;
}
