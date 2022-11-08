#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h>


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
    fd_set readfds;
    struct timeval timeout;

    file_name = argv[1];

    fd = open(file_name, O_RDWR | O_NONBLOCK);
    if (fd < 0) {
        printf("open file %s failed!\r\n", file_name);
        return -1;
    }

    while (1) {
        //清除并把fd添加进readfs中
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);

        //设置超时时间
        timeout.tv_sec = 3;
        timeout.tv_usec = 0;
        ret = select(fd + 1, &readfds, NULL, NULL, &timeout);
        switch (ret) {
            case 0:     //超时
                printf("timeout\r\n");
                break;
            case -1:    //错误
                printf("error\r\n");
                break;
            default:
                if (FD_ISSET(fd, &readfds)) {
                    ret = read(fd, &value, sizeof(value));
                    if (ret < 0) {
                        
                    } else {
                        if (value)
                            printf("key value = %02x\r\n", value);
                    }
                }
                break;
        }
    }

    close(fd);

    return 0;
}