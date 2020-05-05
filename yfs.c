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
#include "dirname.h"

#define DIRSIZE             (int)sizeof(struct dir_entry)
#define MAX_DIRECT_SIZE     BLOCKSIZE * NUM_DIRECT
#define MAX_INDIRECT_SIZE   BLOCKSIZE * (BLOCKSIZE / sizeof(int))
#define MAX_FILE_SIZE       (int)(MAX_DIRECT_SIZE + MAX_INDIRECT_SIZE)
#define INODE_PER_BLOCK     (BLOCKSIZE / INODESIZE)
#define DIR_PER_BLOCK       (BLOCKSIZE / DIRSIZE)
#define GET_DIR_COUNT(n)    (n / DIRSIZE)

struct fs_header *header; /* Pointer to File System Header */

struct block_cache* block_stack; /* Cache for recently accessed blocks */
struct inode_cache* inode_stack; /* Cache for recently accessed inodes */

struct buffer* free_inode_list; /* List of Inodes available to assign to files */
struct buffer* free_block_list; /* List of blocks ready to allocate for file data */

/*
 * Simple helper for getting block count with inode->size
 */
int GetBlockCount(int size) {
    int block_count = size / BLOCKSIZE;
    if ((size % BLOCKSIZE) > 0) block_count++; // Round up
    return block_count;
}

/*
 * Print Everything inside Inode for Debugging purpose
 */
void PrintInode(struct inode *inode) {
    int i;
    int j;
    int block_count;
    int dir_count;
    int inner_count;
    struct dir_entry *block;
    char *char_block;
    // int *indirect_block;

    printf("----- Printing Inode -----\n");
    printf("inode->type: %d\n", inode->type);
    printf("inode->nlink: %d\n", inode->nlink);
    printf("inode->reuse: %d\n", inode->reuse);
    printf("inode->size: %d\n", inode->size);

    if (inode->type == INODE_DIRECTORY) {
        dir_count = GET_DIR_COUNT(inode->size);
        printf("dir_count: %d\n", dir_count);
    }

    block_count = GetBlockCount(inode->size);
    printf("block_count: %d\n", block_count);

    for (i = 0; i < block_count; i++) {
        printf("inode->direct[%d]: %d\n", i, inode->direct[i]);
        if (inode->type == INODE_DIRECTORY) {
            block = GetBlock(inode->direct[i])->block;
            if ((i + 1) * BLOCKSIZE > inode->size) {
                inner_count = dir_count % DIR_PER_BLOCK;
            } else {
                inner_count = DIR_PER_BLOCK;
            }
            for (j = 0; j < inner_count; j++) {
                printf("\t- inum: %d\n", block[j].inum);
                printf("\t- name: %s\n", block[j].name);
            }
        }

        if (inode->type == INODE_REGULAR) {
            char_block = GetBlock(inode->direct[i])->block;
            if ((i + 1) * BLOCKSIZE > inode->size) {
                inner_count = inode->size % BLOCKSIZE;
            } else {
                inner_count = BLOCKSIZE;
            }
            printf("--- Printing Block: %d ---\n", inner_count);
            for (j = 0; j < inner_count; j++) {
                printf("%c", char_block[j]);
            }
            printf("\n--- End of Block ---\n");
        }
    }

    if (inode->size >= MAX_DIRECT_SIZE) {
        printf("inode->indirect: %d\n", inode->indirect);
        // indirect_block = GetBlock(inode->indirect);
    }

    printf("----- End of Inode -----\n");
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
    while (i != value && index < size) {
        search_index++;
        i = arr[search_index];
    }
    if (search_index == index || i == -1 || i > size) return;
    int j = arr[index];
    arr[index] = value;
    arr[search_index] = j;
}

/**
 * Iterates through Inodes and pushes each free one to the buffer
 */
void GetFreeInodeList() {
    free_inode_list = GetBuffer(header->num_inodes);
    int i;
    for (i = 1; i <= header->num_inodes; i ++) {
        struct inode *next = GetInode(i)->inode;
        if (next->type == INODE_FREE) PushToBuffer(free_inode_list, i);
    }
}

