#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    int fd = 0;
    int ret  = 0;
    char *file_name;
    char led_state;
    int cnt = 0;

    file_name = argv[1];

    fd = open(file_name, O_RDWR);
    if (fd < 0) {
        printf("open file %s failed!\r\n", file_name);
        return -1;
    }

    while (1) {
        sleep(5);
        cnt++;
        printf("APP run time:%d.........\r\n", cnt);
        if (5 == cnt)
            break;
    }

    ret = close(fd);
    if (ret < 0) {
        printf("close file %s failed!\r\n", file_name);
        return -1;
    }
    printf("APP exit\r\n");

    return 0;
}