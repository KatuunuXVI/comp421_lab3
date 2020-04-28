#include <stdio.h>
#include <comp421/iolib.h>
#include "path.h"

/*
 * Given pathname, create a linked list of parsed components
 */
// PathIterator *CreatePathIterator(char *pathname);

/*
 * Free Path Iterator.
 * Should be able to delete all iterators even if
 * any step of iterator is provided.
 */
// int DeletePathIterator(PathIterator *it);

/*
 * Prepare new file descriptor using lowest available fd
 * using file data. Return -1 if open file table is all filled up.
 */
// int CreateFileDescriptor();

/*
 * Close file descriptor. Return -1 if fd is invalid.
 */
// int CloseFileDescriptor(int fd);

/*
 * Get file descriptor. Return NULL if fd is invalid.
 */
// FileDescriptor GetFileDescriptor(int fd);

/*
 * Get current directory inode
 */
// int GetCurrentDirectory(int inum);

/*
 * Set current directory inode
 */
// int SetCurrentDirectory(int inum);
// Or just have a global variable here

/**
 * Creates and opens new file named pathname.
 */
int Create(char *pathname) {
    TracePrintf(10, "\t┌─ [Create] path: %s\n", pathname);
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
        - Break if not found

        it = it->next;
    }

    DeletePathIterator(it);

    // If target_inum is found
    // Return ERROR if existing file is directory
    // Request Truncate to file server
    Send the following data to file server
    - Type: TruncateFile
    - Arg1: inode
    and expect the following data back
    - Data1: Success/Fail

    // return with file descriptor

    // If target_inum is not found
    // Return ERROR if not found target_inum is not last component of path
    // Create New File
    Send the following data to file server
    - Type: CreateFile
    - Arg1: filename
    - Arg2: parent_inum
    // return with file descriptor
    */

    TracePrintf(10, "\t└─ [Create]\n\n");
    return 0;
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
