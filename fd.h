typedef struct FileDescriptor {
    int inum; /* Inode number */
    int pos; /* Current position */
} FileDescriptor;

/*
 * Prepare new file descriptor using lowest available fd using file data.
 * Return -1 if open file table is all filled up.
 */
int CreateFileDescriptor(int inum);

/*
 * Close file descriptor. Return -1 if fd is invalid.
 */
int CloseFileDescriptor(int fd);

/*
 * Get file descriptor. Return NULL if fd is invalid.
 */
void *GetFileDescriptor(int fd);
