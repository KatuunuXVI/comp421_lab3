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

#define DEBUG 0
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
    int block_id;
    struct dir_entry *block;
    char *char_block;
    int *indirect_block;

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

    if (inode->size >= MAX_DIRECT_SIZE) {
        indirect_block = GetBlock(inode->indirect)->block;
        printf("indirect blocks %d:\n", inode->indirect);
        for (i = 0; i < block_count - NUM_DIRECT; i++) {
            printf("\t- %d\n", indirect_block[i]);
        }
    }

    for (i = 0; i < block_count; i++) {
        if (i >= NUM_DIRECT) {
            block_id = indirect_block[i - NUM_DIRECT];
            printf("indirect[%d]: %d\n", i - NUM_DIRECT, block_id);
        } else {
            block_id = inode->direct[i];
            printf("direct[%d]: %d\n", i, block_id);
        }

        if (block_id == 0) continue;
        if (inode->type == INODE_DIRECTORY) {
            block = GetBlock(block_id)->block;
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
            char_block = GetBlock(block_id)->block;
            if ((i + 1) * BLOCKSIZE > inode->size) {
                inner_count = inode->size % BLOCKSIZE;
            } else {
                inner_count = BLOCKSIZE;
            }

            printf("----- Printing Block -----\n");
            for (j = 0; j < inner_count; j++) {
                printf("%c", char_block[j]);
            }
            printf("\n----- End of Block -----\n");
        }
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
 * Create a new file inode using provided arguments
 */
struct inode* CreateFileInode(int new_inum, int parent_inum, short type) {
    struct inode_cache_entry *inode_entry = GetInode(new_inum);
    struct inode *inode = inode_entry->inode;
    struct block_cache_entry *block_entry;
    struct dir_entry *block;

    /* New inode is created and it is dirty */
    inode_entry->dirty = 1;
    inode->type = type;
    inode->size = 0;
    inode->nlink = 0;
    inode->reuse++;

    /* Create . and .. by default */
    if (type == INODE_DIRECTORY) {
        inode->nlink = 1; /* Link to itself */
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

    return inode;
}

/*
 * Truncate file inode
 */
struct inode* TruncateFileInode(int target_inum) {
    if (DEBUG) printf("Truncating %d\n", target_inum);
    struct inode_cache_entry *entry = GetInode(target_inum);
    struct inode *inode = entry->inode;
    struct block_cache_entry *indirect_block_entry;
    int *indirect_block = NULL;

    entry = GetInode(target_inum);
    entry->dirty = 1;
    inode = entry->inode;

    int block_count = GetBlockCount(inode->size);
    int i;

    /* Need to free indirect blocks */
    int iterate_count = block_count;
    if (block_count > NUM_DIRECT) {
        iterate_count = NUM_DIRECT;
        indirect_block_entry = GetBlock(inode->indirect);
        indirect_block = indirect_block_entry->block;
        indirect_block_entry->dirty = 1;
        for (i = 0; i < block_count - NUM_DIRECT; i++) {
            if (indirect_block[i] != 0) {
                if (DEBUG) printf("Freed block: %d\n", indirect_block[i]);
                PushToBuffer(free_block_list, indirect_block[i]);
            }
        }
        if (inode->indirect != 0) {
            if (DEBUG) printf("Freed block: %d\n", inode->indirect);
            PushToBuffer(free_block_list, inode->indirect);
        }
    }

    for (i = 0; i < iterate_count; i++) {
        if (inode->direct[i] != 0) {
            if (DEBUG) printf("Freed block: %d\n", inode->direct[i]);
            PushToBuffer(free_block_list, inode->direct[i]);
        }
    }

    inode->size = 0;
    return inode;
}

/*
 * Register provided inum and dirname to directory inode.
 * Return 1 if parent inode becomes dirty for this action.
 */
int RegisterDirectory(struct inode* parent_inode, int new_inum, char *dirname) {
    /* Verify against number of new blocks required */
    struct block_cache_entry *block_entry;
    struct block_cache_entry *indirect_block_entry;
    struct dir_entry *block;
    int *indirect_block = NULL;
    int dir_index = 0;
    int prev_index = -1;
    int outer_index; /* index of direct or indirect */
    int inner_index; /* index of dir_entry array */

    /* Similar process as SearchDirectory to find available inum */
    for (; dir_index < GET_DIR_COUNT(parent_inode->size); dir_index++) {
        outer_index = dir_index / DIR_PER_BLOCK;
        inner_index = dir_index % DIR_PER_BLOCK;

        /* Get block if outer_index is incremented */
        if (prev_index != outer_index) {
            if (outer_index >= NUM_DIRECT) {
                if (indirect_block == NULL) {
                    indirect_block = GetBlock(parent_inode->indirect)->block;
                }
                block_entry = GetBlock(indirect_block[outer_index - NUM_DIRECT]);
                block = block_entry->block;
            } else {
                block_entry = GetBlock(parent_inode->direct[outer_index]);
                block = block_entry->block;
            }
            prev_index = outer_index;
        }

        /* Found empty inum. Finished! */
        if (block[inner_index].inum == 0) {
            block[inner_index].inum = new_inum;
            SetDirectoryName(block[inner_index].name, dirname, 0, DIRNAMELEN);
            block_entry->dirty = 1;
            return 0;
        }
    }

    /* No inum = 0 is found. Need to append and increase size */
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
        /*
         * If size is perfectly divisible by DIR_PER_BLOCK,
         * that means it is time to allocate new block.
         */
        if (inner_index == 0) {
            parent_inode->direct[outer_index] = PopFromBuffer(free_block_list);
        }

        if (DEBUG) printf("parent_inode->direct[outer_index]: %d\n", parent_inode->direct[outer_index]);
        block_entry = GetBlock(parent_inode->direct[outer_index]);
        block = block_entry->block;
    }

    /* Register inum and dirname in the dir_entry */
    block[inner_index].inum = new_inum;
    SetDirectoryName(block[inner_index].name, dirname, 0, DIRNAMELEN);
    block_entry->dirty = 1;
    parent_inode->size += DIRSIZE;
    return 1;
}

/*
 * Remove provided inum in the parent directory
 * Return -1 if target inum is not found.
 */
int UnregisterDirectory(struct inode* parent_inode, int target_inum) {
    int *indirect_block = NULL;
    struct block_cache_entry *block_entry;
    struct dir_entry *block;
    int dir_index = 0;
    int prev_index = -1;
    int outer_index;
    int inner_index;

    for (; dir_index < GET_DIR_COUNT(parent_inode->size); dir_index++) {
        outer_index = dir_index / DIR_PER_BLOCK;
        inner_index = dir_index % DIR_PER_BLOCK;

        /* Get block if outer_index is incremented */
        if (prev_index != outer_index) {
            if (outer_index >= NUM_DIRECT) {
                if (indirect_block == NULL) {
                    indirect_block = GetBlock(parent_inode->indirect)->block;
                }
                block_entry = GetBlock(indirect_block[outer_index - NUM_DIRECT]);
                block = block_entry->block;
            } else {
                block_entry = GetBlock(parent_inode->direct[outer_index]);
                block = block_entry->block;
            }
            prev_index = outer_index;
        }

        /* Found it! */
        if (block[inner_index].inum == target_inum) {
            block[inner_index].inum = 0;
            block_entry->dirty = 1;
            return 0;
        }
    }

    return -1;
}

/*
 * Given directory inode and dirname,
 * find inode number which matches with dirname.
 */
int SearchDirectory(struct inode *inode, char *dirname) {
    int *indirect_block = NULL;
    struct dir_entry *block;
    int dir_index = 0;
    int prev_index = -1;
    int outer_index; /* index of direct or indirect */
    int inner_index; /* index of dir_entry array */

    for (; dir_index < GET_DIR_COUNT(inode->size); dir_index++) {
        outer_index = dir_index / DIR_PER_BLOCK;
        inner_index = dir_index % DIR_PER_BLOCK;

        /* Get block if outer_index is incremented */
        if (prev_index != outer_index) {
            if (outer_index >= NUM_DIRECT) {
                if (indirect_block == NULL) {
                    indirect_block = GetBlock(inode->indirect)->block;
                }
                block = GetBlock(indirect_block[outer_index - NUM_DIRECT])->block;
            } else {
                block = GetBlock(inode->direct[outer_index])->block;
            }
            prev_index = outer_index;
        }

        if (block[inner_index].inum == 0) continue;
        if (CompareDirname(block[inner_index].name, dirname) == 0) {
            return block[inner_index].inum;
        }
    }

    return 0;
}

/*
 * Given directory inode with a link deleted,
 * update size and free blocks if necessary.
 */
int CleanDirectory(struct inode *inode) {
    int dirty = 0;
    struct block_cache_entry *indirect_block_entry;
    struct dir_entry *block;
    int *indirect_block = NULL;
    int dir_index = GET_DIR_COUNT(inode->size) - 1;
    int prev_index = -1;
    int outer_index; /* index of direct or indirect */
    int inner_index; /* index of dir_entry array */

    /* Iterate from backward */
    for (; dir_index > 0; dir_index--) {
        outer_index = dir_index / DIR_PER_BLOCK;
        inner_index = dir_index % DIR_PER_BLOCK;

        /* Get block if outer_index is incremented */
        if (prev_index != outer_index) {
            if (outer_index >= NUM_DIRECT) {
                if (indirect_block == NULL) {
                    indirect_block_entry = GetBlock(inode->indirect);
                    indirect_block = indirect_block_entry->block;
                }
                block = GetBlock(indirect_block[outer_index - NUM_DIRECT])->block;
            } else {
                block = GetBlock(inode->direct[outer_index])->block;
            }

            /* If block is switched 2nd+ time, that block needs to be freed */
            if (prev_index > 0) {
                if (prev_index >= NUM_DIRECT) {
                    if (indirect_block[outer_index - NUM_DIRECT] != 0) {
                        if (DEBUG) printf("Freeing block: %d\n", indirect_block[prev_index - NUM_DIRECT]);
                        PushToBuffer(free_block_list, indirect_block[prev_index - NUM_DIRECT]);
                    }

                    /* Need to free indirect block as well */
                    if (prev_index == NUM_DIRECT && inode->indirect != 0) {
                        if (DEBUG) printf("Freeing block: %d\n", inode->indirect);
                        PushToBuffer(free_block_list, inode->indirect);
                    }
                } else if (inode->direct[prev_index] != 0) {
                    if (DEBUG) printf("Freeing block: %d\n", inode->direct[prev_index]);
                    PushToBuffer(free_block_list, inode->direct[prev_index]);
                }
            }
            prev_index = outer_index;
        }

        /* Found valid directory */
        if (block[inner_index].inum != 0) break;

        /* Decrease parent size */
        dirty = 1;
        inode->size -= DIRSIZE;
    }

    return dirty;
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

void SearchFile(void *packet, int pid) {
    struct inode *parent_inode;
    struct inode *target_inode;

    char dirname[DIRNAMELEN];
    int inum = ((DataPacket *)packet)->arg1;
    void *target = ((DataPacket *)packet)->pointer;

    /* Bleach packet for reuse */
    memset(packet, 0, PACKET_SIZE);
    ((FilePacket *)packet)->packet_type = MSG_SEARCH_FILE;
    ((FilePacket *)packet)->inum = 0;

    if (CopyFrom(pid, dirname, target, DIRNAMELEN) < 0) return;

    if (DEBUG) {
        printf("SearchFile - inum: %d\n", inum);
        printf("SearchFile - dirname: %s\n", dirname);
    }
    parent_inode = GetInode(inum)->inode;


    /* Cannot search inside non-directory. */
    if (parent_inode->type != INODE_DIRECTORY) return;
    int target_inum = SearchDirectory(parent_inode, dirname);

    /* Not found */
    if (target_inum == 0) return;

    target_inode = GetInode(target_inum)->inode;
    ((FilePacket *)packet)->inum = target_inum;
    ((FilePacket *)packet)->type = target_inode->type;
    ((FilePacket *)packet)->size = target_inode->size;
    ((FilePacket *)packet)->nlink = target_inode->nlink;
    ((FilePacket *)packet)->reuse = target_inode->reuse;

    return 0;
}

void CreateFile(void *packet, int pid, short type) {
    struct inode_cache_entry *parent_entry;
    struct inode *parent_inode;
    struct inode *new_inode;

    char dirname[DIRNAMELEN];
    int parent_inum = ((DataPacket *)packet)->arg1;
    void *target = ((DataPacket *)packet)->pointer;

    /* Bleach packet for reuse */
    memset(packet, 0, PACKET_SIZE);
    ((FilePacket *)packet)->packet_type = MSG_CREATE_FILE;

    if (CopyFrom(pid, dirname, target, DIRNAMELEN) < 0) {
        ((FilePacket *)packet)->inum = 0;
        return;
    }

    parent_entry = GetInode(parent_inum);
    parent_inode = parent_entry->inode;

    /* Cannot create inside non-directory. */
    if (parent_inode->type != INODE_DIRECTORY) {
        ((FilePacket *)packet)->inum = -1;
        return;
    }

    /* Maximum file size reached. */
    if (parent_inode->size >= MAX_FILE_SIZE) {
        ((FilePacket *)packet)->inum = -2;
        return;
    }

    if (DEBUG) {
        printf("Printing parent inode %d before creating new file\n", parent_inum);
        PrintInode(parent_inode);
    }

    int target_inum = SearchDirectory(parent_inode, dirname);
    if (target_inum > 0) {
        new_inode = TruncateFileInode(target_inum);
    } else {
        /* If no free inode to spare, error */
        if (free_inode_list->size == 0) {
            ((FilePacket *)packet)->inum = -3;
            return;
        }

        /*
         * Creating a directory will require 1 block.
         * Adding it to parent inode may require 2 blocks.
         */
        if (free_block_list->size < 3) {
            ((FilePacket *)packet)->inum = -4;
            return;
        }

        // Create new file if not found
        target_inum = PopFromBuffer(free_inode_list);
        new_inode = CreateFileInode(target_inum, parent_inum, type);

        /* Child directory refers to parent via .. */
        if (type == INODE_DIRECTORY) parent_inode->nlink += 1;
        parent_entry->dirty = RegisterDirectory(parent_inode, target_inum, dirname);

        if (DEBUG) {
            printf("Printing parent inode %d after creating new file\n", parent_inum);
            PrintInode(parent_inode);
        }

        new_inode->nlink += 1;
    }

    ((FilePacket *)packet)->inum = target_inum;
    ((FilePacket *)packet)->type = new_inode->type;
    ((FilePacket *)packet)->size = new_inode->size;
    ((FilePacket *)packet)->nlink = new_inode->nlink;
    ((FilePacket *)packet)->reuse = new_inode->reuse;
}

void ReadFile(DataPacket *packet, int pid) {
    int inum = packet->arg1;
    int pos = packet->arg2;
    int size = packet->arg3;
    int reuse = packet->arg4;
    void *buffer = packet->pointer;
    struct inode *inode;

    if (DEBUG) {
        printf("ReadFile - inum: %d\n", inum);
        printf("ReadFile - pos: %d\n", pos);
        printf("ReadFile - size: %d\n", size);
        printf("ReadFile - reuse: %d\n", reuse);
    }

    /* Bleach packet for reuse */
    memset(packet, 0, PACKET_SIZE);
    packet->packet_type = MSG_READ_FILE;

    inode = GetInode(inum)->inode;
    if (inode->reuse != reuse) {
        packet->arg1 = -1;
        return;
    }

    if (inode->type == INODE_FREE) {
        packet->arg1 = -2;
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

    if (DEBUG) {
        printf("start_index: %d\n", start_index);
        printf("end_index: %d\n", end_index);
        printf("inode_block_count: %d\n", inode_block_count);
    }

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
    int outer_index;

    /* Prefetch indirect block */
    if (inode->size >= MAX_DIRECT_SIZE) {
        indirect_block = GetBlock(inode->indirect)->block;
    }

    for (outer_index = start_index; outer_index <= end_index; outer_index++) {
        if (outer_index >= NUM_DIRECT) {
            block_id = indirect_block[outer_index - NUM_DIRECT];
        } else {
            block_id = inode->direct[outer_index];
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
        if (outer_index == end_index) {
            copysize = size - copied_size;
        }

        if (DEBUG) {
            printf("CopyTo - prefix: %d\n", prefix);
            printf("CopyTo - copied_size: %d\n", copied_size);
            printf("CopyTo - copysize: %d\n", copysize);
        }

        if (copysize > 0) {
            CopyTo(pid, buffer + copied_size, block + prefix, copysize);
            copied_size += copysize;
        }
    }

    if (DEBUG) printf("Final copied size: %d\n", copied_size);
    packet->arg1 = copied_size;
}

void WriteFile(DataPacket *packet, int pid) {
    int inum = packet->arg1;
    int pos = packet->arg2;
    int size = packet->arg3;
    int reuse = packet->arg4;
    void *buffer = packet->pointer;
    struct block_cache_entry *block_entry;
    struct inode_cache_entry *inode_entry;
    struct inode *inode;
    char *block;

    if (DEBUG) {
        printf("WriteFile - inum: %d\n", inum);
        printf("WriteFile - pos: %d\n", pos);
        printf("WriteFile - size: %d\n", size);
        printf("WriteFile - reuse: %d\n", reuse);
    }

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
        packet->arg1 = -2;
        return;
    }

    if (inode->reuse != reuse) {
        packet->arg1 = -3;
        return;
    }

    struct block_cache_entry *indirect_block_entry;
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
    int outer_index = start_index;
    if (outer_index < inode_block_count) outer_index = inode_block_count;

    if (DEBUG) {
        printf("outer_index: %d\n", outer_index);
        printf("start_index: %d\n", start_index);
        printf("end_index: %d\n", end_index);
        printf("inode_block_count: %d\n", inode_block_count);
    }

    /* Test compute number of additional blocks needed */
    int extra_blocks = 0;
    int i;

    for (i = start_index; i <= end_index; i++) {
        /* Increase the size if current index is less than or equal to block count */
        if (inode_block_count <= i) {
            /* If i is at NUM_DIRECT, it is about to create indirect */
            if (i == NUM_DIRECT) extra_blocks++;

            /* Assign new block if size increased AND is part of writing range */
            if (i >= start_index) extra_blocks++;
        }
    }

    if (free_block_list->size <= extra_blocks) {
        ((FilePacket *)packet)->inum = -4;
        return;
    }

    for (; outer_index <= end_index; outer_index++) {
        /* Increase the size if current index is less than or equal to block count */
        if (inode_block_count <= outer_index) {
            /* If i is at NUM_DIRECT, it is about to create indirect */
            if (outer_index == NUM_DIRECT) {
                inode->indirect = PopFromBuffer(free_block_list);
                indirect_block_entry = GetBlock(inode->indirect);
                indirect_block = indirect_block_entry->block;
                indirect_block_entry->dirty = 1;
                memset(indirect_block, 0, BLOCKSIZE);
            }

            /* Constantly get indirect block since it gets dirty many times */
            if (outer_index > NUM_DIRECT) {
                indirect_block_entry = GetBlock(inode->indirect);
                indirect_block = indirect_block_entry->block;
                indirect_block_entry->dirty = 1;
            }

            /* Assign new block if size increased AND is part of writing range */
            if (outer_index >= start_index) {
                if (outer_index >= NUM_DIRECT) {
                    /* Create new block in indirect block. */
                    indirect_block[outer_index - NUM_DIRECT] = PopFromBuffer(free_block_list);
                    if (DEBUG) printf("Create new block for indirect: %d (block: %d)\n", outer_index - NUM_DIRECT, indirect_block[outer_index - NUM_DIRECT]);
                    block = GetBlock(indirect_block[outer_index - NUM_DIRECT])->block;
                    memset(block, 0, BLOCKSIZE);
                } else {
                    /* Create new block at direct */
                    inode->direct[outer_index] = PopFromBuffer(free_block_list);
                    if (DEBUG) printf("Create new block at outer_index: %d (block: %d)\n", outer_index, inode->direct[outer_index]);
                    block = GetBlock(inode->direct[outer_index])->block;
                    memset(block, 0, BLOCKSIZE);
                    inode_entry->dirty = 1;
                    if (DEBUG) printf("inode->direct[outer_index]: %d\n", inode->direct[outer_index]);
                }
            }
        }
    }

    /* Start writing in the block */
    int block_id;
    int prefix = 0;
    int copied_size = 0; /* Total copied size */
    int copysize; /* size used for CopyFrom */

    /*
     * New file size based on the write operation.
     * If start writing from index 1, then block size is 512+.
     */
    int new_size = start_index * BLOCKSIZE;

    if (DEBUG) {
        printf("start_index: %d\n", start_index);
        printf("end_index: %d\n", end_index);
    }

    for (outer_index = start_index; outer_index <= end_index; outer_index++) {
        if (outer_index >= NUM_DIRECT) {
            block_id = indirect_block[outer_index - NUM_DIRECT];
        } else {
            block_id = inode->direct[outer_index];
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
        if (outer_index == start_index) {
            new_size += prefix;
        }

        /* Adjust size at last index */
        if (outer_index == end_index) {
            copysize = size - copied_size;
        }

        if (DEBUG) {
            printf("Copyfrom - prefix: %d\n", prefix);
            printf("Copyfrom - copied_size: %d\n", copied_size);
            printf("Copyfrom - copysize: %d\n", copysize);
        }

        CopyFrom(pid, block + prefix, buffer + copied_size, copysize);
        copied_size += copysize;
        block_entry->dirty = 1;
    }
    new_size += copied_size;
    if (DEBUG) {
        printf("Final copied size: %d\n", copied_size);
        printf("Old file size: %d\n", inode->size);
        printf("New file size: %d\n", new_size);
    }
    if (new_size > inode->size) {
        inode->size = new_size;
        inode_entry->dirty = 1;
    }
    packet->arg1 = copied_size;

    if (DEBUG) {
        printf("Priting inode %d after write file\n", inum);
        PrintInode(inode);
    }
}

void DeleteDir(DataPacket *packet) {
    struct inode_cache_entry *parent_entry;
    struct inode_cache_entry *target_entry;
    struct inode *parent_inode;
    struct inode *target_inode;
    int target_inum = packet->arg1;
    int parent_inum = packet->arg2;

    if (DEBUG) {
        printf("DeleteDir - target_inum: %d\n", target_inum);
        printf("DeleteDir - parent_inum: %d\n", parent_inum);
    }

    if (target_inum == ROOTINODE) {
        packet->arg1 = -1;
        return;
    }

    /* Bleach packet for reuse */
    memset(packet, 0, PACKET_SIZE);
    packet->packet_type = MSG_DELETE_DIR;

    parent_entry = GetInode(parent_inum);
    parent_inode = parent_entry->inode;

    /* Parent must be directory */
    if (parent_inode->type != INODE_DIRECTORY) {
        packet->arg1 = -2;
        return;
    }

    target_entry = GetInode(target_inum);
    target_inode = target_entry->inode;

    /* Cannot call deleteDir on non-directory */
    if (target_inode->type != INODE_DIRECTORY) {
        packet->arg1 = -3;
        return;
    }

    /* There should be . and .. left only */
    if (target_inode->size > DIRSIZE * 2) {
        packet->arg1 = -4;
        return;
    }

    /* Remove inum from parent inode */
    if (UnregisterDirectory(parent_inode, target_inum) < 0) {
        packet->arg1 = -5;
        return;
    }

    target_entry->dirty = 1;
    target_inode->type = INODE_FREE;
    target_inode->size = 0;
    target_inode->nlink = 0;

    /* Clean parent directory */
    parent_entry->dirty = CleanDirectory(parent_inode);

    struct block_cache_entry *block_entry;
    struct dir_entry *block;
    int i;

    block_entry = GetBlock(target_inode->direct[0]);
    block_entry->dirty = 1;
    block = block_entry->block;

    /* Need to decrement nlink of the parent */
    for (i = 0; i < 2; i++) {
        if (block[i].name[1] == '.') {
            target_entry = GetInode(block[i].inum);
            target_entry->inode->nlink -= 1;
            target_entry->dirty = 1;
        }
        block[i].inum = 0;
    }

    if (DEBUG) {
        printf("Parent inode after dir is deleted.\n");
        PrintInode(parent_inode);
    }
}

void CreateLink(DataPacket *packet, int pid) {
    struct inode_cache_entry *target_entry;
    struct inode_cache_entry *parent_entry;
    struct inode *target_inode;
    struct inode *parent_inode;

    char dirname[DIRNAMELEN];
    int target_inum = packet->arg1;
    int parent_inum = packet->arg2;
    void *target = packet->pointer;

    /* Bleach packet for reuse */
    memset(packet, 0, PACKET_SIZE);
    packet->packet_type = MSG_LINK;
    packet->arg1 = 0;

    if (CopyFrom(pid, dirname, target, DIRNAMELEN) < 0) {
        packet->arg1 = -1;
        return;
    }

    target_entry = GetInode(target_inum);
    target_inode = target_entry->inode;

    /* Already checked by client but just to be sure */
    if (target_inode->type != INODE_REGULAR) {
        packet->arg1 = -2;
        return;
    }

    parent_entry = GetInode(parent_inum);
    parent_inode = parent_entry->inode;

    /* Already checked by client but just to be sure */
    if (parent_inode->type != INODE_DIRECTORY) {
        packet->arg1 = -3;
        return;
    }

    /*
     * Creating a directory will require 1 block.
     * Adding it to parent inode may require 2 blocks.
     */
    if (free_block_list->size < 2) {
        packet->arg1 = -4;
        return;
    }

    parent_entry->dirty = RegisterDirectory(parent_inode, target_inum, dirname);
    target_inode->nlink += 1;
    target_entry->dirty = 1;

    if (DEBUG) {
        printf("Parent inode after link is created.");
        PrintInode(parent_inode);
    }
}

void DeleteLink(DataPacket *packet) {
    struct inode_cache_entry *parent_entry;
    struct inode_cache_entry *target_entry;
    struct inode *parent_inode;
    struct inode *target_inode;
    int target_inum = packet->arg1;
    int parent_inum = packet->arg2;

    /* Bleach packet for reuse */
    memset(packet, 0, PACKET_SIZE);
    packet->packet_type = MSG_UNLINK;
    packet->arg1 = 0;

    parent_entry = GetInode(parent_inum);
    parent_inode = parent_entry->inode;

    /* Parent must be directory */
    if (parent_inode->type != INODE_DIRECTORY) {
        packet->arg1 = -1;
        return;
    }

    target_entry = GetInode(target_inum);
    target_inode = target_entry->inode;

    /* Remove inum from parent inode */
    if (UnregisterDirectory(parent_inode, target_inum) < 0) {
        packet->arg1 = -2;
        return;
    }

    target_entry->dirty = 1;
    target_inode->nlink -= 1;

    /* Target Inode is linked no more */
    if (target_inode->nlink == 0) {
        TruncateFileInode(target_inum);
        if (DEBUG) printf("Deleting inode: %d\n", target_inum);
        target_inode->type = INODE_FREE;
        PushToBuffer(free_inode_list, target_inum);
    }

    /* Clean parent directory */
    parent_entry->dirty = CleanDirectory(parent_inode);

    if (DEBUG) {
        printf("Parent inode after link is deleted.\n");
        PrintInode(parent_inode);
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
    //PrintInodeCacheHashSet(inode_stack);
    //PrintInodeCacheStack(inode_stack);
    for (inode = inode_stack->top; inode != NULL; inode = inode->next_lru) {
        if (inode->dirty) {
            struct block_cache_entry* inode_block_entry = GetBlock((inode->inum / 8) + 1);
            if (DEBUG) printf("Syncing Inode %d to block %d\n",inode->inum,inode_block_entry->block_number);
            void* inode_block = inode_block_entry->block;
            inode_block_entry->dirty = 1;
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
            if (DEBUG) printf("Syncing Block %d\n",block->block_number);
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

    if (DEBUG) {
        printf("Printing root before starting server\n");
        PrintInode(GetInode(ROOTINODE)->inode);
    }

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
                if (DEBUG) printf("MSG_GET_FILE received from pid: %d\n", pid);
                GetFile(packet);
                break;
            case MSG_SEARCH_FILE:
                if (DEBUG) printf("MSG_SEARCH_FILE received from pid: %d\n", pid);
                SearchFile(packet, pid);
                break;
            case MSG_CREATE_FILE:
                if (DEBUG) printf("MSG_CREATE_FILE received from pid: %d\n", pid);
                CreateFile(packet, pid, INODE_REGULAR);
                break;
            case MSG_READ_FILE:
                if (DEBUG) printf("MSG_READ_FILE received from pid: %d\n", pid);
                ReadFile(packet, pid);
                break;
            case MSG_WRITE_FILE:
                if (DEBUG) printf("MSG_WRITE_FILE received from pid: %d\n", pid);
                WriteFile(packet, pid);
                break;
            case MSG_CREATE_DIR:
                if (DEBUG) printf("MSG_CREATE_DIR received from pid: %d\n", pid);
                CreateFile(packet, pid, INODE_DIRECTORY);
                break;
            case MSG_DELETE_DIR:
                if (DEBUG) printf("MSG_DELETE_DIR received from pid: %d\n", pid);
                DeleteDir(packet);
                break;
            case MSG_LINK:
                if (DEBUG) printf("MSG_LINK received from pid: %d\n", pid);
                CreateLink(packet, pid);
                break;
            case MSG_UNLINK:
                if (DEBUG) printf("MSG_UNLINK received from pid: %d\n", pid);
                DeleteLink(packet);
                break;
            case MSG_SYNC:
                if (DEBUG) printf("MSG_SYNC received from pid: %d\n", pid);
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
