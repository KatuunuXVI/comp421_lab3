#include "cache.h"
#include <comp421/filesystem.h>
#include <stdlib.h>
#include <stdio.h>
#include <comp421/yalnix.h>
#include <string.h>
#include "yfs.h"

/**Function Declaration*/
void *GetBlock(int block_num);

/*********************
 * Inode Cache Code *
 ********************/
/**
 * Creates a new Cache for Inodes
 * @return Newly created cache
 */
struct inode_cache *CreateInodeCache(int num_inodes) {
    struct inode_cache *new_cache = malloc(sizeof(struct inode_cache));
    new_cache->stack_size = 0;
    new_cache->hash_set = calloc((num_inodes/8) + 1, sizeof(struct inode_cache_entry));
    printf("Hash Size: %d\n",(num_inodes/8) + 1);
    new_cache->hash_size = (num_inodes/8) + 1;
    return new_cache;
}

/**
 * Adds a new inode to the top of the cache entry
 */
void AddToInodeCache(struct inode_cache *stack, struct inode *in, int inumber) {
    /** Create a Cache Entry for the Inode*/
    struct inode_cache_entry* item = malloc(sizeof(struct inode_cache_entry));
    item->inode_number = inumber;
    item->in = in;
    int index = HashIndex(inumber);
    if(stack->hash_set[index] != NULL) stack->hash_set[index]->prev_hash = item;
    item->next_hash = stack->hash_set[index];
    item->prev_hash = NULL;
    item->prev_lru = NULL;
    stack->hash_set[index] = item;
    /** Place entry into the LRU Stack*/
    if (!stack->stack_size) {
        /** If the stack is empty the entry becomes both the top and base*/
        stack->base = item;
        stack->top = item;
    } else {
        /** If the stack isn't empty the entry is added to the top*/
        item->next_lru = stack->top;
        stack->top->prev_lru = item;
        stack->top = item;

    }
    if (stack->stack_size == INODE_CACHESIZE) {
        /**If the stack is full, the base is de-allocated and the pointers to it are nullified */
        int old_index = HashIndex(stack->base->inode_number);
        /** Write Back the Inode if it is dirty to avoid losing data*/
        if(stack->base->dirty) WriteBackInode(stack->base);
        if(stack->base->prev_hash != NULL && stack->base->next_hash != NULL) {
            /**Both Neighbors aren't Null*/
            stack->base->next_hash->prev_hash = stack->base->prev_hash;
            stack->base->prev_hash->next_hash = stack->base->next_hash;
        } else if(stack->base->prev_hash == NULL && stack->base->next_hash != NULL) {
            /**Tail End of Hash Table Array*/
            stack->hash_set[old_index] = stack->base->next_hash;
            stack->base->next_hash->prev_hash = NULL;
        } else if(stack->base->prev_hash != NULL && stack->base->next_hash == NULL) {
            /**Head of Hash Table Array*/
            stack->base->prev_hash->next_hash = NULL;
        }
        else {
            /**No Neighbors in Hash Table Array*/
            stack->hash_set[old_index] = NULL;
        }
        /**Reassign Base*/
        stack->base = stack->base->prev_lru;
        free(stack->base->next_lru);
        stack->base->next_lru = NULL;
    } else {
        /**If the stack isn't full increase the size*/
        stack->stack_size++;
    }
}

/**
 * Searches up an inode within a cache
 * @param stack Cache to look for the inode in
 * @param inumber Number of the inode being looked up
 * @return The requested inode
 */
struct inode* LookUpInode(struct inode_cache *stack, int inumber) {
    struct inode_cache_entry* ice;
    /**For loop iterating into the hash array index the inode number points to*/
    for(ice = stack->hash_set[HashIndex(inumber)]; ice != NULL; ice = ice->next_hash) {
        if(ice->inode_number == inumber) {
            RaiseInodeCachePosition(stack,ice);
            return ice->in;
        }
    }
    /**Returns null if Inode is not found*/
    return NULL;
}

/**
 *
 * @param out Inode that was pushed out of the cache
 */
void WriteBackInode(struct inode_cache_entry* out) {
    void* inode_block = GetBlock((out->inode_number / 8) + 1);
    struct inode* overwrite = (struct inode *)inode_block + (out->inode_number % 8);
    memcpy(overwrite,out->in, sizeof(struct inode));
    out->dirty = 0;
}

/**
 * When ever an inode is accessed, it is pulled from it's position in the stack
 * and placed at the top
 * @param stack Stack to rearrange
 * @param recent_access Entry to move
 */
void RaiseInodeCachePosition(struct inode_cache* stack, struct inode_cache_entry* recent_access) {
    if((recent_access->inode_number == stack->top->inode_number) || (recent_access->prev_lru == NULL && recent_access->next_lru == NULL)) return;
    if(recent_access->inode_number == stack->base->inode_number) {
        /**Reassign Base*/
        recent_access->prev_lru->next_lru = NULL;
        stack->base = recent_access->prev_lru;

        /**Reassign Top*/
        recent_access->next_lru = stack->top;
        stack->top->prev_lru = recent_access;
        stack->top = recent_access;
    } else {
        /**Reassign Neighbors*/
        recent_access->next_lru->prev_lru = recent_access->prev_lru;
        recent_access->prev_lru->next_lru = recent_access->next_lru;

        /**Reassign Top*/
        recent_access->next_lru = stack->top;
        stack->top->prev_lru = recent_access;
        stack->top = recent_access;
    }
}

