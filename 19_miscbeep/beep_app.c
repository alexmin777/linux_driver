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
    char beep_state;

    file_name = argv[1];

    fd = open(file_name, O_RDWR);
    if (fd < 0) {
        printf("open file %s failed!\r\n", file_name);
        return -1;
    }

    beep_state = atoi(argv[2]);
    if (0 == beep_state || 1 == beep_state) {
        ret = write(fd, &beep_state, sizeof(beep_state));
        if (ret < 0) {
            printf("read file %s failed!\r\n", file_name);
            return -1;
        }
        else {
            printf("read chrdevbase buf %d\r\n", beep_state);
        }
    }
    else {
        printf("please enter 1 or 0 to turn on/turn off\n");
    }

    ret = close(fd);
    if (ret < 0) {
        printf("close file %s failed!\r\n", file_name);
        return -1;
    }

    return 0;
}