#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int flag = 0;
int main() {
    flag = 1;
    int id = fork();
    if (id == 0) {
        wait(NULL);
        printf("flag = %d\n", flag);
    }
    else {
        flag = 2;
        int iid = fork();
        if (iid == 0) {
            wait(NULL); 
            printf("flag = %d\n", flag);
        }
        else {
            flag = 3;
            printf("flag = %d\n", flag);
        }
    }
    return 0;
}