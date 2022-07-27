#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    int ret = 0;
    int fd = 0;
    char *file_name;
    char readbuf[100];
    char writebuf[100] = {"alex's device"};

    file_name = argv[1];

    fd = open(file_name, O_RDWR);
    if (fd < 0) {
        printf("open file %s failed!\r\n", file_name);
        return -1;
    }

    if (0 == atoi(argv[2])) {
        ret = read(fd, readbuf, sizeof(readbuf));
        if (ret < 0) {
            printf("read file %s failed!\r\n", file_name);
            return -1;
        }
        else {
            printf("read chrdevbase buf %s\r\n", readbuf);
        }
    }
    else if (1 == atoi(argv[2])) {
        ret = write(fd, writebuf, sizeof(writebuf));
        if (ret < 0) {
            printf("write file %s failed!\r\n", file_name);
            return -1;
        }
    }
    else {
        printf("please enter 1 or 2 to read/write\n");
    }

    ret = close(fd);
    if (ret < 0) {
        printf("close file %s failed!\r\n", file_name);
        return -1;
    }


    return 0;
}