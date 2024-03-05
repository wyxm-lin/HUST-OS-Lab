#include "user_lib.h"
#include "string.h"
#include "util/types.h"

int main()
{
    printu("\n======== Shell Start ========\n");
    printu_dir("$ ");
    while (TRUE)
    {
        char command[256];
        scanfu("%s", command);
        if (work(command) == -1)
            break;
        printu_dir("$ ");
    }
    exit(0);
    return 0;
}