#ifndef COMP421_LAB3_CACHE_H
#define COMP421_LAB3_CACHE_H

struct block_cache {
    struct block_cache_entry* top; //Top of the cache stack
    struct block_cache_entry* base; //Bottom of the cache stack
    struct block_cache_entry** hash_set;
    int size; //Number of entries in the cache stack

};

struct block_cache_entry {
    void* block;
    int block_number; //Item that this entry represents. Can either be a block or inode
    struct block_cache_entry* prev_lru; //Previous Block in Stack
    struct block_cache_entry* next_lru; //Next Block in Stack
    struct block_cache_entry* next_hash;
    int dirty; //Whether or not this
};

struct inode_cache {
    struct inode_cache_entry* top; //Top of the cache stack
    struct inode_cache_entry* base; //Bottom of the cache stack
    struct inode_cache_entry** hash_set;
    int stack_size; //Number of entries in the cache stack
    int hash_size; //Number of hash collisions

};

struct inode_cache_entry {
    struct inode* in; //Item that this entry represents. Can either be a block or inode
    int inode_number; //Inode/Block number of the cache entry.
    struct inode_cache_entry* prev_lru; //Previous Inode in Stack
    struct inode_cache_entry* next_lru; //Next Inode in Stack
    struct inode_cache_entry* prev_hash;
    struct inode_cache_entry* next_hash;
    int dirty; //Whether or not this
};

struct inode_cache *CreateInodeCache();

void AddToInodeCache(struct inode_cache *stack, struct inode *in, int inumber);

struct inode* LookUpInode(struct inode_cache *stack, int inumber);

void RaiseInodeCachePosition(struct inode_cache* stack, struct inode_cache_entry* recent_access);

void WriteBackInode(struct inode_cache_entry* out);

void PrintInodeCacheHashSet(struct inode_cache* stack);

void PrintInodeCache(struct inode_cache* stack);

struct block_cache *CreateBlockCache();

void AddToBlockCache(struct block_cache *stack, void* block, int block_number);

void RaiseBlockCachePosition(struct block_cache *stack, struct block_cache_entry* recent_access);

void WriteBackBlock(struct block_cache_entry* out);

void PrintBlockCache(struct block_cache* stack);

int HashIndex(int key_value);
#endif //COMP421_LAB3_CACHE_H
