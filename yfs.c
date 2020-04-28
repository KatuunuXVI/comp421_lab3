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

/**
 * Searches for and returns the inode based on the number given
 * @param inode_num The inode being requested
 * @return A pointer to where the inode is
 */
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
    printf("Free Inode List\n");
    printBuffer(free_inode_list);
}

/**
 * Searches for a value in an array, if it's found swaps it with the value
 * at index. Helper function for GetFreeBlockList
 * @param arr Array that is to be swapped
 * @param size Size of the array
 * @param value Value that will be swapped
 * @param index Index to swap to
 */
void SearchAndSwap(int arr[], int size, int value, int index) {
    int i = -1;
    int search_index = -1;
    while(i != value && index < size) {
        search_index++;
        i = arr[search_index];
    }
    if(search_index == index || i == -1) return;
    int j = arr[index];
    arr[index] = value;
    arr[search_index] = j;

}

/**
 * Function initializes the list numbers for blocks that have not been allocated yet
 */
void GetFreeBlockList() {
    /**Initialize a special buffer*/
    free_block_list = malloc(sizeof(struct buffer));
    free_block_list->size = data_blocks;
    free_block_list->in = free_block_list->size;
    free_block_list->out = 0;
    free_block_list->empty = 0;
    free_block_list->full = 1;

    /**Number of data blocks dependent on number of inodes*/
    int data_blocks = ((header->num_inodes + 1) % 8) ? header->num_blocks - ((header->num_inodes + 1) / 8) - 1 : header->num_blocks - ((header->num_inodes + 1) / 8) - 2;
    int first_data_block = header->num_blocks-data_blocks-1;
    int b[data_blocks];
    int i;

    /** Block 0 is the boot block and not used by the file system*/
    for (i = first_data_block; i < data_blocks + first_data_block; i ++) {
        b[i-first_data_block] = i;
        printf("%d\n",i);
    }
    /** Iterate through inodes to check which ones are using blocks */
    int busy_blocks = 0;
    for(i = 1; i <= header->num_inodes; i ++) {
        struct inode *scan = GetInode(i);
        int j = 0;
        /** Iterate through direct array to find first blocks*/
        while(scan->direct[j] && j < NUM_DIRECT) {
            SearchAndSwap(b, data_blocks, scan->direct[j], busy_blocks);
            busy_blocks ++;
            j ++;
        }
        /** If indirect array is a non-zero value add it to the allocated block
         * and the array that's contained */
        if(scan->indirect) {
            SearchAndSwap(b, header->num_blocks, scan->indirect, busy_blocks);
            busy_blocks++;
            int *indirect_blocks = (int *) GetBlock(scan->indirect);
            j = 0;
            while(indirect_blocks[j]) {
                SearchAndSwap(b, header->num_blocks, indirect_blocks[j], busy_blocks);
                busy_blocks++;
                j++;
            }
        }
    }
    /** Give the created array to the buffer*/
    free_block_list->b = b;
    int k;
    for(k = 0; k < busy_blocks; k++) {
        popFromBuffer(free_block_list);
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
    printf("Free Node List\n");
    //PrintInodeCache(inode_stack);
    GetFreeBlockList();
    return 0;
};





