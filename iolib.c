#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <comp421/yalnix.h>
#include <comp421/iolib.h>
#include <comp421/filesystem.h>
#include "path.h"
#include "packet.h"
#include "fd.h"

int current_inum = ROOTINODE;

/* Return -1 if pathname is invalid */
int AssertPathname(char *pathname) {
    /* Verify against null pointer */
    if (pathname == NULL) return -1;

    /* Verify against pathname max length */
    if (strlen(pathname) > MAXPATHNAMELEN) return -1;

    return 0;
}

/*
 * Helper for Iterating over pathname via file server
 * - pathname: pathname to iterate
 * - parent_inum: inum of second last component only if return value is 0 or -1
 * - stat (output): file stat of last component only if return value is 0 (optional)
 * - filename (output): dirname of last component only if return value is 0 or -1 (optional)
 * - reuse (output): reuse count of last component only if return value is 0 (optional)
 * Return 0 if all components are found
 * Return -1 if all but last component is found
 * Else return -2
 */
int IterateFilePath(char *pathname, int *parent_inum, struct Stat *stat, char *filename, int *reuse) {
    /* Used for sending/receiving data */
    void *packet = malloc(PACKET_SIZE);
    ((DataPacket *)packet)->packet_type = MSG_SEARCH_FILE;

    /* Tokenize pathname */
    PathIterator *it = ParsePath(pathname);

    /* Inode from iteration */
    int next_inum = current_inum;
    char *data;

    /* By default, parent_inum is current_inum */
    *parent_inum = current_inum;

    /* Iterate over tokenized pathname components */
    while (it->next != NULL) {
        data = it->data;
        /* We already know the inode for root */
        if (data[0] == '/') {
            next_inum = ROOTINODE;
            *parent_inum = ROOTINODE;
        } else {
            ((DataPacket *)packet)->arg1 = next_inum;
            ((DataPacket *)packet)->pointer = (void *)data;
            Send(packet, -FILE_SERVER);
            *parent_inum = next_inum;
            next_inum = ((FilePacket *)packet)->inum;
        }

        /*
         * Incrementing to next iterator early
         * will tolerate last component not found.
         */
        it = it->next;

        /* Inum = 0 means file was not found */
        if (next_inum == 0) break;
    }

    /*
     * Iterator reached the end.
     * This means all paths are found at least until last component.
     */
    int last_not_found = 0;
    if (it->next == NULL) {
        if (filename) memcpy(filename, data, DIRNAMELEN);
        if (next_inum == 0) last_not_found = 1;
    }

    if (next_inum != 0) {
        if (stat) {
            stat->inum = ((FilePacket *)packet)->inum;
            stat->type = ((FilePacket *)packet)->type;
            stat->size = ((FilePacket *)packet)->size;
            stat->nlink = ((FilePacket *)packet)->nlink;
        }

        if (reuse) {
            *reuse = ((FilePacket *)packet)->reuse;
        }
    }

    DeletePathIterator(it);
    free(packet);

    if (next_inum != 0) return 0;
    if (last_not_found != 0) return -1;
    return -2;
}

/**
 * Creates and opens new file named pathname.
 */
int Create(char *pathname) {
    TracePrintf(10, "\t┌─ [Create] path: %s\n", pathname);

    /* Verify pathname */
    if (AssertPathname(pathname) < 0) {
        fprintf(stderr, "[Error] Invalid pathname\n");
        return -1;
    }

    /* Create file descriptor first */
    FileDescriptor *fd = CreateFileDescriptor();
    if (fd == NULL) {
        fprintf(stderr, "[Error] File Descriptors are all used. Try closing others.\n");
        return -1;
    }

    /* Iterate over all components */
    char filename[DIRNAMELEN];
    int *parent_inum = malloc(sizeof(int));
    struct Stat *stat = malloc(sizeof(struct Stat));
    int result = IterateFilePath(pathname, parent_inum, stat, filename, NULL);

    /* Path was not found */
    if (result == -2) {
        fprintf(stderr, "[Error] Path not found\n");
        free(parent_inum);
        free(stat);
        CloseFileDescriptor(fd->id);
        return -1;
    }

    /* Target found but it is directory */
    if (result == 0 && stat->type == INODE_DIRECTORY) {
        fprintf(stderr, "[Error] Cannot overwrite directory\n");
        free(parent_inum);
        free(stat);
        CloseFileDescriptor(fd->id);
        return -1;
    }

    /* Create new file or truncate existing file */
    void *packet = malloc(PACKET_SIZE);
    ((DataPacket *)packet)->packet_type = MSG_CREATE_FILE;
    ((DataPacket *)packet)->arg1 = *parent_inum;
    ((DataPacket *)packet)->pointer = (void *)filename;
    Send(packet, -FILE_SERVER);

    /* If returned inum is 0, there is an error with file creation */
    int new_inum = ((FilePacket *)packet)->inum;
    if (new_inum <= 0) {
        if (new_inum == 0) fprintf(stderr, "[Error] File creation error\n");
        else if (new_inum == -1) fprintf(stderr, "[Error] Cannot create file in non-directory.\n");
        else if (new_inum == -2) fprintf(stderr, "[Error] Directory has reached max size limit.\n");
        else if (new_inum == -3) fprintf(stderr, "[Error] Not enough inode left.\n");
        else if (new_inum == -4) fprintf(stderr, "[Error] Not enough block left.\n");

        free(packet);
        free(parent_inum);
        free(stat);
        CloseFileDescriptor(fd->id);
        return -1;
    }

    fd->inum = new_inum;
    fd->reuse = ((FilePacket *)packet)->reuse;
    fd->pos = 0;

    free(packet);
    free(parent_inum);
    free(stat);
    TracePrintf(10, "\t└─ [Create fd: %d]\n\n", fd->id);
    return fd->id;
}