/**
 * Function initializes the list numbers for blocks that have not been allocated yet
 */
void GetFreeBlockList() {
    /* Compute number of blocks occupited by inodes */
    int inode_count = header->num_inodes + 1;
    int inode_block_count = inode_count / INODE_PER_BLOCK;
    if (inode_count % INODE_PER_BLOCK > 0) inode_block_count++;

    /* Compute number of blocks with inodes and boot block excluded */
    int block_count = header->num_blocks - inode_block_count - 1;
    int first_data_block = 1 + inode_block_count;
    int *buffer = malloc(block_count * sizeof(int));
    int i;

    /* Block 0 is the boot block and not used by the file system */
    for (i = 0; i < block_count; i++) {
        buffer[i] = i + first_data_block;
    }


    /* Iterate through inodes to check which ones are using blocks */
    int j;
    int pos;
    int busy_blocks = 0;
    struct inode *scan;
    for (i = 1; i <= header->num_inodes; i ++) {
        scan = GetInode(i)->inode;

        /* Iterate through direct array to find first blocks */
        j = 0;
        pos = 0;
        while (pos < scan->size && j < NUM_DIRECT) {
            SearchAndSwap(buffer, block_count, scan->direct[j], busy_blocks);
            busy_blocks++;
            j++;
            pos += BLOCKSIZE;
        }
        /*
         * If indirect array is a non-zero value add it to the allocated block
         * and the array that's contained
         */
        if (pos < scan->size) {
            SearchAndSwap(buffer, header->num_blocks, scan->indirect, busy_blocks);
            busy_blocks++;
            int *indirect_blocks = GetBlock(scan->indirect)->block;
            j = 0;
            while (j < 128 && pos < scan->size) {
                SearchAndSwap(buffer, header->num_blocks, indirect_blocks[j], busy_blocks);
                busy_blocks++;
                j++;
                pos += BLOCKSIZE;
            }
        }
    }

    /* Initialize a special buffer */
    free_block_list = malloc(sizeof(struct buffer));
    free_block_list->size = block_count;
    free_block_list->b = buffer;
    free_block_list->in = free_block_list->size;
    free_block_list->out = 0;
    free_block_list->empty = 0;
    free_block_list->full = 1;

    int k;
    for (k = 0; k < busy_blocks; k++) {
        PopFromBuffer(free_block_list);
    }
}

/*
 * Return -1 if inum has no available room.
 * Create inode. Make sure cache knows it.
 * Register that to inum's inode and update size.
 */
struct inode* CreateDirectory(struct inode* parent_inode, int parent_inum, int new_inum, char *dirname, short type) {
    printf("CreateDirectory - parent_inum: %d\n", parent_inum);
    printf("CreateDirectory - new_inum: %d\n", new_inum);
    printf("CreateDirectory - dirname: %s\n", dirname);
    printf("CreateDirectory - type: %d\n", type);

    if (parent_inode->type != INODE_DIRECTORY) return NULL;
    if (parent_inode->size >= MAX_FILE_SIZE) {
        fprintf(stderr, "Maximum file size reached.\n");
        return NULL;
    }

    /* Verify against number of new blocks required */
    struct inode_cache_entry *inode_entry;
    struct block_cache_entry *block_entry;
    struct inode *inode;
    struct dir_entry *block;
    inode_entry = GetInode(new_inum);
    inode = inode_entry->inode;

    if (inode->type != INODE_FREE) {
        fprintf(stderr, "Why did inode buffer had used inode?\n");
        return NULL;
    }

    /* New inode is created and it is dirty */
    inode_entry->dirty = 1;
    inode->type = type;
    inode->size = 0;
    inode->nlink = 1;
    inode->reuse++;

