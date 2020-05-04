typedef struct FileDescriptor {
    int id; /* FD id */
    int used; /* Only valid if used = 1 */
    int inum; /* Inode number */
    int reuse; /* Same inode with different reuse means file is changed */
    int pos; /* Current position */
} FileDescriptor;

/*
 * Prepare new file descriptor using lowest available fd using file data.
 * Return NULL if open file table is all filled up.
 */
FileDescriptor *CreateFileDescriptor();

/*
 * Close file descriptor. Return -1 if fd is invalid.
 */
int CloseFileDescriptor(int fd);

/*
 * Get file descriptor. Return NULL if fd is invalid.
 */
FileDescriptor *GetFileDescriptor(int fd);
