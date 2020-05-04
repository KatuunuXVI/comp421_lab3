#include <stdio.h>

#include <comp421/yalnix.h>
#include <comp421/iolib.h>

int
main()
{
    int fd;
    int nch;
    int status;
    char ch;

    fd = Create("/seek");
    printf("Create fd %d\n", fd);

    nch = Write(fd, "abcde", 5);
    printf("Write nch %d\n", nch);

    nch = Seek(fd, 3, SEEK_SET);
    printf("Seek %d should be 3\n", nch);

    nch = Write(fd, "12345", 5);
    printf("Write nch %d\n", nch);

    nch = Seek(fd, 3, SEEK_CUR);
    printf("Seek %d should be 11\n", nch);

    nch = Write(fd, "zyx", 3);
    printf("Write nch %d\n", nch);

    nch = Seek(fd, 3, SEEK_END);
    printf("Seek %d should be 17\n", nch);

    nch = Write(fd, "ABC", 3);
    printf("Write nch %d\n", nch);

    status = Close(fd);
    printf("Close status %d\n", status);

    Sync();

    fd = Open("/seek");
    printf("Open fd %d\n", fd);
    printf("Expecting to read 'abc12345   zxc   ABC'\n");

    int i;
    for (i = 0; i < 20; i++) {
        nch = Read(fd, &ch, 1);
        printf("ch 0x%x '%c'\n", ch, ch);
    }

    status = Close(fd);
    printf("Close status %d\n", status);

    Shutdown();
}
