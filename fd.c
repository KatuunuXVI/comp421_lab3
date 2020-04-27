#include <stddef.h>
#include <comp421/filesystem.h>
#include "fd.h"

FileDescriptor open_file_table[MAX_OPEN_FILES];
int initialized = 0;

/*
 * Private helper for initializing open file table
 */
void IntializeOpenFileTable() {
    int i;
    if (initialized == 1) return;
    for (i = 0; i < MAX_OPEN_FILES; i++) {
        open_file_table[0].inum = 0;
        open_file_table[0].pos = 0;
    }
    initialized = 1;
}

/*
 * Prepare new file descriptor using lowest available fd using file data.
 * Return -1 if open file table is all filled up.
 */
int CreateFileDescriptor(int inum) {
    int i;
    if (initialized == 0) IntializeOpenFileTable();
    for (i = 0; i < MAX_OPEN_FILES; i++) {
        if (open_file_table[i].inum == 0) {
            open_file_table[i].inum = inum;
            break;
        }
    }
    if (i >= MAX_OPEN_FILES) return -1;
    return i;
}

/*
 * Close file descriptor. Return -1 if fd is invalid.
 */
int CloseFileDescriptor(int fd) {
    if (initialized == 0) return -1;
    if (fd > MAX_OPEN_FILES || fd < 0) return -1;
    if (open_file_table[fd].inum != 0) {
        open_file_table[fd].inum = 0;
        return 0;
    }
    return -1;
}

/*
 * Get file descriptor. Return NULL if fd is invalid.
 */
void *GetFileDescriptor(int fd) {
    if (initialized == 0) return NULL;
    if (fd > MAX_OPEN_FILES || fd < 0) return NULL;
    if (open_file_table[fd].inum == 0) return NULL;
    return &open_file_table[fd];
}
