#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <comp421/yalnix.h>
#include <comp421/filesystem.h>
#include "yfs.h"
#include "cache.h"
#include "buffer.h"
#include "path.h"
#include "packet.h"

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
struct inode *GetInode(int inode_num) {
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

/*
 * TODO:
 * Return -1 if inum has no available room.
 * Create inode. Make sure cache knows it.
 * Register that to inum's inode and update size.
 */
int CreateDirectory(int parent_inum, int new_inum, char *dirname);

/*
 * TODO:
 * Given directory inode and dirname,
 * find inode which matches with dirname.
 */
int SearchDirectory(struct inode *inode, char *dirname);

/*
 * TODO:
 * Set size as 0. Free all of its blocks.
 */
int TruncateInode(struct inode *inode);

int main(int argc, char **argv) {
    Register(FILE_SERVER);

    /* Obtain File System Header */
    void *sector_one = malloc(SECTORSIZE);
    if (ReadSector(1, sector_one) == 0) {
        header = (struct fs_header *)sector_one;
    } else {
        printf("Error\n");
    }

    /* Obtain Root Directory Inode*/
    root_inode = (struct inode *)header + 1;

    inode_stack = CreateInodeCache();
    block_stack = CreateBlockCache();
    GetFreeInodeList();
    // printBuffer(free_inode_list);
    // PrintInodeCache(inode_stack);

    int pid;
  	if ((pid = Fork()) < 0) {
  	    fprintf(stderr, "Cannot Fork.\n");
  	    return -1;
  	}

    /* Child process exec program */
    if (pid == 0) {
        GetBlock(7);
        Exec(argv[1], argv + 1);
        fprintf(stderr, "Cannot Exec.\n");
        return -1;
    }

    /* Variables used for handling server */
    struct inode *inode;
    void *packet = malloc(PACKET_SIZE);
    char dirname[DIRNAMELEN];
    int inum;

    while (1) {
        if ((pid = Receive(packet)) < 0) {
            fprintf(stderr, "Receive Error.\n");
            return -1;
        }

        switch (((UnknownPacket *)packet)->packet_type) {
          case MSG_GET_FILE:
            printf("MSG_GET_FILE received from pid: %d\n", pid);
            inum = ((DataPacket *)packet)->arg1;

            if (CopyFrom(pid, dirname, ((DataPacket *)packet)->pointer, DIRNAMELEN) < 0) {
                fprintf(stderr, "CopyFrom Error.\n");
                return -1;
            }

            printf("dirname: %s\n", dirname);

            /* Bleach packet for reuse */
            memset(packet, 0, PACKET_SIZE);
            ((FilePacket *)packet)->packet_type = MSG_GET_FILE;
            ((FilePacket *)packet)->inum = 0;

            inode = GetInode(inum);
            /*
             * Expected to search dirname in directory.
             * Everything else is irrelevant.
             */
            if (inode->type == INODE_DIRECTORY) {
                /*
                // Search inode to find relevant block
                int blockId = SearchDirectory(inode, dirname);
                if (blockId > 0) {
                    struct dir_entry *dir = (struct dir_entry *)GetBlock(blockId);
                    inode = GetInode(dir->inum);
                    ((FilePacket *)packet)->inum = dir->inum;
                    ((FilePacket *)packet)->type = inode->type;
                    ((FilePacket *)packet)->size = inode->size;
                    ((FilePacket *)packet)->nlink = inode->nlink;
                }
                */
            }

            if (Reply(packet, pid) < 0) {
                fprintf(stderr, "Reply Error.\n");
                return -1;
            }
            break;
          case MSG_CREATE_FILE:
            printf("MSG_CREATE_FILE received from pid: %d\n", pid);
            inum = ((DataPacket *)packet)->arg1;
            if (CopyFrom(pid, dirname, ((DataPacket *)packet)->pointer, DIRNAMELEN) < 0) {
                fprintf(stderr, "CopyFrom Error.\n");
                return -1;
            }

            /* Bleach packet for reuse */
            memset(packet, 0, PACKET_SIZE);
            ((FilePacket *)packet)->packet_type = MSG_CREATE_FILE;
            ((FilePacket *)packet)->inum = 0;
            inode = GetInode(inum);

            /*
             * Expected to create file in directory.
             * Everything else is irrelevant.
             */
            if (inode->type == INODE_DIRECTORY) {
                /*
                // Search inode to find relevant block
                int blockId = SearchDirectory(inode, dirname);
                if (blockId > 0) {
                    struct dir_entry *dir = (struct dir_entry *)GetBlock(blockId);
                    inode = GetInode(dir->inum);

                    // If inode exists, set its size as 0 and free all blocks
                    if (inode > 0) {
                        TruncateInode(inode);
                        ((FilePacket *)packet)->inum = dir->inum;
                    } else {
                        int next_inum = GetNextInodeNumber();
                        inode = CreateDirectory(inum, next_inum, dirname);
                        ((FilePacket *)packet)->inum = next_inum;
                    }
                    ((FilePacket *)packet)->type = inode->type;
                    ((FilePacket *)packet)->size = inode->size;
                    ((FilePacket *)packet)->nlink = inode->nlink;
                }
                */
            }

            if (Reply(packet, pid) < 0) {
                fprintf(stderr, "Reply Error.\n");
                return -1;
            }
            break;
          default:
            break;
        }
    }

    return 0;
};
