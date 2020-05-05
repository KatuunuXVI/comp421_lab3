/*
 * Automated test 1
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <comp421/iolib.h>

#define ERROR    -1
#define SUCCESS  0


/* Helper Functions */
int MkDir_Wrapper(char *pathname);
int RmDir_Wrapper(char *pathname);
int ChDir_Wrapper(char *pathname);
int Create_Wrapper(char *pathname);
int Read_Wrapper(int fd, void *buf, int size);
int Write_Wrapper(int fd, void *buf, int size);
int Close_Wrapper(int fd);
int Sync_Wrapper();
int Open_Wrapper(char *pathname);
int Link_Wrapper(char *oldname, char *newname);
int Unlink_Wrapper(char *pathname);
int Seek_Wrapper(int fd, int offset, int whence);


int main() {
    int i;
    int status;
    int bytes;
    int fd_0, fd_1, fd_2, fd_3, fd_4;
    char *content_1024;
    char *buf;

    content_1024 = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Morbi mollis ultricies ligula suscipit molestie. Maecenas non lacinia quam. Sed posuere imperdiet tempus. Nam dignissim magna luctus ipsum dictum lacinia. Duis porttitor sagittis eros, et tincidunt purus mollis in. Praesent a posuere est. Quisque blandit augue eget odio convallis euismod. Sed quis lacus nulla. Maecenas id erat ex. Orci varius natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Curabitur quis dolor nec elit elementum imperdiet. Duis enim ante, imperdiet non tincidunt eget, dictum ut odio. Aliquam vulputate ultricies molestie. Donec scelerisque felis non arcu feugiat, sit amet consectetur mi scelerisque. Aenean orci eros, interdum pretium felis id, vulputate dignissim quam. Etiam ultrices egestas dolor ut cursus. Etiam rutrum nibh non sem sodales rhoncus. Sed tortor libero, venenatis blandit tempor id, vehicula a nisl. Sed nisl nunc, lacinia at ligula sit amet, congue elementum tortor. In at porttitor ex, eget proin";
    buf = malloc(9999);
    memset(buf, '\0', 9999);

    printf("Reminder: Use an empty DISK every time you run this test.\n");

    printf("SECTION 1\n");
    if ( (status = MkDir_Wrapper("folder1/")) == SUCCESS ) { Shutdown(); return ERROR; }
    if ( (status = MkDir_Wrapper("folder1")) == ERROR ) { Shutdown(); return ERROR; }
    if ( (status = MkDir_Wrapper("folder2")) == ERROR ) { Shutdown(); return ERROR; }
    if ( (status = MkDir_Wrapper("folder3")) == ERROR ) { Shutdown(); return ERROR; }
    if ( (status = MkDir_Wrapper("folder4/subfolder1")) == SUCCESS ) { Shutdown(); return ERROR; }

    if ( (status = ChDir_Wrapper("folder1/subfolder1")) == SUCCESS ) { Shutdown(); return ERROR; }
    if ( (status = ChDir_Wrapper("//folder1///")) == ERROR ) { Shutdown(); return ERROR; }
    if ( (status = MkDir_Wrapper("folder1/subfolder1")) == SUCCESS ) { Shutdown(); return ERROR; }
    if ( (status = MkDir_Wrapper("subfolder1")) == ERROR ) { Shutdown(); return ERROR; }
    if ( (status = MkDir_Wrapper("subfolder2")) == ERROR ) { Shutdown(); return ERROR; }
    if ( (status = MkDir_Wrapper("subfolder3")) == ERROR ) { Shutdown(); return ERROR; }

    if ( (status = ChDir_Wrapper("./subfolder1/")) == ERROR ) { Shutdown(); return ERROR; }
    if ( (status = ChDir_Wrapper("/folder1/subfolder1")) == ERROR ) { Shutdown(); return ERROR; }
    if ( (fd_1 = Create_Wrapper("1.txt")) == ERROR ) { Shutdown(); return ERROR; }
    if ( (fd_2 = Create_Wrapper("2.txt")) == ERROR ) { Shutdown(); return ERROR; }
    if ( (fd_3 = Create_Wrapper("3.txt")) == ERROR ) { Shutdown(); return ERROR; }
    if ( (bytes = Read_Wrapper(fd_3, buf, 10)) != 0 ) { Shutdown(); return ERROR; }
    if ( (fd_4 = Open_Wrapper("4.txt")) >= 0 ) { Shutdown(); return ERROR; }
    if ( (status = Create_Wrapper("///")) >= 0 ) { Shutdown(); return ERROR; }
    if ( (status = Create_Wrapper("./")) >= 0 ) { Shutdown(); return ERROR; }
    if ( (status = ChDir_Wrapper("1.txt")) == SUCCESS) { Shutdown(); return ERROR; }

    printf("SECTION 2\n");
    if ( (status = Write_Wrapper(fd_1, content_1024, strlen(content_1024))) != 1024 ) { Shutdown(); return ERROR; }
    if ( (status = Write_Wrapper(fd_2, content_1024, strlen(content_1024))) != 1024 ) { Shutdown(); return ERROR; }
    if ( (status = Write_Wrapper(fd_3, content_1024, strlen(content_1024))) != 1024 ) { Shutdown(); return ERROR; }
    if ( (status = Write_Wrapper(10, content_1024, strlen(content_1024))) >= 0 ) { Shutdown(); return ERROR; }

    if ( (bytes = Seek_Wrapper(fd_1, 0, SEEK_SET)) != 0 ) { Shutdown(); return ERROR; }
    if ( (bytes = Seek_Wrapper(fd_2, -1024, SEEK_CUR)) != 0 ) { Shutdown(); return ERROR; }
    if ( (bytes = Seek_Wrapper(fd_2, 1025, SEEK_CUR)) != 1025 ) { Shutdown(); return ERROR; }
    if ( (bytes = Seek_Wrapper(fd_2, 0, SEEK_SET)) != 0 ) { Shutdown(); return ERROR; }
    if ( (bytes = Seek_Wrapper(fd_3, -1024, SEEK_END)) != 0 ) { Shutdown(); return ERROR; }

    if ( (bytes = Read_Wrapper(fd_1, buf, 24)) != 24 ) { Shutdown(); return ERROR; }
    if ( (bytes = Read_Wrapper(fd_1, buf, 1000)) != 1000 ) { Shutdown(); return ERROR; }
    if ( (bytes = Read_Wrapper(fd_2, buf, 24)) != 24 ) { Shutdown(); return ERROR; }
    if ( (bytes = Read_Wrapper(fd_2, buf, 1024)) != 1000 ) { Shutdown(); return ERROR; }
    if ( (bytes = Read_Wrapper(fd_3, buf, 0)) != 0 ) { Shutdown(); return ERROR; }
    if ( (bytes = Read_Wrapper(fd_3, buf, -10)) == SUCCESS ) { Shutdown(); return ERROR; }
    if ( (bytes = Read_Wrapper(fd_3, buf, 1000)) != 1000 ) { Shutdown(); return ERROR; }
    if ( (bytes = Read_Wrapper(fd_4, buf, 10)) == SUCCESS ) { Shutdown(); return ERROR; }
    // fd_1, fd_2, fd_3 are all at EOF
    if ( (bytes = Seek_Wrapper(fd_1, -2000, SEEK_END)) >= 0 ) { Shutdown(); return ERROR; }
    if ( (bytes = Seek_Wrapper(fd_1, -1024, SEEK_CUR)) != 0) { Shutdown(); return ERROR; }
    if ( (bytes = Seek_Wrapper(fd_1, 2000, SEEK_SET)) != 2000) { Shutdown(); return ERROR; }
    if ( (status = Write_Wrapper(fd_1, content_1024, strlen(content_1024))) != 1024 ) { Shutdown(); return ERROR; }
    if ( (bytes = Seek_Wrapper(fd_1, 0, SEEK_SET)) != 0) { Shutdown(); return ERROR; }
    if ( (bytes = Read_Wrapper(fd_1, buf, 5000)) != 3024 ) { Shutdown(); return ERROR; } // Read hole.

    if ( (status = Close_Wrapper(fd_1)) == ERROR ) { Shutdown(); return ERROR; }
    if ( (status = Close_Wrapper(fd_2)) == ERROR ) { Shutdown(); return ERROR; }
    if ( (status = Close_Wrapper(fd_3)) == ERROR ) { Shutdown(); return ERROR; }

    printf("SECTION 3\n");
    if ( (status = Link_Wrapper("1.txt", "1_link_1.txt")) == ERROR ) { Shutdown(); return ERROR; }
    if ( (status = Link_Wrapper("1.txt", "1_link_2.txt")) == ERROR ) { Shutdown(); return ERROR; }
    if ( (status = Link_Wrapper("//", "1_link_2.txt")) == SUCCESS ) { Shutdown(); return ERROR; }
    if ( (status = Link_Wrapper("../subfolder3", "1_link_2.txt")) == SUCCESS ) { Shutdown(); return ERROR; }
    if ( (fd_0 = Open_Wrapper("1.txt")) == ERROR ) { Shutdown(); return ERROR; }
    if ( (fd_1 = Open_Wrapper("1_link_1.txt")) == ERROR ) { Shutdown(); return ERROR; }
    if ( (status = Seek_Wrapper(fd_1, 0, SEEK_END)) != 3024) { Shutdown(); return ERROR; }
    if ( (bytes = Write_Wrapper(fd_1, content_1024, strlen(content_1024))) != 1024 ) { Shutdown(); return ERROR; }
    if ( (bytes = Read_Wrapper(fd_0, buf, 9999)) != 4048 ) { Shutdown(); return ERROR; }
    if ( (status = Seek_Wrapper(fd_1, 0, SEEK_SET)) != 0) { Shutdown(); return ERROR; }
    if ( (bytes = Read_Wrapper(fd_1, buf, 9999)) != 4048 ) { Shutdown(); return ERROR; }

    if ( (status = Link_Wrapper("1_link_1.txt", "../1_link_1_link_1.txt")) == ERROR ) { Shutdown(); return ERROR; }
    if ( (fd_2 = Open_Wrapper("../1_link_1_link_1.txt")) == ERROR ) { Shutdown(); return ERROR; }
    if ( (bytes = Read_Wrapper(fd_2, buf, 5000)) != 4048 ) { Shutdown(); return ERROR; }
    if ( (status = Unlink_Wrapper("1_link_1.txt")) == ERROR ) { Shutdown(); return ERROR; }
    if ( (status = Unlink_Wrapper("1_link_2.txt")) == ERROR ) { Shutdown(); return ERROR; }
    if ( (status = Unlink_Wrapper("../1_link_1_link_1.txt")) == ERROR ) { Shutdown(); return ERROR; }

    if ( (status = Unlink_Wrapper("/folder1")) == SUCCESS ) { Shutdown(); return ERROR; }
    if ( (status = Unlink_Wrapper("2.txt")) == ERROR ) { Shutdown(); return ERROR; }
    if ( (status = Unlink_Wrapper("3.txt")) == ERROR ) { Shutdown(); return ERROR; }
    if ( (status = Unlink_Wrapper("3.txt")) == SUCCESS ) { Shutdown(); return ERROR; }
    if ( (status = Unlink_Wrapper("4.txt")) == SUCCESS ) { Shutdown(); return ERROR; }
    if ( (status = Link_Wrapper("4.txt", "4_link_1.txt")) == SUCCESS ) { Shutdown(); return ERROR; }

    if ( (status = ChDir_Wrapper("../subfolder2")) == ERROR ) { Shutdown(); return ERROR; }
    if ( (status = RmDir_Wrapper("/folder1")) == SUCCESS ) { Shutdown(); return ERROR; }
    if ( (status = RmDir_Wrapper("/subfolder1")) == SUCCESS ) { Shutdown(); return ERROR; }
    if ( (status = RmDir_Wrapper("/folder1/subfolder1")) == SUCCESS ) { Shutdown(); return ERROR; }
    if ( (status = RmDir_Wrapper("/folder1/subfolder2/")) == SUCCESS ) { Shutdown(); return ERROR; }
    if ( (status = RmDir_Wrapper("/folder1/subfolder2")) == ERROR ) { Shutdown(); return ERROR; }
    if ( (status = RmDir_Wrapper("/folder3")) == ERROR ) { Shutdown(); return ERROR; }

    printf("SECTION 4\n");
    if ( (status = ChDir_Wrapper("../")) == SUCCESS ) { Shutdown(); return ERROR; }
    if ( (status = ChDir_Wrapper("/")) == ERROR ) { Shutdown(); return ERROR; }
    if ( (status = MkDir_Wrapper("folder1")) == SUCCESS ) { Shutdown(); return ERROR; }
    if ( (status = MkDir_Wrapper("folder1/subfolder2")) == ERROR ) { Shutdown(); return ERROR; }
    if ( (status = MkDir_Wrapper("folder3")) == ERROR ) { Shutdown(); return ERROR; }

    if ( (fd_4 = Create_Wrapper("folder1/subfolder1/1.txt")) == ERROR ) { Shutdown(); return ERROR; } // Truncate 1.txt
    if ( (bytes = Read_Wrapper(fd_4, buf, 10)) != 0 ) { Shutdown(); return ERROR; } // Read truncated file.

    if ( (fd_4 = Open_Wrapper("///")) == ERROR ) { Shutdown(); return ERROR; }
    if ( (bytes = Read_Wrapper(fd_4, buf, 1024)) == ERROR ) { Shutdown(); return ERROR; }
    if ( (status = Close_Wrapper(fd_4)) == ERROR ) { Shutdown(); return ERROR; }

    if ( (status = MkDir_Wrapper("///")) == SUCCESS ) { Shutdown(); return ERROR; }
    if ( (status = RmDir_Wrapper("///")) == SUCCESS ) { Shutdown(); return ERROR; }

    for (i = 0; i < 10; i++) {
        if ( (status = MkDir_Wrapper("/folder2/subfolder1")) == ERROR ) { Shutdown(); return ERROR; }
        if ( (fd_0 = Create_Wrapper("/folder2/subfolder1/1.txt")) == ERROR ) { Shutdown(); return ERROR; }
        if ( (status = Close_Wrapper(fd_0)) == ERROR ) { Shutdown(); return ERROR; }
        if ( (status = Unlink_Wrapper("/folder2/subfolder1/1.txt")) == ERROR ) { Shutdown(); return ERROR; }
        if ( (status = RmDir_Wrapper("/folder2/subfolder1")) == ERROR ) { Shutdown(); return ERROR; }
        if ( (status = RmDir_Wrapper("/folder2")) == ERROR ) { Shutdown(); return ERROR; }
        if ( (status = MkDir_Wrapper("/folder2")) == ERROR ) { Shutdown(); return ERROR; }
    }

    printf("SECTION 5\n");
    if ( (status = ChDir_Wrapper("///folder2///subfolder1//")) == SUCCESS ) { Shutdown(); return ERROR; }
    if ( (status = ChDir_Wrapper("//folder1/subfolder2//..//../folder2//")) == ERROR ) { Shutdown(); return ERROR; }
    if ( (fd_0 = Open_Wrapper(".///")) == ERROR ) { Shutdown(); return ERROR; }
    if ( (bytes = Read_Wrapper(fd_0, buf, 10)) != 10 ) { Shutdown(); return ERROR; }
    if ( (bytes = Write_Wrapper(fd_0, content_1024, strlen(content_1024))) >= 0 ) { Shutdown(); return ERROR; }
    if ( (status = Close_Wrapper(fd_0)) == ERROR ) { Shutdown(); return ERROR; }

    if ( (fd_1 = Create_Wrapper(content_1024)) == SUCCESS ) { Shutdown(); return ERROR; }
    if ( (status = MkDir_Wrapper(content_1024)) == SUCCESS ) { Shutdown(); return ERROR; }
    if ( (status = Link_Wrapper("/folder1/subfolder1/1.txt", content_1024)) == SUCCESS ) { Shutdown(); return ERROR; }



    printf("TEST PASSED!\n");
    Shutdown();

    return 0;
}

