#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    int fd = 0;
    int size = 0;
    char *file_name;
    char *buf;
    int ret;

    file_name = argv[1];

    fd = open(file_name, O_RDWR);
    if (fd < 0) {
        printf("open file %s failed!\r\n", file_name);
        return -1;
    }

    read(fd, buf, size);

    ret = close(fd);
    if (ret < 0) {
        printf("close file %s failed!\r\n", file_name);
        return -1;
    }

    return 0;
}