#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

/*
 * TODO:
 * Read inode from pos to pos + size and invoke CopyTo to target buffer.
 * Return copied size. Return -1 if nothing is copied.
 */
int ReadFromInode(int pid, int inum, int pos, int size, char *buffer);

/*
 * TODO:
 * Write to inode from pos to pos + size with data obtained from CopyFrom.
 * Return copied size. Return -1 if nothing is copied.
 */
int WriteToInode(int pid, int inum, int pos, int size, char *buffer);


/*************************
 * File Request Hanlders *
 *************************/
int GetFile(void *packet, int pid) {
    struct inode *inode;
    char dirname[DIRNAMELEN];
    int inum = ((DataPacket *)packet)->arg1;
    void *target = ((DataPacket *)packet)->pointer;

    if (CopyFrom(pid, dirname, target, DIRNAMELEN) < 0) {
        fprintf(stderr, "CopyFrom Error.\n");
        return -1;
    }

    inode = GetInode(inum);

    /* Bleach packet for reuse */
    memset(packet, 0, PACKET_SIZE);
    ((FilePacket *)packet)->packet_type = MSG_GET_FILE;
    ((FilePacket *)packet)->inum = 0;

    /*
     * Cannot search inside non-directory.
     * TODO: return here for link
     */
    if (inode->type != INODE_DIRECTORY) return 0;

    /*
    // Search inode to find relevant block
    int blockId = SearchDirectory(inode, dirname);
    if (blockId == 0) return 0;

    // Find inode of target directory
    struct dir_entry *dir = (struct dir_entry *)GetBlock(blockId);
    inode = GetInode(dir->inum);
    ((FilePacket *)packet)->inum = dir->inum;
    ((FilePacket *)packet)->type = inode->type;
    ((FilePacket *)packet)->size = inode->size;
    ((FilePacket *)packet)->nlink = inode->nlink;
    */
    return 0;
}

int CreateFile(void *packet, int pid) {
    struct inode *inode;
    char dirname[DIRNAMELEN];
    int inum = ((DataPacket *)packet)->arg1;
    void *target = ((DataPacket *)packet)->pointer;

    if (CopyFrom(pid, dirname, target, DIRNAMELEN) < 0) {
        fprintf(stderr, "CopyFrom Error.\n");
        return -1;
    }

    inode = GetInode(inum);

    /* Bleach packet for reuse */
    memset(packet, 0, PACKET_SIZE);
    ((FilePacket *)packet)->packet_type = MSG_CREATE_FILE;
    ((FilePacket *)packet)->inum = 0;

    /*
     * Cannot create inside non-directory.
     * TODO: return here for link
     */
    if (inode->type != INODE_DIRECTORY) return 0;

    /*
    int blockId = SearchDirectory(inode, dirname);
    int target_inum;
    if (blockId > 0) {
        // Inode is found. Truncate
        struct dir_entry *dir = (struct dir_entry *)GetBlock(blockId);
        inode = GetInode(dir->inum);
        TruncateInode(inode);
        target_inum = dir->inum;
    } else {
        // Create new file if not found
        target_inum = GetNextInodeNumber();
        inode = CreateDirectory(inum, next_inum, dirname);
    }

    ((FilePacket *)packet)->inum = target_inum;
    ((FilePacket *)packet)->type = inode->type;
    ((FilePacket *)packet)->size = inode->size;
    ((FilePacket *)packet)->nlink = inode->nlink;
    */
    return 0;
}

void ReadFile(DataPacket *packet, int pid) {
    int inum = packet->arg1;
    int pos = packet->arg2;
    int size = packet->arg3;
    void *buffer = packet->pointer;

    // int result = ReadFromInode(
    //     pid,
    //     inum,
    //     ((DataPacket *)packet)->arg2, // pos
    //     ((DataPacket *)packet)->arg3, // size
    //     ((DataPacket *)packet)->pointer // buffer
    // );

    memset(packet, 0, PACKET_SIZE);
    packet->packet_type = MSG_READ_FILE;
    packet->arg1 = -1;
}

void WriteFile(DataPacket *packet, int pid) {
    int inum = packet->arg1;
    int pos = packet->arg2;
    int size = packet->arg3;
    void *buffer = packet->pointer;

    // int result = WriteToInode(
    //     pid,
    //     inum,
    //     ((DataPacket *)packet)->arg2, // pos
    //     ((DataPacket *)packet)->arg3, // size
    //     ((DataPacket *)packet)->pointer // buffer
    // );

    memset(packet, 0, PACKET_SIZE);
    packet->packet_type = MSG_WRITE_FILE;
    packet->arg1 = -1;
}

void CreateDir(void *packet, int pid) {
    char dirname[DIRNAMELEN];
    int inum = ((DataPacket *)packet)->arg1;
    void *target = ((DataPacket *)packet)->pointer;

    memset(packet, 0, PACKET_SIZE);
    ((FilePacket *)packet)->packet_type = MSG_CREATE_DIR;
    ((FilePacket *)packet)->inum = 0;
}

void DeleteDir(DataPacket *packet, int pid) {
    int inum = packet->arg1;

    memset(packet, 0, PACKET_SIZE);
    packet->packet_type = MSG_DELETE_DIR;
    packet->arg1 = -1;
}

void SyncCache() {

}

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

    void *packet = malloc(PACKET_SIZE);
    while (1) {
        if ((pid = Receive(packet)) < 0) {
            fprintf(stderr, "Receive Error.\n");
            return -1;
        }

        switch (((UnknownPacket *)packet)->packet_type) {
            case MSG_GET_FILE:
                printf("MSG_GET_FILE received from pid: %d\n", pid);
                GetFile(packet, pid);
                break;
            case MSG_CREATE_FILE:
                printf("MSG_CREATE_FILE received from pid: %d\n", pid);
                CreateFile(packet, pid);
                break;
            case MSG_READ_FILE:
                printf("MSG_READ_FILE received from pid: %d\n", pid);
                ReadFile(packet, pid);
                break;
            case MSG_WRITE_FILE:
                printf("MSG_WRITE_FILE received from pid: %d\n", pid);
                WriteFile(packet, pid);
            case MSG_CREATE_DIR:
                printf("MSG_CREATE_DIR received from pid: %d\n", pid);
                CreateDir(packet, pid);
                break;
            case MSG_DELETE_DIR:
                printf("MSG_DELETE_DIR received from pid: %d\n", pid);
                DeleteDir(packet, pid);
                break;
            case MSG_SYNC:
                printf("MSG_SYNC received from pid: %d\n", pid);
                SyncCache();
                break;
            default:
                continue;
        }

        if (Reply(packet, pid) < 0) {
            fprintf(stderr, "Reply Error.\n");
            return -1;
        }
    }

    return 0;
};
