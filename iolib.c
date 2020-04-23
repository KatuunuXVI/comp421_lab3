#include <stdio.h>
#include <comp421/iolib.h>

int Create(char *pathname) {
    printf("Pathname: %s\n",pathname);
    return 0;
}

/**
 * Opens file 'pathname'
 */
int Open(char *pathname) {
    printf("Pathname: %s\n",pathname);
    return 0;
}

/**
 * Closes file specified by fd
 */
int Close(int fd) {
    printf("File Descriptor: %d\n",fd);
    return 0;
}



/**
 * Copies 'size' bytes from file 'fd' to '*buf'.
 */
int Read(int fd, void *buf, int size) {
    printf("File Descriptor: %d\n",fd);
    printf("Size: %d\n",size);
    printf("Address: %p\n",buf);
    return 0;
}

/**
 * Copies 'size' bytes from '*buf' to file 'fd'.
 */
int Write(int fd, void *buf, int size) {
    printf("File Descriptor: %d\n",fd);
    printf("Size: %d\n",size);
    printf("Address: %p\n",buf);
    return 0;
}

/**
 * Changes position of the open file
 */
int Seek(int fd, int offset, int whence) {
    printf("File Descriptor: %d\n",fd);
    printf("Offset: %d\n",offset);
    printf("Whence: %d\n",whence);
    return 0;
}

/**
 * Creates hard link from 'newname' to 'oldname'
 */
int Link(char *oldname, char *newname) {
    printf("Old Name: %s\n",oldname);
    printf("New Name: %s\n",newname);
    return 0;
}

/**
 * Removes directory entry for 'pathname'
 */
int Unlink(char *pathname) {
    printf("Pathname: %s\n",pathname);
    return 0;
}

/**
 * Makes new directory at 'pathname'
 */
int MkDir(char *pathname) {
    printf("Pathname: %s\n",pathname);
    return 0;
}

/**
 * Deletes the existing directory at 'pathname'
 */
int RmDir(char *pathname) {
    printf("Pathname: %s\n",pathname);
    return 0;
}

int ChDir(char *pathname) {
    printf("Pathname: %s\n",pathname);
    return 0;
}

/**
 * Writes to 'statbuf'
 */
int Stat(char *pathname, struct Stat *statbuf) {
    printf("Pathname: %s\n",pathname);
    printf("Stat Address: %p\n", statbuf);
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