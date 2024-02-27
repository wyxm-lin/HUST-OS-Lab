#include <iostream>
using namespace std;

int main(int argc, char *argv[]) {
    //char a[] = "dadssssssss";
    printf("argc: %p\n", &argc);
    printf("argv[0]: %p\n", argv[0]);
    printf("argv[1]: %p\n", argv[1]);
    return 0;
}