int MkDir_Wrapper(char *pathname) {
    int status;

    printf("Making directory %s ...\n", pathname);
    status = MkDir(pathname);
    if (status == ERROR) {
        printf("Error: MkDir() failed.\n");
        return ERROR;
    }
    return 0;
}

int RmDir_Wrapper(char *pathname) {
    int status;

    printf("Removing directory %s ...\n", pathname);
    status = RmDir(pathname);
    if (status == ERROR) {
        printf("Error: RmDir() failed.\n");
        return ERROR;
    }
    return 0;
}

int ChDir_Wrapper(char *pathname) {
    int status;

    printf("Change directory to %s ...\n", pathname);
    status = ChDir(pathname);
    if (status == ERROR) {
        printf("Error: ChDir() failed.\n");
        return ERROR;
    }
    return 0;
}

int Create_Wrapper(char *pathname) {
    int status;

    printf("Creating file %s ...\n", pathname);
    status = Create(pathname);
    if (status == ERROR) {
        printf("Error: Create() failed.\n");
        return ERROR;
    }
    return status;
}

int Read_Wrapper(int fd, void *buf, int size) {
    int status;

    printf("Reading from fd %d with size = %d bytes ...\n", fd, size);
    status = Read(fd, buf, size);
    if (status == ERROR) {
        printf("Error: Read failed.\n");
        return -1;
    } else {
        printf("Success: read %d bytes.\n", status);
    }
    return status;
}

