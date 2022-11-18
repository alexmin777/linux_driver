#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <signal.h>

int fd = 0;

void signal_handle(int num)
{
    int key_value;

    read(fd, &key_value, sizeof(key_value));
    printf("key value %02x\r\n", key_value);
}

int main(int argc, char *argv[])
{
    char *file_name;
    int flag;

    file_name = argv[1];

    fd = open(file_name, O_RDWR | O_NONBLOCK);
    if (fd < 0) {
        printf("open file %s failed!\r\n", file_name);
        return -1;
    }

    signal(SIGIO, signal_handle);
    fcntl(fd, F_SETOWN, getpid());        //将本应用程序的进程号告诉给内核
    flag  = fcntl(fd, F_GETFL);           //获取当前的进程状态
    fcntl(fd, F_SETFL, flag | FASYNC);    //开启 当前进程异步通知功能

    while (1) {
        sleep(1);
    }

    close(fd);

    return 0;
}