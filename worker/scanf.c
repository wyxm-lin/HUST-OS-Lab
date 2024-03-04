#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>
#include <stdbool.h>

#define BUFFER_SIZE 1024

void custom_vfscanf(int fd, const char *format, va_list args) {
    const char *p = format;
    char *sval;
    int *ival;
    float *fval;
    void **pval;
    char buffer[BUFFER_SIZE];
    int bytes_read;
    int buffer_index = 0;

    while ((bytes_read = read(fd, buffer + buffer_index, BUFFER_SIZE - buffer_index)) > 0) {
        for (int i = 0; i < bytes_read; i++) {
            if (*p == '%') {
                switch (*(p + 1)) {
                    case 'd':
                        ival = va_arg(args, int *);
                        *ival = 0;
                        while (buffer[buffer_index] >= '0' && buffer[buffer_index] <= '9') {
                            *ival = *ival * 10 + buffer[buffer_index] - '0';
                            buffer_index++;
                        }
                        p += 2;
                        break;
                    case 'f':
                        fval = va_arg(args, float *);
                        *fval = 0;
                        float decimal = 0.1;
                        while ((buffer[buffer_index] >= '0' && buffer[buffer_index] <= '9') || buffer[buffer_index] == '.') {
                            if (buffer[buffer_index] == '.') {
                                decimal = 1.0;
                                buffer_index++;
                                continue;
                            }
                            *fval = *fval + (buffer[buffer_index] - '0') * decimal;
                            decimal *= 0.1;
                            buffer_index++;
                        }
                        p += 2;
                        break;
                    case 'c':
                        sval = va_arg(args, char *);
                        *sval = buffer[buffer_index];
                        buffer_index++;
                        p += 2;
                        break;
                    case 's':
                        sval = va_arg(args, char *);
                        while (buffer[buffer_index] != ' ' && buffer[buffer_index] != '\n' && buffer[buffer_index] != '\0') {
                            *sval++ = buffer[buffer_index++];
                        }
                        *sval = '\0'; // 结尾加上 null 字符
                        p += 2;
                        break;
                    case 'x':
                        ival = va_arg(args, int *);
                        *ival = 0;
                        while (true) {
                            if (buffer[buffer_index] >= '0' && buffer[buffer_index] <= '9') {
                                *ival = *ival * 16 + buffer[buffer_index] - '0';
                            } else if (buffer[buffer_index] >= 'a' && buffer[buffer_index] <= 'f') {
                                *ival = *ival * 16 + buffer[buffer_index] - 'a' + 10;
                            } else if (buffer[buffer_index] >= 'A' && buffer[buffer_index] <= 'F') {
                                *ival = *ival * 16 + buffer[buffer_index] - 'A' + 10;
                            } else {
                                break;
                            }
                            buffer_index++;
                        }
                        p += 2;
                        break;
                    // case 'p':
                    //     pval = va_arg(args, void **);
                    //     *pval = (void *)(intptr_t)0;
                    //     while (true) {
                    //         if (buffer[buffer_index] >= '0' && buffer[buffer_index] <= '9') {
                    //             *pval = (void *)((intptr_t)*pval * 16 + buffer[buffer_index] - '0');
                    //         } else if (buffer[buffer_index] >= 'a' && buffer[buffer_index] <= 'f') {
                    //             *pval = (void *)((intptr_t)*pval * 16 + buffer[buffer_index] - 'a' + 10);
                    //         } else if (buffer[buffer_index] >= 'A' && buffer[buffer_index] <= 'F') {
                    //             *pval = (void *)((intptr_t)*pval * 16 + buffer[buffer_index] - 'A' + 10);
                    //         } else {
                    //             break;
                    //         }
                    //         buffer_index++;
                    //     }
                    //     p += 2;
                    //     break;
                    default:
                        p++;
                        break;
                }
            } else {
                if (*p == buffer[buffer_index]) {
                    buffer_index++;
                    p++;
                }
            }

            if (*p == '\0' || buffer[buffer_index] == '\n') {
                break;
            }
        }

        if (*p == '\0' || buffer[buffer_index] == '\n') {
            break;
        }
    }
}

void custom_fscanf(int fd, const char *format, ...) {
    va_list args;
    va_start(args, format);

    custom_vfscanf(fd, format, args);

    va_end(args);
}

int main() {
    int fd = open("input.txt", O_RDONLY); // 打开文件进行读取
    if (fd == -1) {
        perror("文件打开失败");
        return 1;
    }

    int num;
    float floatValue;
    char character;
    char string[100];
    int hex;
    void *ptr;

    custom_fscanf(fd, "%d %f %c %s %x %p", &num, &floatValue, &character, string, &hex, &ptr);

    printf("您输入的整数是: %d\n", num);
    printf("您输入的浮点数是: %.2f\n", floatValue);
    printf("您输入的字符是: %c\n", character);
    printf("您输入的字符串是: %s\n", string);
    printf("您输入的十六进制数是: %x\n", hex);
    printf("您输入的指针是: %p\n", ptr);

    close(fd); // 关闭文件

    return 0;
}
