#include <stdio.h>

#include <comp421/yalnix.h>
#include <comp421/iolib.h>

int
main()
{
    struct Stat sb;
    int fd;
    int nch;
    int result;

    fd = Create("/hole-target-1");
    printf("Create fd %d\n", fd);

    nch = Write(fd, "abcdefghijklmnopqrstuvwxyz", 26);
    printf("Write nch %d\n", nch);

    result = Stat("/hole-target-1", &sb);
    printf("Stat result %d\n", result);
    printf("File size %d == 26\n", sb.size);

    nch = Seek(fd, 1024, SEEK_END);
    printf("Seek %d == 1050\n", nch);

    result = Stat("/hole-target-1", &sb);
    printf("Stat result %d\n", result);
    printf("File size %d == 26\n", sb.size);

    nch = Write(fd, "abcdefghijklmnopqrstuvwxyz", 26);
    printf("Write nch %d\n", nch);

    result = Stat("/hole-target-1", &sb);
    printf("Stat result %d\n", result);
    printf("File size %d == 1072\n", sb.size);

    printf("Confirm that hole-target-1 inode->direct[1] = 0\n");

    fd = Create("/hole-target-1");
    printf("Create fd %d\n", fd);
    printf("This should have freed two blocks instead of three.\n");
    Shutdown();
    return 0;
}
