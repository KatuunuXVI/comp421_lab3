#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <comp421/yalnix.h>
#include <comp421/iolib.h>
#include <comp421/filesystem.h>
#include "path.h"
#include "packet.h"
#include "fd.h";

int current_inum = ROOTINODE;

/**
 * Creates and opens new file named pathname.
 */
int Create(char *pathname) {
    TracePrintf(10, "\t┌─ [Create] path: %s\n", pathname);

    /* Verify against pathname max length */
    if (strlen(pathname) > MAXPATHNAMELEN) return -1;

    /* Used for sending/receiving data */
    void *packet = malloc(PACKET_SIZE);

    /* Tokenize pathname */
    PathIterator *it = ParsePath(pathname);

    /* Last component of path */
    char filename[DIRNAMELEN];

    /* Inode from iteration */
    int next_inum = ROOTINODE;

    /* Parent inode for file */
    int parent_inum = current_inum;
    int last_it = 0;

    /* Request for file info for all components */
    ((DataPacket *)packet)->packet_type = MSG_GET_FILE;

    /* Iterate over tokenized pathname components */
    while (it->next != NULL) {
      /* We already know the inode for root */
      if (it->data[0] == '/') {
        next_inum = ROOTINODE;
        parent_inum = ROOTINODE;
      } else {
        ((DataPacket *)packet)->arg1 = current_inum;
        ((DataPacket *)packet)->pointer = (void *)it->data;
        Send(packet, -FILE_SERVER);
        parent_inum = next_inum;
        next_inum = ((FilePacket *)packet)->inum;
      }

      it = it->next;
      /* To confirm that it was last iteration when breaking out */
      if (it->next == NULL) {
        last_it = 1;
        /* Iterator will be freed. Keep filename here. */
        memcpy(filename, it->data, DIRNAMELEN);
      }

      /* Inum = 0 means file was not found */
      if (next_inum == 0) break;
    }
    DeletePathIterator(it);

    /* Target is not found and it isn't the last one */
    if (next_inum == 0 && last_it == 0) return -1;

    /* Target is found but is directory */
    if (next_inum != 0) {
        /* Target to override is directory */
        if (((FilePacket *)packet)->type == INODE_DIRECTORY) return ERROR;
    }

    /* Create new file or truncate existing file */
    memset(packet, 0, PACKET_SIZE);
    ((DataPacket *)packet)->packet_type = MSG_CREATE_FILE;
    ((DataPacket *)packet)->arg1 = parent_inum;
    ((DataPacket *)packet)->pointer = (void *)filename;
    Send(packet, -FILE_SERVER);

    /* If returned inum is 0, there is an error with file creation */
    if (((FilePacket *)packet)->inum == 0) return -1;
    int fd = CreateFileDescriptor(((FilePacket *)packet)->inum);
    if (fd < 0) {
        fprintf(stderr, "File Descriptor error\n");
        return -1;
    }

    TracePrintf(10, "\t└─ [Create fd: %d]\n\n", fd);
    return fd;
}

/**
 * Opens file 'pathname'
 */
int Open(char *pathname) {
    TracePrintf(10, "\t┌─ [Open] path: %s\n", pathname);

    /*
    // Verify against pathname max length

    // Create an iterator through tokenized pathname
    int target_inum;
    PathIterator *it = CreatePathIterator(pathname);
    while (it->data != NULL) {
        Send the following data to file server
        - Type: GetFile
        - Arg1: current directory inode
        - Arg2: next component (it->data)
        and expect the following data back
        - Type: GetFile
        - Data1: target_inum = inode if requested component exists, 0 if not

        it = it->next;
    }

    DeletePathIterator(it);

    // If target_inum is not found, return ERROR

    // If target_inum is found, return file descriptor
    */
    TracePrintf(10, "\t└─ [Open]\n\n");
    return 0;
}

/**
 * Closes file specified by fd
 */
int Close(int fd) {
    TracePrintf(10, "\t┌─ [Close] fd: %d\n", fd);
    // Throw error if fd is invalid number
    // Throw error if fd is not currently opened
    /*
    if (CloseFileDescriptor(fd) < 0) return ERROR;
    */
    TracePrintf(10, "\t└─ [Close]\n\n");
    return 0;
}

/**
 * Copies 'size' bytes from file 'fd' to '*buf'.
 */