    /* Create . and .. by default */
    if (type == INODE_DIRECTORY) {
        inode->nlink = 2;
        inode->size = sizeof(struct dir_entry) * 2;
        inode->direct[0] = PopFromBuffer(free_block_list);

        block_entry = GetBlock(inode->direct[0]);
        block_entry->dirty = 1;
        block = block_entry->block;
        block[0].inum = new_inum;
        block[1].inum = parent_inum;
        SetDirectoryName(block[0].name, ".", 0, 1);
        SetDirectoryName(block[1].name, "..", 0, 2);
    }

    /* Direct is full. Use indirect. */
    struct block_cache_entry *indirect_block_entry;
    int *indirect_block;
    int outer_index;
    int inner_index;

    if (parent_inode->size >= MAX_DIRECT_SIZE) {
        /* If it just reached MAX_DIRECT_SIZE, need extra block for indirect */
        if (parent_inode->size == MAX_DIRECT_SIZE) {
            parent_inode->indirect = PopFromBuffer(free_block_list);
        }

        indirect_block_entry = GetBlock(parent_inode->indirect);
        indirect_block = indirect_block_entry->block;

        outer_index = (parent_inode->size - MAX_DIRECT_SIZE) / BLOCKSIZE;
        inner_index = GET_DIR_COUNT(parent_inode->size) % DIR_PER_BLOCK;

        /*
         * If size is perfectly divisible by DIR_PER_BLOCK,
         * that means it is time to allocate new block.
         */
        if (inner_index == 0) {
            indirect_block[outer_index] = PopFromBuffer(free_block_list);
            indirect_block_entry->dirty = 1;
        }

        block_entry = GetBlock(indirect_block[outer_index]);
        block = block_entry->block;
    } else {
        outer_index = parent_inode->size / BLOCKSIZE;
        inner_index = GET_DIR_COUNT(parent_inode->size) % DIR_PER_BLOCK;
        printf("outer_index: %d\n", outer_index);
        printf("inner_index: %d\n", inner_index);
        /*
         * If size is perfectly divisible by DIR_PER_BLOCK,
         * that means it is time to allocate new block.
         */
        if (inner_index == 0) {
            parent_inode->direct[outer_index] = PopFromBuffer(free_block_list);
        }

        printf("parent_inode->direct[outer_index]: %d\n", parent_inode->direct[outer_index]);
        block_entry = GetBlock(parent_inode->direct[outer_index]);
        block = block_entry->block;
    }

    block[inner_index].inum = new_inum;
    SetDirectoryName(block[inner_index].name, dirname, 0, DIRNAMELEN);
    block_entry->dirty = 1;
    parent_inode->size += DIRSIZE;
    parent_inode->nlink += 1;

    printf("Printing parent inode before creating new file\n");
    PrintInode(parent_inode);
    return inode;
}

/*
 * Given directory inode and dirname,
 * find inode number which matches with dirname.
 */
int SearchDirectory(struct inode *inode, char *dirname) {
    if (inode->type != INODE_DIRECTORY) return 0;
    printf("SearchDirectory - inode->size: %d\n", inode->size);
    struct dir_entry *block;
    int dir_index = 0;
    int prev_index = -1;
    int outer_index;
    int inner_index;

    for (; dir_index < GET_DIR_COUNT(inode->size); dir_index++) {
        outer_index = dir_index / DIR_PER_BLOCK;
        inner_index = dir_index % DIR_PER_BLOCK;

        /* Get block if outer_index is incremented */
        if (prev_index < outer_index) {
            block = GetBlock(inode->direct[outer_index])->block;
            prev_index = outer_index;
        }

        if (CompareDirname(block[inner_index].name, dirname) == 0) {
            return block[inner_index].inum;
        }
    }

    return 0;
}

/*************************
 * File Request Hanlders *
 *************************/
void GetFile(FilePacket *packet) {
    int inum = packet->inum;
    struct inode *inode = GetInode(inum)->inode;

    /* Bleach packet for reuse */
    memset(packet, 0, PACKET_SIZE);
    packet->packet_type = MSG_SEARCH_FILE;
    packet->inum = inum;
    packet->type = inode->type;
    packet->size = inode->size;
    packet->nlink = inode->nlink;
    packet->reuse = inode->reuse;
}

