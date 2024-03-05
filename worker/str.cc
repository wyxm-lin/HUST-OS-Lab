#include <stdio.h>
#include <iostream>
#include <cstring>

using namespace std;

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

int main() {
    cout << "===========================================\n";
    char a[256] = "/../.././test";
    ParsePath(a);
    cout << "============================================\n";
    char b[256] = "/.././test";
    ParsePath(b);
    cout << "============================================\n";
    char c[256] = "/.././test/..";
    ParsePath(c);
    cout << "============================================\n";
    char d[256] = "/test/..";
    ParsePath(d);
    cout << "============================================\n";
    char e[256] = "/test/../";
    ParsePath(e);
    cout << "============================================\n";
    char f[256] = "/..";
    ParsePath(f);
    cout << "============================================\n";

    return 0;
}



// #include <iostream>
// #include <stdio.h>
// #include <cstring>


// using namespace std;

// typedef enum {invalid, valid} status;

// char origin_path[256] = "/home";
// char root_path[256] = "/";
// char current_path[256] = "/home/wyxm";

// char* strip_redundant_backslashes(char* path) {
//     char* ptr = path;
//     while (*ptr != '\0' && *ptr == '/') {
//         ptr ++;
//     }
//     ptr --;
//     return ptr;
// }

// void strip_relative_path(char* path) {
//     char new_path[256];
//     memset(new_path, 0, sizeof(new_path));
//     char* new_path_ptr = new_path;
//     char* path_ptr = path;
//     while (*path_ptr != '\0') {
//         if (*path_ptr == '.' && *(path_ptr + 1) == '.' && *(path_ptr + 2) == '/') {
//             path_ptr += 3;
//             new_path_ptr --;
//             while (*new_path_ptr != '/') {
//                 new_path_ptr --;
//             }
//             new_path_ptr ++;
//         } else if (*path_ptr == '.' && *(path_ptr + 1) == '/') {
//             path_ptr += 2;
//         } else {
//             *new_path_ptr = *path_ptr;
//             new_path_ptr ++;
//             path_ptr ++;
//         }
//     }
//     strcpy(path, new_path);
// }

// status path_cat(char* path) {
//     printf("origin %s\n", path);
//     char new_path[256];
//     memset(new_path, 0, sizeof(new_path));
//     char* new_path_ptr = new_path;
//     strcpy(new_path_ptr, origin_path);
//     strcat(new_path_ptr, path);
//     // printf("first %s\n", new_path_ptr);
//     new_path_ptr = strip_redundant_backslashes(new_path_ptr);
//     // printf("second %s\n", new_path_ptr);
//     strip_relative_path(new_path_ptr);
//     printf("final %s\n", new_path_ptr);
//     return valid;
// }

// void get_path(char* path) {
//     if (path[0] == '\0') {
//         strcpy(path, current_path);
//     } 
//     else if (path[0] == '/') {
//         return;
//     } 
//     else {
        
//     }
// }

// void check(char* path) {
//     char new_path[256];
//     memset(new_path, 0, sizeof(new_path));
//     char* new_path_ptr = new_path;
//     strcpy(new_path_ptr, origin_path);
//     strcat(new_path_ptr, "/");
//     strcat(new_path_ptr, path);

//     printf("%s\n", new_path);

//     char ret[256];
//     memset(ret, 0, sizeof(ret));

//     char* retptr = ret;

//     char* token = strtok(new_path_ptr, "/");
//     while (token != NULL) {
//         if (strcmp(token, ".") == 0) {
//             // do nothing
//         } 
//         else if (strcmp(token, "..") == 0) {
//             cout << "token: ..\n";
//             if (*retptr == '\0') {
//                 cout << "here\n";
//             }
//             else {
//                 cout << *retptr << endl;
//             }
//             while( *retptr != '\0' && *retptr != '/') {
//                 *retptr = '\0';
//                 retptr --;
//             }
//             if (ret != retptr)
//                 *retptr = '\0';
//             printf("%s\n", ret);
//         } 
//         else if (strcmp(token, "/") == 0) {
//             // do nothing
//         }
//         else {
//             cout << "token: " <<  token << "\n";
//             strcat(ret, "/");
//             strcat(ret, token);
//             retptr = ret + strlen(ret) - 1;
//             printf("%s\n", ret);
//         }
//         token = strtok(NULL, "/");
//     }
//     // if (*ret == '\0')
//     //     *ret = '/';
//     printf("%s\n", ret);
// }

// int main() {
//     cout << "===========================================\n";
//     char a[256] = "/../.././test";
//     check(a);
//     cout << "============================================\n";
//     char b[256] = "/.././test";
//     check(b);
//     cout << "============================================\n";
//     char c[256] = "/.././test/..";
//     check(c);
//     cout << "============================================\n";
//     char d[256] = "test/..";
//     check(d);
//     cout << "============================================\n";
//     char e[256] = "test/../";
//     check(e);
//     cout << "============================================\n";
//     char f[256] = "..";
//     check(f);
//     cout << "============================================\n";

//     return 0;
// }