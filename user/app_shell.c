#include "user_lib.h"
#include "string.h"
#include "util/types.h"

#define EXIT 1

typedef enum
{
    valid,
    invalid
} status;

void my_sscanf(char *dst, char *src, int *index);
int work(char *commandlist);
void ls(char *path);

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

void my_sscanf(char *dst, char *src, int *index)
{
    int i = *index;
    while (src[i] == ' ' && src[i] != '\0')
    {
        i++;
    }
    while (src[i] != ' ' && src[i] != '\0')
    {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
    *index = i;
}

status path_cat(char *path)
{
    if (path[0] == '\0' || path[0] == '/' || (path[0] == '.' && path[1] == '/') || (path[0] == '.' && path[1] == '.' && path[2] == '/'))
    {
        if (path[0] == '/')
            ;
        else if (path[0] == '\0')
        {
            char cwd[256];
            read_cwd(cwd);
            strcpy(path, cwd);
        }
        else if (path[0] == '.' && path[1] == '/')
        {
            char cwd[256];
            read_cwd(cwd);
            strcat(cwd, path + 2);
            strcpy(path, cwd);
        }
        else if (path[0] == '.' && path[1] == '.' && path[2] == '/')
        {
            char cwd[256];
            read_cwd(cwd);
            strcat(cwd, path + 3);
            strcpy(path, cwd);
        }
        return valid;
    }
    return invalid;
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
        char arg[256];
        memset(arg, 0, 256);
        my_sscanf(arg, commandlist, &idx);
        ls(arg);
    }
    else if (strcmp(command, "pwd") == 0)
    {
        char path[256];
        read_cwd(path);
        printu("%s\n", path);
    }
    else if (strcmp(command, "cd") == 0)
    {
        char arg[256];
        memset(arg, 0, 256);
        my_sscanf(arg, commandlist, &idx);
        change_cwd(arg);
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

void ls(char *path)
{
    if (path_cat(path) == valid)
    {
        printu("the path is %s\n", path);
        int dir_fd = opendir_u(path);
        if (dir_fd == -1)
        {
            printu("ls: cannot access '%s': No such file or directory\n", path);
            return;
        }
        printu("---------- ls command -----------\n");
        printu("ls \"%s\":\n", path);
        printu("[name]               [inode_num]\n");
        struct dir dir;
        int width = 20;
        while (readdir_u(dir_fd, &dir) == 0)
        {
            // we do not have %ms :(
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
        printu("------------------------------\n");
        closedir_u(dir_fd);
    }
}