int SearchFile(void *packet, int pid) {
    struct inode *parent_inode;
    struct inode *target_inode;

    char dirname[DIRNAMELEN];
    int inum = ((DataPacket *)packet)->arg1;
    void *target = ((DataPacket *)packet)->pointer;

    if (CopyFrom(pid, dirname, target, DIRNAMELEN) < 0) {
        fprintf(stderr, "CopyFrom Error.\n");
        return -1;
    }

    printf("GetFile - inum: %d\n", inum);
    printf("GetFile - dirname: %s\n", dirname);
    parent_inode = GetInode(inum)->inode;

    /* Bleach packet for reuse */
    memset(packet, 0, PACKET_SIZE);
    ((FilePacket *)packet)->packet_type = MSG_SEARCH_FILE;
    ((FilePacket *)packet)->inum = 0;

    /* Cannot search inside non-directory. */
    if (parent_inode->type != INODE_DIRECTORY) return 0;

    int target_inum = SearchDirectory(parent_inode, dirname);
    if (target_inum == 0) return 0;
    printf("target_inum: %d\n", target_inum);

    target_inode = GetInode(target_inum)->inode;
    ((FilePacket *)packet)->inum = target_inum;
    ((FilePacket *)packet)->type = target_inode->type;
    ((FilePacket *)packet)->size = target_inode->size;
    ((FilePacket *)packet)->nlink = target_inode->nlink;
    ((FilePacket *)packet)->reuse = target_inode->reuse;

    return 0;
}

int CreateFile(void *packet, int pid, short type) {
    struct inode_cache_entry *parent_entry;
    struct inode_cache_entry *entry;
    struct inode *parent_inode;
    struct inode *new_inode;

    char dirname[DIRNAMELEN];
    int parent_inum = ((DataPacket *)packet)->arg1;
    void *target = ((DataPacket *)packet)->pointer;

    if (CopyFrom(pid, dirname, target, DIRNAMELEN) < 0) {
        fprintf(stderr, "CopyFrom Error.\n");
        return -1;
    }

    parent_entry = GetInode(parent_inum);
    parent_inode = parent_entry->inode;

    /* Bleach packet for reuse */
    memset(packet, 0, PACKET_SIZE);
    ((FilePacket *)packet)->packet_type = MSG_CREATE_FILE;
    ((FilePacket *)packet)->inum = 0;

    /* Cannot create inside non-directory. */
    if (parent_inode->type != INODE_DIRECTORY) return 0;

    int target_inum = SearchDirectory(parent_inode, dirname);
    if (target_inum > 0) {
        // Inode is found. Truncate
        entry = GetInode(target_inum);
        entry->dirty = 1;
        new_inode = entry->inode;
        new_inode->size = 0;
        new_inode->nlink = 0; // ??
        // TODO: Free all blocks here
    } else {
        // Create new file if not found
        target_inum = PopFromBuffer(free_inode_list);
        new_inode = CreateDirectory(parent_inode, parent_inum, target_inum, dirname, type);
        parent_entry->dirty = 1;
    }

    ((FilePacket *)packet)->inum = target_inum;
    ((FilePacket *)packet)->type = new_inode->type;
    ((FilePacket *)packet)->size = new_inode->size;
    ((FilePacket *)packet)->nlink = new_inode->nlink;
    ((FilePacket *)packet)->reuse = new_inode->reuse;
    return 0;
}

