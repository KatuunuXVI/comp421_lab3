#include <stdio.h>

#include <comp421/yalnix.h>
#include <comp421/iolib.h>

int
main()
{
    int result;

    result = MkDir("/trmdir2");
    printf("MkDir result %d\n", result);

    result = ChDir("/trmdir2/");
    printf("ChDir result %d\n", result);

    result = RmDir("/trmdir2");
    printf("RmDir should succeed: %d\n", result);

    result = Create("test-file");
    printf("Create should fail: %d\n", result);

    Shutdown();
    return 0;
}
