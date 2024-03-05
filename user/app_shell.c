#include "user_lib.h"
#include "string.h"
#include "util/types.h"

#define EXIT 1

typedef enum
{
    valid,
    invalid
} status;

#define RFSRoot "/RAMDISK0"

void ParsePath(char *path);
void my_sscanf(char *dst, char *src, int *index);
int work(char *commandlist);

void ls(char *commandList, int* index);
void pwd();
void cd(char *commandList, int* index);

int main()
{
    printu("\n======== Shell Start ========\n");
    printu_dir("$ ");
    while (TRUE)
    {
        char command[256];
        scanfu("%s", command);
        if (work(command) == EXIT)
            break;
        printu_dir("$ ");
    }
    exit(0);
    return 0;
}

void ParsePath(char *path)
{
    char result[512];
    memset(result, 0, sizeof(result));
    char *ptr = result;
    char *token = strtok(path, "/");

    while (token != NULL)
    {
        if (strcmp(token, ".") == 0 || strcmp(token, "/") == 0)
        {
            // do nothing
        }
        else if (strcmp(token, "..") == 0)
        {
            while (*ptr != '\0' && *ptr != '/')
            {
                *ptr = '\0';
                ptr--;
            }
            *ptr = '\0';
        }
        else
        {
            strcat(result, "/");
            strcat(result, token);
            ptr = result + strlen(result) - 1;
        }
        token = strtok(NULL, "/");
    }
    if (*result == '\0')
        *result = '/';

    strcpy(path, result);
}

void my_sscanf(char *dst, char *src, int *index)
{
    int i = *index;
    while (src[i] == ' ' && src[i] != '\0')
    {
        i++;
    }
    char* ptr = dst;
    while (src[i] != ' ' && src[i] != '\0')
    {
        *ptr = src[i];
        i++;
        ptr++;
    }
    *ptr = '\0';
    *index = i;
}

int work(char *commandlist)
{
    char command[256];
    memset(command, 0, 256);

    int idx = 0;
    my_sscanf(command, commandlist, &idx);

    if (strcmp(command, "exit") == 0)
    {
        return EXIT;
    }
    else if (strcmp(command, "ls") == 0)
    {
        ls(commandlist, &idx);
    }
    else if (strcmp(command, "pwd") == 0)
    {
        pwd();
    }
    else if (strcmp(command, "cd") == 0)
    {
        cd(commandlist, &idx);
    }
    else if (strcmp(command, "mkdir") == 0)
    {
    }
    else
    {
        printu("Command not found: %s\n", command);
    }
    return 0;
}

void ls(char *commandlist, int* index) {
    char arg[256];
    memset(arg, 0, 256);
    my_sscanf(arg, commandlist, index);

    char path[256];
    memset(path, 0, 256);
    read_cwd(path);
    strcat(path, "/");
    strcat(path, arg);
    ParsePath(path);

    int dir_fd = opendir_u(path);
    if (dir_fd < 0)
    {
        printu("ls: cannot access '%s': No such file or directory\n", arg);
        return;
    }
    printu("[name]               [inode_num]\n");
    struct dir dir;
    int width = 256;
    while (readdir_u(dir_fd, &dir) == 0)
    {
        char name[width + 1];
        memset(name, ' ', width + 1);
        name[width] = '\0';
        if (strlen(dir.name) < width)
        {
            strcpy(name, dir.name);
            name[strlen(dir.name)] = ' ';
            printu("%s %d\n", name, dir.inum);
        }
        else
            printu("%s %d\n", dir.name, dir.inum);
    }
    printu("\n");
    closedir_u(dir_fd);
}

void pwd() {
    char path[256];
    read_cwd(path);
    printu("%s\n", path);
}

void cd(char *commandlist, int* index) {
    char arg[256];
    memset(arg, 0, 256);
    my_sscanf(arg, commandlist, index);

    char path[256];
    read_cwd(path);
    strcat(path, "/");
    strcat(path, arg);
    ParsePath(path);

    int result = change_cwd(path);
    if (result == -1) {
        printu("cd: %s: No such file or directory\n", arg);
    }
}