/**
 * Opens file 'pathname'
 */
int Open(char *pathname) {
    TracePrintf(10, "\t┌─ [Open] path: %s\n", pathname);

    /* Verify pathname */
    if (AssertPathname(pathname) < 0) {
        fprintf(stderr, "[Error] Invalid pathname\n");
        return -1;
    }

    /* Create file descriptor first */
    FileDescriptor *fd = CreateFileDescriptor();
    if (fd == NULL) {
        fprintf(stderr, "[Error] File Descriptors are all used. Try closing others.\n");
        return -1;
    }

    /* Iterate over all components */
    int *parent_inum = malloc(sizeof(int));
    struct Stat *stat = malloc(sizeof(struct Stat));
    int result = IterateFilePath(pathname, parent_inum, stat, NULL, &fd->reuse);

    /* Path was not found */
    if (result < 0) {
        fprintf(stderr, "[Error] Path not found\n");
        free(parent_inum);
        free(stat);
        CloseFileDescriptor(fd->id);
        return -1;
    }

    fd->inum = stat->inum;
    fd->pos = 0;

    free(parent_inum);
    free(stat);
    TracePrintf(10, "\t└─ [Open fd_id: %d]\n\n", fd->id);
    return fd->id;
}

/**
 * Closes file specified by fd
 */
int Close(int fd_id) {
    TracePrintf(10, "\t┌─ [Close] fd_id: %d\n", fd_id);

    /* Throw error if fd is not currently opened */
    if (CloseFileDescriptor(fd_id) < 0) {
        fprintf(stderr, "[Error] Provided fd is not open.\n");
        return -1;
    }

    TracePrintf(10, "\t└─ [Close]\n\n");
    return 0;
}

/**
 * Copies 'size' bytes from file 'fd' to '*buf'.
 */
int Read(int fd_id, void *buf, int size) {
    TracePrintf(10, "\t┌─ [Read] fd_id: %d\n", fd_id);
    /* Throw error if fd is invalid number */
    if (buf == NULL || size < 0) {
        fprintf(stderr, "[Error] Invalid arguments on buffer or size.\n");
        return -1;
    }

    /* Throw error if fd is invalid number */
    FileDescriptor *fd = GetFileDescriptor(fd_id);
    if (fd == NULL) {
        fprintf(stderr, "[Error] Provided fd is not open.\n");
        return -1;
    }

    int result;
    DataPacket *packet = malloc(PACKET_SIZE);
    memset(packet, 0, PACKET_SIZE);
    packet->packet_type = MSG_READ_FILE;
    packet->arg1 = fd->inum;
    packet->arg2 = fd->pos;
    packet->arg3 = size;
    packet->arg4 = fd->reuse;
    packet->pointer = (void *)buf;
    Send(packet, -FILE_SERVER);

    result = packet->arg1;
    free(packet);

    if (result < 0) {
        if (result == -1) fprintf(stderr, "[Error] Reuse count has changed. Please close this fd.\n");
        else if (result == -2) fprintf(stderr, "[Error] This file is freed.\n");
        return -1;
    }

    fd->pos += result;
    TracePrintf(10, "\t└─ [Read size: %d]\n\n", result);
    return result;
}

/**
 * Copies 'size' bytes from '*buf' to file 'fd'.
 */
