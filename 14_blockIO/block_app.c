#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>


int main(int argc, char *argv[])
{
    int fd = 0;
    char *file_name;
    int ret = 0;
    int value = 0;
    int key;
    unsigned int cmd;
    unsigned int arg;
    unsigned char str[100];

    file_name = argv[1];

    fd = open(file_name, O_RDWR);
    if (fd < 0) {
        printf("open file %s failed!\r\n", file_name);
        return -1;
    }

    while (1) {
        ret = read(fd, &value, sizeof(value));
        if (ret < 0) {
            
        } else {
            if (value)
                printf("key value = %02x\r\n", value);
        }
    }

    close(fd);

    return 0;
}