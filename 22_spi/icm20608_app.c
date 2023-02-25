#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

struct imu_data {
    uint16_t gyro_x;
    uint16_t gyro_y;
    uint16_t gyro_z;
    uint16_t accel_x;
    uint16_t accel_y;
    uint16_t accel_z;
    uint16_t temp;
};
static struct imu_data icm20608_data;

int main(int argc, char *argv[])
{
    int fd;
    char *file_name;
    uint16_t data_buf[7];
    int ret;
    float gyro_x, gyro_y, gyro_z;
    float accel_x, accel_y, accel_z;
    float temp;

    if (argc != 2) {
        printf("Usage:./app file_name");
        return -1;
    }

    file_name = argv[1];

    fd = open(file_name, O_RDONLY);
    if (fd < 0) {
        printf("open %s failed\n", file_name);
    }

    while (1) {
        ret = read(fd, data_buf, sizeof(data_buf));
        if (ret == 0) {
            icm20608_data.gyro_x = data_buf[0];
            icm20608_data.gyro_y = data_buf[1];
            icm20608_data.gyro_z = data_buf[2];
            icm20608_data.accel_x = data_buf[3];
            icm20608_data.accel_x = data_buf[4];
            icm20608_data.accel_x = data_buf[5];
            icm20608_data.temp = data_buf[6];

            gyro_x = (float)icm20608_data.gyro_x / 16.4;
            gyro_y = (float)icm20608_data.gyro_y / 16.4;
            gyro_z = (float)icm20608_data.gyro_z / 16.4;
            accel_x = (float)icm20608_data.accel_x / 2048;
            accel_y = (float)icm20608_data.accel_y / 2048;
            accel_z = (float)icm20608_data.accel_z / 2048;

            temp = ((float)icm20608_data.temp - 25) / 326.8 + 25;

            printf("raw gx%d gy%d gz%d ax%d ay%d az%d temp%d\n", icm20608_data.gyro_x, \
            icm20608_data.gyro_y, icm20608_data.gyro_z, icm20608_data.accel_x, icm20608_data.accel_y, \
            icm20608_data.accel_z, icm20608_data.temp);
            printf("actual gx%.2f°/S gy%.2f°/S gz%.2f°/S ax%.2fg ay%.2fg az%.2fg temp%.2f°C\n", gyro_x, \
            gyro_y, gyro_z, accel_x, accel_y, \
            accel_z, temp);

        } else {
            printf("read failed\n");
            return -1;
        }
        usleep(100 * 1000);
    }

    close(fd);

    return 0;
}