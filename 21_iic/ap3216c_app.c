#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

struct dev_data {
    uint16_t ir;
    uint16_t als;
    uint16_t ps;
};

int main(int argc, char *argv[])
{
    int fd = 0;
    int size = 0;
    char *file_name;
    char *buf;
    int ret;
    struct dev_data data = {0};

    file_name = argv[1];

    fd = open(file_name, O_RDWR);
    if (fd < 0) {
        printf("open file %s failed!\r\n", file_name);
        return -1;
    }

    while (1) {
        read(fd, &data, sizeof(data));
        printf("ir:%d als:%d ps:%d\n", data.ir, data.als, data.ps);
        usleep(1000000);
    }

    ret = close(fd);
    if (ret < 0) {
        printf("close file %s failed!\r\n", file_name);
        return -1;
    }

    return 0;
}