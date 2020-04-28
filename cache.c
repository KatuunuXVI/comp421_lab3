#include "cache.h"
#include <comp421/filesystem.h>
#include <stdlib.h>
#include <stdio.h>
#include <comp421/yalnix.h>
/*********************
 * Inode Cache Code *
 ********************/
/**
 * Creates a new Cache for Inodes
 * @return Newly created cache
 */
struct inode_cache *CreateInodeCache() {
    struct inode_cache *new_cache = malloc(sizeof(struct inode_cache));
    new_cache->size = 0;
    return new_cache;
}

/**
 * Adds a new inode to the top of the cache entry
 */
void AddToInodeCache(struct inode_cache *stack, struct inode *in, int inumber) {
    /** Create an Cache Entry for the Inode*/
    struct inode_cache_entry* item = malloc(sizeof(struct inode_cache_entry));
    item->inode_number = inumber;
    item->in = in;
    if (!stack->size) {
        /** If the stack is empty the entry becomes both the top and base*/
        stack->base = item;
        stack->top = item;
    } else {
        /** If the stack isn't empty the entry is added to the top*/
        item->next = stack->top;
        stack->top->prev = item;
        stack->top = item;
    }
    if (stack->size == INODE_CACHESIZE) {
        /**If the stack is full, the base is de-allocated and the pointers to it are nullified */
        if(stack->base->dirty) WriteBackInode(stack->base);
        stack->base = stack->base->prev;
        free(stack->base->next);
        stack->base->next = NULL;
    } else {
        /**If the stack isn't full increase the size*/
        stack->size++;
    }
}

/**
 *
 * @param out Inode that was pushed out of the cache
 */
void WriteBackInode(struct inode_cache_entry* out) {
    void *block = malloc(SECTORSIZE);
    ReadSector((out->inode_number / 8) + 1, block);
    struct inode *overwrite = (struct inode *) block + (out->inode_number % 8);
    *overwrite = *out->in;
    WriteSector((out->inode_number / 8) + 1, block);
    free(block);
}

/**
 * When ever an inode is accessed, it is pulled from it's position in the stack
 * and placed at the top
 * @param stack Stack to rearrange
 * @param recent_access Entry to move
 */
void RaiseInodeCachePosition(struct inode_cache* stack, struct inode_cache_entry* recent_access) {
    recent_access->next->prev = recent_access->prev;
    recent_access->prev->next = recent_access->next;
    recent_access->next = stack->top;
    stack->top = recent_access;
}

/**
 * Prints out the numbers of the inodes currently in Cache
 * @param stack Stack to print out
 */
void PrintInodeCache(struct inode_cache* stack) {
    struct inode_cache_entry* position = stack->top;
    while (position != NULL) {
        printf("| %d |\n", position->inode_number);
        position = position->next;
    }
}

/*********************
 * Block Cache Code *
 ********************/
/**
 * Creates new LIFO Cache for blocks
 */
struct block_cache *CreateBlockCache() {
    struct block_cache *new_cache = malloc(sizeof(struct block_cache));
    new_cache->size = 0;
    return new_cache;
}

/**
 * Adds a new inode to the top of the cache entry
 */
void AddToBlockCache(struct block_cache *stack, void* block, int block_number) {
    struct block_cache_entry* item = malloc(sizeof(struct block_cache_entry));
    item->block_number = block_number;
    item->block = block;

    /** If the stack is empty the entry becomes both the top and base*/
    if (!stack->size) {
        stack->base = item;
        stack->top = item;
    } else {
        /** If the stack isn't empty the entry is added to the top*/
        item->next = stack->top;
        stack->top->prev = item;
        stack->top = item;
    }

    /**If the cache is at max size, the last used block is removed. */
    if (stack->size == BLOCK_CACHESIZE) {
        if(stack->base->dirty) WriteSector(stack->base->block_number,stack->base->block);
        stack->base = stack->base->prev;
        free(stack->base->next);
        stack->base->next = NULL;
    } else { /**If the stack isn't full increase the size*/
        stack->size++;
    }
}

/**
 * Repositions block at top of stack whenever used
 */
void RaiseBlockCachePosition(struct block_cache *stack, struct block_cache_entry* recent_access) {
    recent_access->next->prev = recent_access->prev;
    recent_access->prev->next = recent_access->next;
    recent_access->next = stack->top;
    stack->top = recent_access;
}

void WriteBackBlock(struct block_cache_entry* out) {
    void *block = malloc(SECTORSIZE);
    ReadSector(out->block_number, block);

}

/**
 * Prints out the Block Cache as a stack
 */
 void PrintBlockCache(struct block_cache* stack) {
    struct block_cache_entry* position = stack->top;
    while (position != NULL) {
        printf("| %d |\n", position->block_number);
        position = position->next;
    }
 }