void ReadFile(DataPacket *packet, int pid) {
    int inum = packet->arg1;
    int pos = packet->arg2;
    int size = packet->arg3;
    int reuse = packet->arg4;
    void *buffer = packet->pointer;
    struct inode *inode;

    printf("ReadFile - inum: %d\n", inum);
    printf("ReadFile - pos: %d\n", pos);
    printf("ReadFile - size: %d\n", size);
    printf("ReadFile - reuse: %d\n", reuse);

    /* Bleach packet for reuse */
    memset(packet, 0, PACKET_SIZE);
    packet->packet_type = MSG_READ_FILE;

    inode = GetInode(inum)->inode;
    if (inode->reuse != reuse) {
        packet->arg1 = -1;
        return;
    }

    /* If trying to read more than size, adjust size */
    if (pos + size > inode->size) {
        size = inode->size - pos;

        /* If pos is already beyond size, read 0 */
        if (size <= 0) {
            packet->arg1 = 0;
            return;
        }
    }

    int inode_block_count = GetBlockCount(inode->size);
    int start_index = pos / BLOCKSIZE; /* Block index where writing starts */
    int end_index = (pos + size) / BLOCKSIZE; /* Block index where writing ends */
    /* ex) if pos = 0 and size = 512, it should only iterate 0 ~ 0 */
    if ((pos + size) % BLOCKSIZE == 0) end_index--;

    printf("start_index: %d\n", start_index);
    printf("end_index: %d\n", end_index);
    printf("inode_block_count: %d\n", inode_block_count);

    /* Just in case there is a hole */
    char hole_buffer[BLOCKSIZE];
    memset(hole_buffer, 0, BLOCKSIZE);

    /* Start reading from the blocks */
    int *indirect_block = NULL;
    char *block;
    int block_id;
    int copied_size = 0;
    int prefix = 0;
    int copysize;
    int i;

    /* Prefetch indirect block */
    if (inode->size >= MAX_DIRECT_SIZE) {
        indirect_block = GetBlock(inode->indirect)->block;
    }

    for (i = start_index; i <= end_index; i++) {
        if (i >= NUM_DIRECT) {
            block_id = indirect_block[i - NUM_DIRECT];
        } else {
            block_id = inode->direct[i];
        }

        /* Use hole block if block does not exist */
        if (block_id != 0) block = GetBlock(block_id)->block;
        else block = hole_buffer;

        /*
         * If current_pos is not divisible by BLOCKSIZE,
         * this does not start from the beginning of the block.
         */
        prefix = (pos + copied_size) % BLOCKSIZE;
        copysize = BLOCKSIZE - prefix;

        /* Adjust size at last index */
        if (i == end_index) {
            copysize = size - copied_size;
        }

        printf("CopyTo - prefix: %d\n", prefix);
        printf("CopyTo - copied_size: %d\n", copied_size);
        printf("CopyTo - copysize: %d\n", copysize);
        if (copysize > 0) {
            CopyTo(pid, buffer + copied_size, block + prefix, copysize);
            copied_size += copysize;
        }
    }

    printf("Final copied size: %d\n", copied_size);
    packet->arg1 = copied_size;
}

