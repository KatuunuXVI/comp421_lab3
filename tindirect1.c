#include <stdio.h>

#include <comp421/yalnix.h>
#include <comp421/iolib.h>

int
main()
{
    int fd;
    int nch;
    fd = Create("/mass-1");

    nch = Write(fd, "abcdefghijklmnopqrstuvwxyz", 26);
    printf("Write nch %d\n", nch);

    nch = Seek(fd, 6144, SEEK_END);
    printf("Seek %d == 6170\n", nch);

    nch = Write(fd, "abcdefghijklmnopqrstuvwxyz", 26);
    printf("Write nch %d\n", nch);

    nch = Unlink("/mass-1");
    printf("Unlink %d\n", nch);

    Shutdown();
    return 0;
}
