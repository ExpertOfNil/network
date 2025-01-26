#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stdbool.h>

#include "proto.h"

typedef enum {
    UNUSED,
    USED,
} block_status_e;

typedef struct {
    u8* memory;
    block_status_e* status;
    u32 n_blocks;
    u32 block_size;
} Arena;

Arena Arena_init(u32 n_blocks, u32 block_size);
void* Arena_alloc(Arena* arena);
void Arena_dealloc(Arena* arena, void* block);
void Arena_clear(Arena* arena);
void Arena_free(Arena* arena);
void Arena_print(Arena* arena);

/* Initializer for the arena allocator */
Arena Arena_init(u32 n_blocks, u32 block_size) {
    u8* memory = malloc(n_blocks * block_size);
    memset(memory, 0, n_blocks * block_size);
    block_status_e* status = malloc(n_blocks * sizeof(block_status_e));
    memset(status, UNUSED, n_blocks * sizeof(block_status_e));
    Arena arena = {0};
    arena.memory = memory;
    arena.status = status;
    arena.n_blocks = n_blocks;
    arena.block_size = block_size;
    return arena;
}

/* Find a free memory block and return a pointer to its location */
void* Arena_alloc(Arena* arena) {
    for (size_t i = 0; i < arena->n_blocks; ++i) {
        if (arena->status[i] == UNUSED) {
            arena->status[i] = USED;
            return &arena->memory[i * arena->block_size];
        }
    }
    return NULL;
}

/* Clear a memory block and make it available for reuse */
void Arena_dealloc(Arena* arena, void* block) {
    for (size_t i = 0; i < arena->n_blocks; ++i) {
        size_t idx = i * sizeof(arena->block_size);
        if (block == &arena->memory[idx]) {
            memset(&arena->memory[idx], 0, arena->block_size);
            arena->status[i] = UNUSED;
            return;
        }
    }
    return;
}

/* Clear all data, but keep blocks intact */
void Arena_clear(Arena* arena) {
    memset(arena->memory, 0, arena->n_blocks * arena->block_size);
    memset(arena->status, UNUSED, arena->n_blocks * sizeof(block_status_e));
}

/* Free all heap allocations */
void Arena_free(Arena* arena) {
    free(arena->status);
    free(arena->memory);
}

/* Print arena statistics */
void Arena_print(Arena* arena) {
    u32 unused = 0;
    for (size_t i = 0; i < arena->n_blocks; ++i) {
        if (arena->status[i] == UNUSED) {
            unused++;
        }
    }
    printf("Block size: %u bytes\n", arena->block_size);
    printf("Total blocks: %u\n", arena->n_blocks);
    printf("Unused blocks: %u\n", unused);
}

#endif /* ALLOCATOR_H */