void WriteFile(DataPacket *packet, int pid) {
    int inum = packet->arg1;
    int pos = packet->arg2;
    int size = packet->arg3;
    int reuse = packet->arg4;
    void *buffer = packet->pointer;
    struct inode_cache_entry *inode_entry;
    struct inode *inode;

    printf("WriteFile - inum: %d\n", inum);
    printf("WriteFile - pos: %d\n", pos);
    printf("WriteFile - size: %d\n", size);
    printf("WriteFile - reuse: %d\n", reuse);

    /* Bleach packet for reuse */
    memset(packet, 0, PACKET_SIZE);
    packet->packet_type = MSG_WRITE_FILE;

    /* Attempting to write beyond max file size */
    if (pos + size > MAX_FILE_SIZE) {
        packet->arg1 = -1;
        return;
    }

    /* Cannot write to non-regular file */
    inode_entry = GetInode(inum);
    inode = inode_entry->inode;
    if (inode->type != INODE_REGULAR) {
        fprintf(stderr, "Trying to write to non-regular file.\n");
        packet->arg1 = -2;
        return;
    }

    if (inode->reuse != reuse) {
        packet->arg1 = -3;
        return;
    }

    int *indirect_block = NULL;
    int inode_block_count = GetBlockCount(inode->size);
    int start_index = pos / BLOCKSIZE; /* Block index where writing starts */
    int end_index = (pos + size) / BLOCKSIZE; /* Block index where writing ends */
    /* ex) if pos = 0 and size = 512, it should only iterate 0 ~ 0 */
    if ((pos + size) % BLOCKSIZE == 0) end_index--;

    /* Prefetch indirect block */
    if (inode->size >= MAX_DIRECT_SIZE) {
        indirect_block = GetBlock(inode->indirect)->block;
    }

    /*
     * Start iterating from whichever the lowest between start and block count.
     * - Increase size if end_index is greater than block_count.
     * - Assign new block only if size is increased and within writing range.
     * - Note: @634 if block is entirely hole, it is 0.
     */
    int i = start_index;
    if (i < inode_block_count) i = inode_block_count;
    for (; i <= end_index; i++) {
        /* Increase the size if current index is less than or equal to block count */
        if (i <= inode_block_count) {
            /* If i is at NUM_DIRECT, it is about to create indirect */
            if (i == NUM_DIRECT) {
                inode->indirect = PopFromBuffer(free_block_list);
                indirect_block = GetBlock(inode->indirect)->block;
                memset(indirect_block, 0, BLOCKSIZE);
            }

            /* Assign new block if size increased AND is part of writing range */
            if (i <= start_index) {
                if (i >= NUM_DIRECT) {
                    /* Create new block in indirect block. */
                    indirect_block[i - NUM_DIRECT] = PopFromBuffer(free_block_list);
                } else {
                    /* Create new block at direct */
                    inode->direct[i] = PopFromBuffer(free_block_list);
                    inode_entry->dirty = 1;
                    printf("inode->direct[i]: %d\n", inode->direct[i]);
                }
            }
        }
    }

    /* Start writing in the block */
    struct block_cache_entry *block_entry;
    char *block;
    int block_id;
    int prefix = 0;
    int copied_size = 0; /* Total copied size */
    int copysize; /* size used for CopyFrom */

    /*
     * New file size based on the write operation.
     * If start writing from index 1, then block size is 512+.
     */
    int new_size = start_index * BLOCKSIZE;

    printf("start_index: %d\n", start_index);
    printf("end_index: %d\n", end_index);

    for (i = start_index; i <= end_index; i++) {
        if (i >= NUM_DIRECT) {
            block_id = indirect_block[i - NUM_DIRECT];
        } else {
            block_id = inode->direct[i];
        }

        block_entry = GetBlock(block_id);
        block = block_entry->block;
        /*
         * If current_pos is not divisible by BLOCKSIZE,
         * this does not start from the beginning of the block.
         */
        prefix = (pos + copied_size) % BLOCKSIZE;
        copysize = BLOCKSIZE - prefix;

        /* Prefix should impact new size */
        if (i == start_index) {
            new_size += prefix;
        }

        /* Adjust size at last index */
        if (i == end_index) {
            copysize = size - copied_size;
        }

        printf("Copyfrom - prefix: %d\n", prefix);
        printf("Copyfrom - copied_size: %d\n", copied_size);
        printf("Copyfrom - copysize: %d\n", copysize);
        CopyFrom(pid, block + prefix, buffer + copied_size, copysize);
        copied_size += copysize;
        block_entry->dirty = 1;
    }
    new_size += copied_size;
    printf("Final copied size: %d\n", copied_size);
    printf("Old file size: %d\n", inode->size);
    printf("New file size: %d\n", new_size);
    if (new_size > inode->size) {
        inode->size = new_size;
        inode_entry->dirty = 1;
    }
    packet->arg1 = copied_size;

    PrintInode(inode);
}

