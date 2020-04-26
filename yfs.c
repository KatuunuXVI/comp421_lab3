#include <comp421/yalnix.h>
#include <comp421/filesystem.h>
#include "yfs.h"
#include "cache.h"
#include "buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

struct fs_header *header; /**Pointer to File System Header*/

struct inode *root_inode; /**Pointer to Root Directory Inode*/

struct block_cache* block_stack; /**Cache for recently accessed blocks*/

struct inode_cache* inode_stack; /**Cache for recently accessed inodes*/

struct buffer* free_inode_list; /**List of Inodes available to assign to files*/

struct buffer* free_block_list; /**List of blocks ready to allocate for file data*/

/**
 * Returns a block, either by searching the cache or reading its sector
 * @param block_num The number of the block being requested
 * @return Pointer to the data that the block encapsulates
 */
void *GetBlock(int block_num) {
    /**Must be a valid block number */
    assert(block_num >= 1 && block_num <= header->num_inodes);

    /** First Check Block Cache*/
    int found = 0;
    struct block_cache_entry *current = block_stack->top;
    while (current != NULL && !found) {
        found = (current->block_number == block_num);
        current = current->next;
    }
    if (found) return current;

    /** If not found in cache, read directly from disk */
    void *block_buffer = malloc(SECTORSIZE);
    ReadSector(block_num, block_buffer);
    return block_buffer;
}

struct inode* GetInode(int inode_num) {
    /** Inode number must be in valid range*/
    assert(inode_num >= 1 && inode_num <= header->num_inodes);

    /** First Check the Inode Cache */
    struct inode_cache_entry *current = inode_stack->top;
    int found = 0;
    while (current != NULL && !found) {//Searches until it runs to the bottom of the stack
        found = (current->inode_number == inode_num);
        current = current->next;
    }
    if (found) {
        RaiseInodeCachePosition(inode_stack, current);
        return current->in;
    }
    /**If it's not in the Inode Cache, check the Block */
    void* inode_block = GetBlock((inode_num / 8) + 1);
    AddToInodeCache(inode_stack, (struct inode *)inode_block + (inode_num % 8), inode_num); /**Add inode to cache, when accessed*/
    return (struct inode *)inode_block + (inode_num % 8);

}

/**
 * Iterates through Inodes and pushes each free one to the buffer
 */
void GetFreeInodeList() {
    free_inode_list = getBuffer(header->num_inodes);
    int i;
    for (i = 1; i <= header->num_inodes; i ++) {
        struct inode *next = GetInode(i);
        if (next->type == INODE_FREE) pushToBuffer(free_inode_list, i);
    }
}



int main(int argc, char **argv) {
    Register(FILE_SERVER);
    /** Obtain File System Header */
    void *sector_one = malloc(SECTORSIZE);
    if (ReadSector(1, sector_one) == 0) {
        header = (struct fs_header *)sector_one;
    } else {
        printf("Error\n");
    }
    /**Obtain Root Directory Inode*/
    root_inode = (struct inode *)header + 1;
    inode_stack = CreateInodeCache();
    block_stack = CreateBlockCache();
    GetFreeInodeList();
    printBuffer(free_inode_list);
    PrintInodeCache(inode_stack);
    return 0;
};





