
#ifndef ZERO_COPY_ALLOCATOR_H
#define ZERO_COPY_ALLOCATOR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Zero-copy memory pool avec memory mapping
typedef struct {
    void* memory_region;
    size_t total_size;
    size_t used_size;
    size_t alignment;
    bool is_mmap_backed;
    int mmap_fd;
    char* region_name;
    
    // Free list pour réutilisation zero-copy
    struct free_block* free_list;
    size_t free_blocks_count;
    
    // Statistics
    size_t allocations_served;
    size_t zero_copy_hits;
    size_t memory_reused_bytes;
} zero_copy_pool_t;

// Block metadata pour zero-copy tracking
typedef struct free_block {
    void* ptr;
    size_t size;
    struct free_block* next;
    uint64_t last_used_timestamp;
} free_block_t;

// Zero-copy allocation result
typedef struct {
    void* ptr;
    size_t size;
    bool is_zero_copy;
    bool is_reused_memory;
    uint64_t allocation_id;
} zero_copy_allocation_t;

// Function declarations
zero_copy_pool_t* zero_copy_pool_create(size_t size, const char* name);
void zero_copy_pool_destroy(zero_copy_pool_t* pool);

// Zero-copy operations
zero_copy_allocation_t* zero_copy_alloc(zero_copy_pool_t* pool, size_t size);
bool zero_copy_free(zero_copy_pool_t* pool, zero_copy_allocation_t* allocation);
bool zero_copy_resize_inplace(zero_copy_pool_t* pool, zero_copy_allocation_t* allocation, size_t new_size);

// Memory mapping optimizations
bool zero_copy_enable_mmap_backing(zero_copy_pool_t* pool);
bool zero_copy_prefault_pages(zero_copy_pool_t* pool);
bool zero_copy_advise_sequential(zero_copy_pool_t* pool);

// Statistics and monitoring
void zero_copy_print_stats(zero_copy_pool_t* pool);
double zero_copy_get_efficiency_ratio(zero_copy_pool_t* pool);
size_t zero_copy_get_fragmentation_bytes(zero_copy_pool_t* pool);

// Advanced features
bool zero_copy_defragment_pool(zero_copy_pool_t* pool);
bool zero_copy_compact_free_list(zero_copy_pool_t* pool);
void zero_copy_allocation_destroy(zero_copy_allocation_t* allocation);

#endif // ZERO_COPY_ALLOCATOR_H