void PrintInodeCacheHashSet(struct inode_cache* stack) {
    int index;
    struct inode_cache_entry* entry;

    for(index = 0; index < stack->hash_size; index++) {
        printf("[");
        for(entry = stack->hash_set[index]; entry != NULL; entry = entry->next_hash) {
            printf(" %d ",entry->inode_number);
        }
        printf("]\n");
    }
}

/**
 * Prints out the numbers of the inodes currently in Cache
 * @param stack Stack to print out
 */
void PrintInodeCacheStack(struct inode_cache* stack) {
    struct inode_cache_entry* position = stack->top;
    while (position != NULL) {
        printf("| %d |\n", position->inode_number);
        position = position->next_lru;
    }
}

/*********************
 * Block Cache Code *
 ********************/
/**
 * Creates new LIFO Cache for blocks
 */
struct block_cache *CreateBlockCache(int num_blocks) {
    struct block_cache *new_cache = malloc(sizeof(struct block_cache));
    new_cache->stack_size = 0;
    new_cache->hash_set = calloc((num_blocks/8) + 1, sizeof(struct block_cache_entry));
    new_cache->hash_size = (num_blocks/8) + 1;
    return new_cache;
}

/**
 * Adds a new inode to the top of the cache entry
 */
void AddToBlockCache(struct block_cache *stack, void* block, int block_number) {
    /**Create a Cache Entry for the Block*/
    struct block_cache_entry* item = malloc(sizeof(struct block_cache_entry));
    item->block_number = block_number;
    item->block = block;
    int index = HashIndex(block_number);
    /** If the stack is empty the entry becomes both the top and base*/
    if(stack->hash_set[index] != NULL) stack->hash_set[index]->prev_hash = item;
    item->next_hash = stack->hash_set[index];
    item->prev_hash = NULL;
    item->prev_lru = NULL;
    stack->hash_set[index] = item;
    if (!stack->stack_size) {
        stack->base = item;
        stack->top = item;
    } else {
        /** If the stack isn't empty the entry is added to the top*/
        item->next_lru = stack->top;
        stack->top->prev_lru = item;
        stack->top = item;
    }
    /**If the cache is at max size, the last used block is removed. */
    if (stack->stack_size == BLOCK_CACHESIZE) {
        int old_index = HashIndex(stack->base->block_number);
        if(stack->base->dirty) WriteSector(stack->base->block_number,stack->base->block);
        if(stack->base->prev_hash != NULL && stack->base->next_hash != NULL) {
            /**Both Neighbors aren't Null*/
            stack->base->next_hash->prev_hash = stack->base->prev_hash;
            stack->base->prev_hash->next_hash = stack->base->next_hash;
        } else if(stack->base->prev_hash == NULL && stack->base->next_hash != NULL) {
            /**Tail End of Hash Table Array*/
            stack->hash_set[old_index] = stack->base->next_hash;
            stack->base->next_hash->prev_hash = NULL;
        } else if(stack->base->prev_hash != NULL && stack->base->next_hash == NULL) {
            /**Head of Hash Table Array*/
            stack->base->prev_hash->next_hash = NULL;
        }
        else {
            /**No Neighbors in Hash Table Array*/
            stack->hash_set[old_index] = NULL;
        }
        stack->base = stack->base->prev_lru;
        free(stack->base->next_lru);
        stack->base->next_lru = NULL;
    } else { /**If the stack isn't full increase the size*/
        stack->stack_size++;
    }
}

/**
 * Searches for a Block in the Cache
 * @param stack Stack to search for the Block in
 * @param block_number Number of the block to search for
 * @return The requested Block or a pointer to Null
 */
void* LookUpBlock(struct block_cache *stack, int block_number) {
    struct block_cache_entry* block;
    /**For loop iterating into the hash array index the inode number points to*/
    for(block = stack->hash_set[HashIndex(block_number)]; block != NULL; block = block->next_hash) {
        if(block->block_number == block_number) {
            RaiseBlockCachePosition(stack,block);
            return block->block;
        }
    }
    /**Returns null if Inode is not found*/
    return NULL;
}

/**
 * Repositions block at top of stack whenever used
 */
void RaiseBlockCachePosition(struct block_cache *stack, struct block_cache_entry* recent_access) {
    if((recent_access->block_number == stack->top->block_number) || (recent_access->prev_lru == NULL && recent_access->next_lru == NULL)) return;
    if(recent_access->block_number == stack->base->block_number) {
        /**Reassign Base*/
        recent_access->prev_lru->next_lru = NULL;
        stack->base = recent_access->prev_lru;

        /**Reassign Top*/
        recent_access->next_lru = stack->top;
        stack->top->prev_lru = recent_access;
        stack->top = recent_access;
    } else {
        /**Reassign Neighbors*/
        recent_access->next_lru->prev_lru = recent_access->prev_lru;
        recent_access->prev_lru->next_lru = recent_access->next_lru;

        /**Reassign Top*/
        recent_access->next_lru = stack->top;
        stack->top->prev_lru = recent_access;
        stack->top = recent_access;
    }
}


void PrintBlockCacheHashSet(struct block_cache* stack) {
    int index;
    struct block_cache_entry* entry;

    for(index = 0; index < stack->hash_size; index++) {
        if(stack->hash_set[index] == NULL) continue;
        printf("[");
        for(entry = stack->hash_set[index]; entry != NULL; entry = entry->next_hash) {
            printf(" %d ",entry->block_number);
        }
        printf("]\n");
    }
}

/**
 * Prints out the Block Cache as a stack
 */
 void PrintBlockCacheStack(struct block_cache* stack) {
     struct block_cache_entry* position = stack->top;
     while (position != NULL) {
         printf("| %d |\n", position->block_number);
         position = position->next_lru;
     }
 }

int HashIndex(int key_value) {
     return key_value/8;
 }