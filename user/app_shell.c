#include "user_lib.h"
#include "string.h"
#include "util/types.h"

int main() {
    printu("\n======== Shell Start ========\n");
    printu_dir("$ ");
    while (TRUE) {
        char command[256];
        scanfu("%s", command);
        printu("Command: %s\n", command);
        if (strcmp(command, "exit") == 0) {
            printu("Shell exit\n");
            break;
        }
        printu_dir("shell$ ");
    }
    exit(0);
    return 0;
}