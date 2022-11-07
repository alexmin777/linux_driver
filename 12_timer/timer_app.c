#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define TIMER_MAGIC_BASE    'T'
#define CMD_OPEN    (_IO(TIMER_MAGIC_BASE, 0x01))
#define CMD_CLOSE   (_IO(TIMER_MAGIC_BASE, 0x02))
#define CMD_MODIFY  (_IOW(TIMER_MAGIC_BASE, 0x03, int))

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
        printf("Enter 1/2/3 to /open/close/modity timer:");
        ret = scanf("%d", &key);
        if (1 != ret) {
            gets(str);
        }

        if (1 == key)
            cmd = CMD_OPEN;
        else if (2 == key)
            cmd = CMD_CLOSE;
        else if (3 == key) {
            cmd = CMD_MODIFY;
            printf("Enter timer period:");
            ret = scanf("%d", &arg);
            if (1 != ret) {
                gets(str);
            }
        }
        ioctl(fd, cmd, &arg);
    }

    ret = close(fd);

    return 0;
}