void DeleteDir(DataPacket *packet) {
    int inum = packet->arg1;
    if (inum == ROOTINODE) {
        packet->arg1 = -1;
        return;
    }

    memset(packet, 0, PACKET_SIZE);
    packet->packet_type = MSG_DELETE_DIR;

    struct inode_cache_entry *inode_entry;
    struct inode *inode;
    inode_entry = GetInode(inum);
    inode = inode_entry->inode;

    /* Cannot call deleteDir on non-directory */
    if (inode->type != INODE_DIRECTORY) {
        packet->arg1 = -2;
        return;
    }

    /* There should be . and .. left only */
    if (inode->size > DIRSIZE * 2) {
        packet->arg1 = -3;
        return;
    }

    inode_entry->dirty = 1;
    inode->type = INODE_FREE;
    inode->size = 0;
    inode->nlink = 0;

    struct block_cache_entry *block_entry;
    struct dir_entry *block;
    int i;

    block_entry = GetBlock(inode->direct[0]);
    block_entry->dirty = 1;
    block = block_entry->block;

    for (i = 0; i < 2; i++) {
        /* Need to decrement nlink of the parent */
        if (block[i].name[1] == '.') {
            inode_entry = GetInode(block[i].inum);
            inode_entry->inode->nlink -= 1;
            inode_entry->dirty = 1;
        }
        block[i].inum = 0;
    }
}

/**
 * Writes all Dirty Inodes
 */
void SyncCache() {
    /**
     * Synchronize Inodes in Cache to Blocks in Cache
     */
    struct inode_cache_entry* inode;
    for (inode = inode_stack->top; inode != NULL; inode = inode->next_lru) {
        if (inode->dirty) {
            void* inode_block = GetBlock((inode->inum / 8) + 1);
            struct inode* overwrite = (struct inode *)inode_block + (inode->inum % 8);
            memcpy(overwrite, inode->inode, sizeof(inode));
            inode->dirty = 0;
        }
    }

    /**
     * Synchronize Blocks in Cache to Disk
     */
    struct block_cache_entry* block;
    for (block = block_stack->top; block != NULL; block = block->next_lru) {
        if (block->dirty) {
            WriteSector(block->block_number,block->block);
            block->dirty = 0;
        }
    }
    return;
}

int main(int argc, char **argv) {
    Register(FILE_SERVER);
    (void)argc;
    /* Obtain File System Header */
    void *sector_one = malloc(SECTORSIZE);
    if (ReadSector(1, sector_one) == 0) {
        header = (struct fs_header *)sector_one;
    } else {
        printf("Error\n");
    }

    inode_stack = CreateInodeCache(header->num_inodes);
    block_stack = CreateBlockCache(header->num_blocks);
    GetFreeInodeList();
    GetFreeBlockList();

    int pid;
  	if ((pid = Fork()) < 0) {
  	    fprintf(stderr, "Cannot Fork.\n");
  	    return -1;
  	}

    /* Child process exec program */
    if (pid == 0) {
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

        if (pid == 0) continue;

        switch (((UnknownPacket *)packet)->packet_type) {
            case MSG_GET_FILE:
                printf("MSG_GET_FILE received from pid: %d\n", pid);
                GetFile(packet);
                break;
            case MSG_SEARCH_FILE:
                printf("MSG_SEARCH_FILE received from pid: %d\n", pid);
                SearchFile(packet, pid);
                break;
            case MSG_CREATE_FILE:
                printf("MSG_CREATE_FILE received from pid: %d\n", pid);
                CreateFile(packet, pid, INODE_REGULAR);
                break;
            case MSG_READ_FILE:
                printf("MSG_READ_FILE received from pid: %d\n", pid);
                ReadFile(packet, pid);
                break;
            case MSG_WRITE_FILE:
                printf("MSG_WRITE_FILE received from pid: %d\n", pid);
                WriteFile(packet, pid);
                break;
            case MSG_CREATE_DIR:
                printf("MSG_CREATE_DIR received from pid: %d\n", pid);
                CreateFile(packet, pid, INODE_DIRECTORY);
                break;
            case MSG_DELETE_DIR:
                printf("MSG_DELETE_DIR received from pid: %d\n", pid);
                DeleteDir(packet);
                break;
            case MSG_SYNC:
                printf("MSG_SYNC received from pid: %d\n", pid);
                SyncCache();
                if (((DataPacket *)packet)->arg1 == 1) {
                    Reply(packet, pid);
                    printf("Shutdown by pid: %d. Bye bye!\n", pid);
                    Exit(0);
                }
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