int Write(int fd_id, void *buf, int size) {
    TracePrintf(10, "\t┌─ [Write] fd_id: %d\n", fd_id);
    /* Throw error if fd is invalid number */
    if (buf == NULL || size < 0) {
        fprintf(stderr, "[Error] Invalid arguments on buffer or size.\n");
        return -1;
    }

    /* Throw error if fd is not opened */
    FileDescriptor *fd = GetFileDescriptor(fd_id);
    if (fd == NULL) {
        fprintf(stderr, "[Error] Provided fd is not open.\n");
        return -1;
    }

    int result;
    DataPacket *packet = malloc(PACKET_SIZE);
    memset(packet, 0, PACKET_SIZE);
    packet->packet_type = MSG_WRITE_FILE;
    packet->arg1 = fd->inum;
    packet->arg2 = fd->pos;
    packet->arg3 = size;
    packet->arg4 = fd->reuse;
    packet->pointer = (void *)buf;
    Send(packet, -FILE_SERVER);
    result = packet->arg1;
    free(packet);

    if (result < 0) {
        if (result == -1) fprintf(stderr, "[Error] Trying to write beyond max file size.\n");
        else if (result == -2) fprintf(stderr, "[Error] Trying to write to non-regular file.\n");
        else if (result == -3) fprintf(stderr, "[Error] Reuse count has changed. Please close this fd.\n");
        else if (result == -4) fprintf(stderr, "[Error] Not enough block left.\n");
        return -1;
    }
    fd->pos += result;

    TracePrintf(10, "\t└─ [Write size: %d]\n\n", result);
    return result;
}

/**
 * Changes position of the open file
 */
int Seek(int fd_id, int offset, int whence) {
    TracePrintf(10, "\t┌─ [Seek] fd_id: %d\n", fd_id);
    /* Throw error if fd is not opened */
    FileDescriptor *fd = GetFileDescriptor(fd_id);
    if (fd == NULL) {
        fprintf(stderr, "[Error] Provided fd is not open.\n");
        return -1;
    }

    /* Fetch file data using inum saved in fd */
    FilePacket *packet = malloc(PACKET_SIZE);
    memset(packet, 0, PACKET_SIZE);
    packet->packet_type = MSG_GET_FILE;
    packet->inum = fd->inum;
    Send(packet, -FILE_SERVER);

    int size = packet->size;
    int reuse = packet->reuse;
    free(packet);

    if (fd->reuse != reuse) {
        fprintf(stderr, "[Error] Reuse count has changed. Please close this fd.\n");
        free(packet);
        return -1;
    }

    int new_pos;
    switch (whence) {
        case SEEK_SET:
            new_pos = offset;
            break;
        case SEEK_CUR:
            new_pos = fd->pos + offset;
            break;
        case SEEK_END:
            new_pos = size + offset;
            break;
        default:
            fprintf(stderr, "[Error] Invalid whence provided.\n");
            return -1;
    }

    if (new_pos < 0) {
        fprintf(stderr, "[Error] Updated position cannot be negative.\n");
        return -1;
    }

    fd->pos = new_pos;

    TracePrintf(10, "\t└─ [Seek]\n\n");
    return new_pos;
}

/**
 * Creates hard link from 'newname' to 'oldname'
 */
