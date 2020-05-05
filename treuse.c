#include <stdio.h>

#include <comp421/yalnix.h>
#include <comp421/iolib.h>

int
main()
{
    int fd;
    int result;

    fd = Create("/reuse-target");
    printf("Create fd %d\n", fd);

    fd = Create("/used-1");
    printf("Create fd %d\n", fd);

    fd = Create("/used-2");
    printf("Create fd %d\n", fd);

    result = Unlink("/reuse-target");
    printf("Unlink result %d\n", result);

    fd = Create("/used-3");
    printf("Create fd %d\n", fd);

    fd = Create("/used-4");
    printf("Create fd %d\n", fd);

    printf("Call tls to confirm the order is 3, 1, 2, 4.\n");
    Shutdown();
    return 0;
}
