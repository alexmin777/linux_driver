#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    int fd = 0;
    char *file_name;
    int ret = 0;
    int value = 0;

    file_name = argv[1];

    fd = open(file_name, O_RDWR);
    if (fd < 0) {
        printf("open file %s failed!\r\n", file_name);
        return -1;
    }

    while (1) {
        read(fd, &value, sizeof(value));
        printf("read key value %d\n", value);
        sleep(1);
    }

    ret = close(fd);

    return 0;
}