int Write_Wrapper(int fd, void *buf, int size) {
    int status;

    printf("Writing to fd %d with size = %d bytes ...\n", fd, size);
    status = Write(fd, buf, size);
    if (status == ERROR) {
        printf("Error: Write failed.\n");
        return ERROR;
    }

    printf("Success: wrote %d bytes.\n", status);
    return status;
}

int Close_Wrapper(int fd) {
    int status;

    printf("Closing fd %d ...\n", fd);
    status = Close(fd);
    if (status == ERROR) {
        printf("Error: Close failed.\n");
        return ERROR;
    }
    return 0;
}

int Sync_Wrapper() {
    int status;

    printf("Syncing...\n");
    status = Sync();
    if (status == ERROR) {
        printf("Error: Sync failed.\n");
        return ERROR;
    }
    return 0;
}

int Open_Wrapper(char *pathname) {
    int fd;

    printf("Opening file %s ...\n", pathname);
    fd = Open(pathname);
    if (fd == ERROR) {
        printf("Error: Open failed.\n");
        return ERROR;
    } else {
        printf("Success: opened fd %d\n", fd);
    }
    return fd;
}

int Link_Wrapper(char *oldname, char *newname) {
    int status;

    printf("Linking file %s (new) to file %s (old) ...\n", newname, oldname);
    status = Link(oldname, newname);
    if (status == ERROR) {
        printf("Error: Link file %s (new) to file %s (old) failed.\n", newname, oldname);
        return ERROR;
    }
    return 0;
}

int Unlink_Wrapper(char *pathname) {
    int status;

    printf("Unlinking file %s ...\n", pathname);
    status = Unlink(pathname);
    if (status == ERROR) {
        printf("Error: Unlinking file %s failed.\n", pathname);
        return ERROR;
    }
    return 0;
}

int Seek_Wrapper(int fd, int offset, int whence) {
    int status;

    printf("Seeking fd: %d; offset: %d, whence: %d ...\n", fd, offset, whence);
    status = Seek(fd, offset, whence);
    if (status == ERROR) {
        printf("Error: Seeking.\n");
        return ERROR;
    }
    return status;
}
