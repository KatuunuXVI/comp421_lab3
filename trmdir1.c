#include <stdio.h>

#include <comp421/yalnix.h>
#include <comp421/iolib.h>

int
main()
{
    int result;
    int fd;
    result = MkDir("/trmdir1");
    printf("MkDir result %d\n", result);

    fd = Create("/trmdir1/test-file");
    printf("Create fd %d\n", fd);

    result = RmDir("/trmdir1");
    printf("RmDir should fail: %d\n", result);

    result = Unlink("/trmdir1/test-file");
    printf("Unlink result %d\n", result);

    result = RmDir("/trmdir1");
    printf("RmDir should succeed: %d\n", result);

    Shutdown();
    return 0;
}
