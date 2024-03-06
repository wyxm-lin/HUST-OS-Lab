/*
 * This app fork a child process to read and write the heap data from parent process.
 * Because implemented copy on write, when child process only read the heap data,
 * the physical address is the same as the parent process.
 * But after writing, child process heap will have different physical address.
 */

// #include "user/user_lib.h"
// #include "util/types.h"
// #include "util/string.h"

// int main(void)
// {
//     printu("parent\n");
//     int pid = fork();
//     if (pid == 0)
//     {
//         printu("child\n");
//     }
//     exit(0);
//     return 0;
// }

// /*
//  * This app fork a child process to read and write the heap data from parent process.
//  * Because implemented copy on write, when child process only read the heap data,
//  * the physical address is the same as the parent process.
//  * But after writing, child process heap will have different physical address.
//  */

#include "user/user_lib.h"
#include "util/types.h"
#include "util/string.h"

int main(void)
{
    char *heap_data = (char*)naive_malloc(100);
    strcpy(heap_data, "hello world");
    printu("%s\n", heap_data);    
    printu("the physical address of parent process heap is: ");
    printpa((void*)heap_data);
    int pid = fork();
    if (pid == 0)
    {
        printu("the physical address of child process heap before copy on write is: ");
        printpa((void*)heap_data);
        strcpy(heap_data, "world");
        printu("%s\n", heap_data);    
        printu("the physical address of child process heap after copy on write is: ");
        printpa((void*)heap_data);
    }
    exit(0);
    return 0;
}