#include "cache.h"

/*********************
 * Inode Cache Code *
 ********************/

struct inode_cache *create_inode_cache() {
    struct cache *new_cache = malloc(sizeof(struct inode_cache));
    new_cache->size = 0;
    return new_cache;
}

/**
 * Adds a new inode to the top of the cache entry
 */
void add_to_inode_cache(struct inode_cache *stack, struct inode *in, int inumber) {
    struct inode_cache_entry* item = malloc(sizeof(struct inode_cache_entry));
    item->inode_number = inumber;
    item->in = in;
    if(stack->base == NULL) stack->base = item;
    item->next = stack->top;
    if(item->next != NULL) item->next->prev = item;
    stack->top = in;
    if(stack->size == INODE_CACHESIZE) {
        stack->base = stack->base->prev;
        free(stack->base->next);
    } else {
        stack->size++;
    }
}


/*
struct inode_cache_entry *search_inode_stack(struct inode_cache *inode_stack, int inode_number) {
    int i = 0;
    struct inode_cache_entry *current = inode_stack->top;
    int found = (current->inode_number == inode_number);
    while(i < INODE_CACHESIZE && !found) {//Searches until it runs to the bottom of the stack
        current = current->next;
        i++;
        found = current->inode_number == inode_number;
    }
    if(found) {
        return current
    }else {
        null;
    }
}
*/
//^ Will be coded into get_inode

/**
 * When ever an inode is accessed, it is pulled from it's position in the stack
 * and placed at the top
 */
void raise_inode_cache_entry(struct inode_cache* stack, struct inode_cache_entry* recent_access) {
    recent_access->next->prev = recent_access->prev;
    recent_access->prev->next = recent_access->next;
    recent_access->next = stack->top;
    stack->top = recent_access;
}

/*********************
 * Block Cache Code *
 ********************/
/**
 * Creates new LIFO Cache for blocks
 */
struct block_cache *create_block_cache() {
    struct block_cache *new_cache = malloc(sizeof(struct block_cache));
    new_cache->size = 0;
    return new_cache;
}

/**
 * Adds a new inode to the top of the cache entry
 */
void add_to_block_cache(struct block_cache *stack, void* block, int block_number) {
    struct block_cache_entry* item = malloc(sizeof(struct block_cache_entry));
    item->block_number = block_number;
    item->block = block;
    if(stack->base == NULL) stack->base = item;
    item->next = stack->top;
    if(item->next != NULL) item->next->prev = item;
    stack->top = in;
    /**
     * If the cache is at max size, the last used block is removed.
     */
    if(stack->size == BLOCK_CACHESIZE) {
        stack->base = stack->base->prev;
        free(stack->base->next);
    } else {
        stack->size++;
    }
}

/**
 * Repositions block at top of stack whenever used
 */
void raise_block_cache_position(struct block_cache *stack, struct block_cache_entry* recent_access) {
    recent_access->next->prev = recent_access->prev;
    recent_access->prev->next = recent_access->next;
    recent_access->next = stack->top;
    stack->top = recent_access;
}