int Link(char *oldname, char *newname) {
    TracePrintf(10, "\t┌─ [Link]\n");

    /* Verify pathname */
    if (AssertPathname(oldname) < 0) {
        fprintf(stderr, "[Error] Invalid old pathname\n");
        return -1;
    }

    if (AssertPathname(newname) < 0) {
        fprintf(stderr, "[Error] Invalid new pathname\n");
        return -1;
    }

    /* Iterate over both components */
    char new_filename[DIRNAMELEN];
    int *old_parent_inum = malloc(sizeof(int));
    int *new_parent_inum = malloc(sizeof(int));
    struct Stat *old_stat = malloc(sizeof(struct Stat));
    int result1 = IterateFilePath(oldname, old_parent_inum, old_stat, NULL, NULL);
    int result2 = IterateFilePath(newname, new_parent_inum, NULL, new_filename, NULL);

    /* Oldname was not found */
    if (result1 < 0) {
        fprintf(stderr, "[Error] Path %s not found\n", oldname);
        free(old_parent_inum);
        free(new_parent_inum);
        free(old_stat);
        return -1;
    }

    /* Oldname cannot be directory */
    if (old_stat->type != INODE_REGULAR) {
        fprintf(stderr, "[Error] You can only create hard link on regular file.\n");
        free(old_parent_inum);
        free(new_parent_inum);
        free(old_stat);
        return -1;
    }

    /* Newname path was not found */
    if (result2 == -2) {
        fprintf(stderr, "[Error] Path %s not found.\n", newname);
        free(old_parent_inum);
        free(new_parent_inum);
        free(old_stat);
        return -1;
    }

    /* Newname file already exists */
    if (result2 == 0) {
        fprintf(stderr, "[Error] File %s already exists.\n", newname);
        free(old_parent_inum);
        free(new_parent_inum);
        free(old_stat);
        return -1;
    }

    DataPacket *packet = malloc(PACKET_SIZE);
    packet->packet_type = MSG_LINK;
    packet->arg1 = old_stat->inum;
    packet->arg2 = *new_parent_inum;
    packet->pointer = (void *)new_filename;
    Send(packet, -FILE_SERVER);
    int result = packet->arg1;

    free(old_parent_inum);
    free(new_parent_inum);
    free(old_stat);
    free(packet);

    if (result < 0) {
        if (result == -1) fprintf(stderr, "[Error] Unexpected CopyFrom error.\n");
        else if (result == -2) fprintf(stderr, "[Error] You can only create hard link on regular file.\n");
        else if (result == -3) fprintf(stderr, "[Error] Parent of new path is not a directory.\n");
        else if (result == -4) fprintf(stderr, "[Error] Not enough block left.\n");
        return -1;
    }

    TracePrintf(10, "\t└─ [Link]\n\n");
    return 0;
}

/**
 * Removes directory entry for 'pathname'
 */
int Unlink(char *pathname) {
    TracePrintf(10, "\t┌─ [Unlink] pathname: %s\n", pathname);

    /* Verify pathname */
    if (AssertPathname(pathname) < 0) return -1;

    int *parent_inum = malloc(sizeof(int));
    struct Stat *stat = malloc(sizeof(struct Stat));
    int result = IterateFilePath(pathname, parent_inum, stat, NULL, NULL);

    /* Path was not found */
    if (result < 0) {
        fprintf(stderr, "[Error] Path not found\n");
        free(parent_inum);
        free(stat);
        return -1;
    }

    /* Target found but it is directory */
    if (stat->type == INODE_DIRECTORY) {
        fprintf(stderr, "[Error] Cannot unlink directory\n");
        free(parent_inum);
        free(stat);
        return -1;
    }

    DataPacket *packet = malloc(PACKET_SIZE);
    packet->packet_type = MSG_UNLINK;
    packet->arg1 = stat->inum;
    packet->arg2 = *parent_inum;
    Send(packet, -FILE_SERVER);
    result = packet->arg1;
    free(parent_inum);
    free(stat);
    free(packet);

    if (result < 0) {
        fprintf(stderr, "[Error] Unlink error.\n");
        return -1;
    }

    TracePrintf(10, "\t└─ [Unlink]\n\n");
    return 0;
}

int SymLink(char *oldname, char *newname) {
    return -1;
}

int ReadLink(char *pathname, char *buf, int len) {
    return -1;
}

/**
 * Makes new directory at 'pathname'
 */
int MkDir(char *pathname) {
    TracePrintf(10, "\t┌─ [MkDir] path: %s\n", pathname);

    /* Verify pathname */
    if (AssertPathname(pathname) < 0) return -1;

    /* Iterate over all components */
    char filename[DIRNAMELEN];
    int *parent_inum = malloc(sizeof(int));
    struct Stat *stat = malloc(sizeof(struct Stat));
    int result = IterateFilePath(pathname, parent_inum, stat, filename, NULL);

    /* If target_inum is found, return ERROR */
    if (result == 0) {
        fprintf(stderr, "[Error] File already exist\n");
        free(parent_inum);
        free(stat);
        return -1;
    }

    /* Target is not found and it isn't the last one */
    if (result == -2) {
        fprintf(stderr, "[Error] Path not found\n");
        free(parent_inum);
        free(stat);
        return -1;
    }

    /* Create new directory */
    int new_inum;
    void *packet = malloc(PACKET_SIZE);
    ((DataPacket *)packet)->packet_type = MSG_CREATE_DIR;
    ((DataPacket *)packet)->arg1 = *parent_inum;
    ((DataPacket *)packet)->pointer = (void *)filename;
    Send(packet, -FILE_SERVER);
    new_inum = ((FilePacket *)packet)->inum;

    free(packet);
    free(parent_inum);
    free(stat);

    /* If returned inum is 0, there is an error with file creation */
    if (new_inum == 0) {
        fprintf(stderr, "[Error] File creation error\n");
        return -1;
    }

    TracePrintf(10, "\t└─ [MkDir]\n\n");
    return 0;
}