int Read(int fd, void *buf, int size) {
    TracePrintf(10, "\t┌─ [Read] fd: %d\n", fd);
    printf("Size: %d\n",size);
    printf("Address: %p\n",buf);
    /*
    // Throw error if fd is invalid number
    // Throw error if fd is not opened

    Send the following data to file server
    - Type: ReadFile
    - arg1: inode (fd's inode)
    - arg2: offset (fd's current position)
    - arg3: size
    - arg4: buf pointer
    - Expect file server to call CopyTo
    Expect the following result
    - data1: read bytes, -1 if error
    */
    TracePrintf(10, "\t└─ [Read]\n\n");
    return 0;
}

/**
 * Copies 'size' bytes from '*buf' to file 'fd'.
 */
int Write(int fd, void *buf, int size) {
    TracePrintf(10, "\t┌─ [Write] fd: %d\n", fd);
    printf("Size: %d\n", size);
    printf("Address: %p\n", buf);
    /*
    // Throw error if fd is invalid number
    // Throw error if fd is not opened

    Send the following data to file server
    - Type: WriteFile
    - arg1: inode (fd's inode)
    - arg2: offset (fd's current position)
    - arg3: size
    - arg4: buf pointer
    - Expect file server to call CopyFrom
    - Seek can travel beyond end of file to create hole.
    Expect the following result
    - data1: written bytes, -1 if error
    */
    TracePrintf(10, "\t└─ [Write]\n\n");
    return 0;
}

/**
 * Changes position of the open file
 */
int Seek(int fd, int offset, int whence) {
    TracePrintf(10, "\t┌─ [Seek] fd: %d\n", fd);
    printf("Offset: %d\n", offset);
    printf("Whence: %d\n", whence);
    /*
    // Throw error if fd is invalid number
    // Throw error if fd is not opened
    // This call may not required file server
    // if local fd remembers all file metadata
    */
    TracePrintf(10, "\t└─ [Seek]\n\n");
    return 0;
}

/**
 * Creates hard link from 'newname' to 'oldname'
 */
int Link(char *oldname, char *newname) {
    printf("Old Name: %s\n", oldname);
    printf("New Name: %s\n", newname);
    return 0;
}

/**
 * Removes directory entry for 'pathname'
 */
int Unlink(char *pathname) {
    printf("Pathname: %s\n", pathname);
    return 0;
}

/**
 * Makes new directory at 'pathname'
 */
int MkDir(char *pathname) {
    TracePrintf(10, "\t┌─ [MkDir] path: %s\n", pathname);
    /*
    // Verify against pathname max length

    // Iterate through tokenized pathname

    // If target_inum is found return ERROR

    // If target_inum is not found, return ERROR if not found target_inum is not last component

    // Create new directory
    Send the following data to file server
    - Type: CreateDir
    - Arg1: dir_name
    - Arg2: parent_inum
    */

    TracePrintf(10, "\t└─ [MkDir]\n\n");
    return 0;
}

/**
 * Deletes the existing directory at 'pathname'
 */
int RmDir(char *pathname) {
    TracePrintf(10, "\t┌─ [RmDir] path: %s\n", pathname);
    /*
    // Verify against pathname max length

    // Create an iterator through tokenized pathname

    // Return ERROR if target_inum not found

    // Request server to delete directory
    */
    TracePrintf(10, "\t└─ [RmDir]\n\n");
    return 0;
}

int ChDir(char *pathname) {
    TracePrintf(10, "\t┌─ [ChDir] path: %s\n", pathname);
    /*
    // Verify against pathname max length

    // Create an iterator through tokenized pathname

    // Return ERROR if target_inum not found

    // Update current directory inode
    */
    TracePrintf(10, "\t└─ [ChDir]\n\n");
    return 0;
}

/**
 * Writes to 'statbuf'
 */
int Stat(char *pathname, struct Stat *statbuf) {
    TracePrintf(10, "\t┌─ [Stat] path: %s\n", pathname);
    /*
    // Verify against pathname max length

    // Create an iterator through tokenized pathname

    // Return ERROR if target_inum is not found

    // Get file stat from either GetFile or GetFileStat
    - Data1: inum
    - Data2: type
    - Data3: size
    - Data4: nlink
    */
    statbuf->inum = 0;
    statbuf->type = 0;
    statbuf->size = 0;
    statbuf->nlink = 0;
    TracePrintf(10, "\t└─ [Stat]\n\n");
    return 0;
}

/**
 * Writes dirty caches to the disk so they are not lost
 */
int Sync(void) {
    return 0;
}

/**
 * Syncs cache and closes the library
 */
int Shutdown(void) {
    return 0;
}
