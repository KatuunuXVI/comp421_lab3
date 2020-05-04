#ifndef COMP421_LAB3_CACHE_H
#define COMP421_LAB3_CACHE_H


struct block_cache {
    struct block_cache_entry* top; //Top of the cache stack
    struct block_cache_entry* base; //Bottom of the cache stack
    struct block_cache_entry** hash_set;
    int stack_size; //Number of entries in the cache stack
    int hash_size;
};

struct block_cache_entry {
    void* block;
    int block_number; //Item that this entry represents. Can either be a block or inode
    struct block_cache_entry* prev_lru; //Previous Block in Stack
    struct block_cache_entry* next_lru; //Next Block in Stack
    struct block_cache_entry* prev_hash;
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
    struct inode* inode; //Item that this entry represents. Can either be a block or inode
    int inum; //Inode/Block number of the cache entry.
    struct inode_cache_entry* prev_lru; //Previous Inode in Stack
    struct inode_cache_entry* next_lru; //Next Inode in Stack
    struct inode_cache_entry* prev_hash;
    struct inode_cache_entry* next_hash;
    int dirty; //Whether or not this
};

/*********************
 * Inode Cache Code *
 ********************/

struct inode_cache *CreateInodeCache();

void AddToInodeCache(struct inode_cache *stack, struct inode *in, int inumber);

struct inode_cache_entry* LookUpInode(struct inode_cache *stack, int inumber);

void RaiseInodeCachePosition(struct inode_cache* stack, struct inode_cache_entry* recent_access);

void WriteBackInode(struct inode_cache_entry* out);

struct inode_cache_entry* GetInode(int inode_num);

void PrintInodeCacheHashSet(struct inode_cache* stack);

void PrintInodeCacheStack(struct inode_cache* stack);

/*********************
 * Block Cache Code *
 ********************/

struct block_cache *CreateBlockCache(int num_blocks);

void AddToBlockCache(struct block_cache *stack, void* block, int block_number);

struct block_cache_entry* LookUpBlock(struct block_cache *stack, int block_number);

void RaiseBlockCachePosition(struct block_cache *stack, struct block_cache_entry* recent_access);

void WriteBackBlock(struct block_cache_entry* out);

struct block_cache_entry* GetBlock(int block_num);

void PrintBlockCacheHashSet(struct block_cache* stack);

void PrintBlockCacheStack(struct block_cache* stack);

int HashIndex(int key_value);

void TestInodeCache(int num_inodes);

void TestBlockCache(int num_blocks);

#endif //COMP421_LAB3_CACHE_H
