#include <comp421/yalnix.h>
#include <comp421/filesystem.h>
#include "yfs.h"
#include "cache.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

struct fs_header *header; /**Pointer to File System Header*/

struct inode *root_inode; /**Pointer to Root Inode*/

struct block_cache* block_stack;

struct inode_cache* inode_stack;

int main(int argc, char **argv) {
    printf("Argument Count: %d\n",argc);
    int i;
    for(i = 0; i < argc; i++) {
        printf("Argument %d: %s\n", i, argv[i]);
    }
    Register(FILE_SERVER);
    /** Obtain File System Header */
    void *sector_one = malloc(SECTORSIZE);
    if(ReadSector(1,sector_one) == 0) {
        header = (struct fs_header *)sector_one;
    } else {
        printf("Error\n");
    }
    /**Obtain Root Directory Inode*/
    root_inode = (struct inode *)header + 1;
    //TODO Make List of free Inodes
    return 0;
};

void *get_block(int block_num) {
    assert(block_num >= 1 && block_num <= header->num_inodes);
    /** First Check Block Cache*/
    int i = 0;
    struct block_cache_entry *current = block_stack->top;
    int found = (current->block_number == block_num);
    while(i < BLOCK_CACHESIZE && !found) {
        current = current->next;
        i++;
        found = (current->block_number == block_num);
    }
    if(found) return current;
    /** If not found in cache, read directly from disk */
    void *block_buffer = malloc(SECTORSIZE);
    ReadSector(block_num,block_buffer);
    return block_buffer;
}

struct inode* get_inode(int inode_num) {
    assert(inode_num >= 1 && inode_num <= header->num_inodes);
    /** First Check the Inode Cache */
    int i = 0;
    struct inode_cache_entry *current = inode_stack->top;
    int found = (current->inode_number == inode_num);
    while(i < INODE_CACHESIZE && !found) {//Searches until it runs to the bottom of the stack
        current = current->next;
        i++;
        found = current->inode_number == inode_num;
    }
    if(found) return current->in;
    /**If it's not in the Inode Cache, check the Block */
    void* inode_block = get_block((inode_num / 8) + 1);

    return (struct inode *)inode_block + (inode_num % 8);

}