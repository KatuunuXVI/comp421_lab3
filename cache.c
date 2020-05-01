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
    printf("Adding Inode %d to Cache\n",inumber);
    /** Create an Cache Entry for the Inode*/
    struct inode_cache_entry* item = malloc(sizeof(struct inode_cache_entry));
    item->inode_number = inumber;
    item->in = in;
    int index = HashIndex(inumber);
    if(stack->hash_set[index] != NULL) stack->hash_set[index]->prev_hash = item;
    item->next_hash = stack->hash_set[index];
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
        printf("Full\n");
        int old_index = HashIndex(stack->base->inode_number);
        if(stack->base->dirty) WriteBackInode(stack->base);
        printf("Popping out %d\n",stack->base->inode_number);
        if(stack->base->prev_hash != NULL && stack->base->next_hash != NULL) {
            printf("Middle\n");
            stack->base->next_hash->prev_hash = stack->base->prev_hash;
            stack->base->prev_hash->next_hash = stack->base->next_hash;
        } else if(stack->base->prev_hash == NULL && stack->base->next_hash != NULL) {
            printf("Head\n");
            stack->hash_set[old_index] = stack->base->next_hash;
        } else if(stack->base->prev_hash != NULL && stack->base->next_hash == NULL) {
            printf("Base: %d\n",stack->base->inode_number);
            printf("Base: %d\n",stack->base->prev_hash == NULL);
            printf("Prev: %d\n",stack->base->prev_hash->next_hash->inode_number);
            stack->base->prev_hash->next_hash = NULL;
        }
        else {
            printf("Alone\n");
            stack->hash_set[old_index] = NULL;
        }
        printf("Chain Deletion\n");
        stack->base = stack->base->prev_lru;
        free(stack->base->next_lru);
        stack->base->next_lru = NULL;
    } else {
        /**If the stack isn't full increase the size*/
        stack->stack_size++;
    }
    printf("Done\n");
}


struct inode* LookUpInode(struct inode_cache *stack, int inumber) {
    struct inode_cache_entry* ice;
    for(ice = stack->hash_set[HashIndex(inumber)]; ice != NULL; ice = ice->next_hash) {
        if(ice->inode_number == inumber) {
            printf("ICE Number: %d\n",ice->inode_number);
            RaiseInodeCachePosition(stack,ice);
            return ice->in;
        }
    }
    return NULL;
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
    if(recent_access->inode_number == stack->top->inode_number) return;
    if(recent_access->inode_number == stack->base->inode_number) {
        recent_access->prev_lru->next_lru = NULL;
        recent_access->next_lru = stack->top;
        stack->top = recent_access;
    } else {
        recent_access->next_lru->prev_lru = recent_access->prev_lru;
        recent_access->prev_lru->next_lru = recent_access->next_lru;
        recent_access->next_lru = stack->top;
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
        item->next_lru = stack->top;
        stack->top->prev_lru = item;
        stack->top = item;
    }
    /**If the cache is at max size, the last used block is removed. */
    if (stack->size == BLOCK_CACHESIZE) {
        printf("Full\n");
        if(stack->base->dirty) WriteSector(stack->base->block_number,stack->base->block);
        stack->base = stack->base->prev_lru;
        free(stack->base->next_lru);
        stack->base->next_lru = NULL;
    } else { /**If the stack isn't full increase the size*/
        stack->size++;
    }
}

/**
 * Repositions block at top of stack whenever used
 */
void RaiseBlockCachePosition(struct block_cache *stack, struct block_cache_entry* recent_access) {
    if(recent_access->block_number == stack->top->block_number) return;
    if(recent_access->block_number == stack->base->block_number) {
        recent_access->prev_lru->next_lru = NULL;
        recent_access->next_lru = stack->top;
        stack->top = recent_access;
    } else {
        recent_access->next_lru->prev_lru = recent_access->prev_lru;
        recent_access->prev_lru->next_lru = recent_access->next_lru;
        recent_access->next_lru = stack->top;
        stack->top = recent_access;
    }

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
         printf("While Loop\n");
         printf("| %d |\n", position->block_number);
         position = position->next_lru;
     }
 }

int HashIndex(int key_value) {
     return key_value/8;
 }