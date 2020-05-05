#include <stdio.h>

#include <comp421/yalnix.h>
#include <comp421/iolib.h>

int
main()
{
    struct Stat sb;
    int fd;
    int result;

    printf("Note: Format before running this test.\n");

    fd = Create("/size-test-1");
    printf("Create fd %d\n", fd);

    fd = Create("/size-test-2");
    printf("Create fd %d\n", fd);

    fd = Create("/size-test-3");
    printf("Create fd %d\n", fd);

    result = Stat("/", &sb);
    printf("Stat result %d\n", result);
    printf("Root size %d == 160\n", sb.size);

    result = Unlink("/size-test-1");
    printf("Unlink result %d\n", result);

    result = Stat("/", &sb);
    printf("Root size %d == 160\n", sb.size);

    result = Unlink("/size-test-2");
    printf("Unlink result %d\n", result);

    result = Stat("/", &sb);
    printf("Root size %d == 160\n", sb.size);

    result = Unlink("/size-test-3");
    printf("Unlink result %d\n", result);

    result = Stat("/", &sb);
    printf("Root size %d == 64\n", sb.size);

    Shutdown();
    return 0;
}