/**
 * Deletes the existing directory at 'pathname'
 */
int RmDir(char *pathname) {
    TracePrintf(10, "\t┌─ [RmDir] path: %s\n", pathname);

    /* Verify pathname */
    if (AssertPathname(pathname) < 0) return -1;

    /* Iterate over all components */
    char filename[DIRNAMELEN];
    int *parent_inum = malloc(sizeof(int));
    struct Stat *stat = malloc(sizeof(struct Stat));
    int result = IterateFilePath(pathname, parent_inum, stat, filename, NULL);

    if (filename[0] == '.' && filename[1] == '\0') {
        fprintf(stderr, "[Error] Cannot RmDir .\n");
        free(parent_inum);
        free(stat);
        return -1;
    }

    if (filename[0] == '.' && filename[1] == '.' && filename[2] == '\0') {
        fprintf(stderr, "[Error] Cannot RmDir ..\n");
        free(parent_inum);
        free(stat);
        return -1;
    }

    /* Target is not found */
    if (result < 0) {
        fprintf(stderr, "[Error] Path not found\n");
        free(parent_inum);
        free(stat);
        return -1;
    }

    /* Delete directory */
    DataPacket *packet = malloc(PACKET_SIZE);
    packet->packet_type = MSG_DELETE_DIR;
    packet->arg1 = stat->inum;
    packet->arg2 = *parent_inum;
    Send(packet, -FILE_SERVER);
    result = packet->arg1;

    free(packet);
    free(parent_inum);
    free(stat);

    if (result < 0) {
        if (result == -1) fprintf(stderr, "[Error] Cannot delete root directory.\n");
        else if (result == -2) fprintf(stderr, "[Error] Parent is not a directory.\n");
        else if (result == -3) fprintf(stderr, "[Error] Cannot call RmDir on regular file.\n");
        else if (result == -4) fprintf(stderr, "[Error] There are other directories left in this directory.\n");
        return -1;
    }

    TracePrintf(10, "\t└─ [RmDir]\n\n");
    return 0;
}

int ChDir(char *pathname) {
    TracePrintf(10, "\t┌─ [ChDir] path: %s\n", pathname);

    /* Verify pathname */
    if (AssertPathname(pathname) < 0) return -1;

    /* Iterate over all components */
    int *parent_inum = malloc(sizeof(int));
    struct Stat *stat = malloc(sizeof(struct Stat));
    int result = IterateFilePath(pathname, parent_inum, stat, NULL, NULL);

    /* Path was not found */
    if (result < 0) {
        fprintf(stderr, "[Error] Path not found\n");
        free(parent_inum);
        free(stat);
        return -1;
    }

    if (stat->type != INODE_DIRECTORY) {
        fprintf(stderr, "[Error] Not directory\n");
        free(parent_inum);
        free(stat);
        return -1;
    }

    current_inum = stat->inum;
    free(parent_inum);
    free(stat);
    TracePrintf(10, "\t└─ [ChDir]\n\n");
    return 0;
}

/**
 * Writes to 'statbuf'
 */
int Stat(char *pathname, struct Stat *statbuf) {
    TracePrintf(10, "\t┌─ [Stat] path: %s\n", pathname);

    /* Verify pathname */
    if (AssertPathname(pathname) < 0) return -1;

    /* Iterate over all components */
    int *parent_inum = malloc(sizeof(int));
    int result = IterateFilePath(pathname, parent_inum, statbuf, NULL, NULL);

    /* Path was not found */
    if (result < 0) {
        fprintf(stderr, "[Error] Path not found\n");
        free(parent_inum);
        return -1;
    }

    TracePrintf(10, "\t└─ [Stat]\n\n");
    return 0;
}

/**
 * Writes dirty caches to the disk so they are not lost
 */
int Sync() {
    /* Create new file or truncate existing file */
    void *packet = malloc(PACKET_SIZE);
    ((DataPacket *)packet)->packet_type = MSG_SYNC;
    ((DataPacket *)packet)->arg1 = 0; /* Don't shut down */
    Send(packet, -FILE_SERVER);
    free(packet);
    return 0;
}

/**
 * Syncs cache and closes the library
 */
int Shutdown() {
    /* Create new file or truncate existing file */
    int success;
    void *packet = malloc(PACKET_SIZE);
    ((DataPacket *)packet)->packet_type = MSG_SYNC;
    ((DataPacket *)packet)->arg1 = 1; /* Shut down */
    Send(packet, -FILE_SERVER);
    free(packet);
    return 0;
}
