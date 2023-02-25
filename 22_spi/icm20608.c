#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/spi/spi.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include "icm20608.h"

#define DEVICE_NAME     "icm20608"
#define DEVICE_NUM      1
#define DEBUG_LOG

#ifdef DEBUG_LOG
  #define spi_debug(format,args...) printk(format,##args)
#else
  #define spi_debug(format,args...)
#endif

struct char_dev {
    int devID;
    int major;
    int minor;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    void *private_data;
    u16 gyro_x;
    u16 gyro_y;
    u16 gyro_z;
    u16 accel_x;
    u16 accel_y;
    u16 accel_z;
    u16 temp;
};
static struct char_dev icm20608_dev;

static int icm20608_read_reg(struct spi_device* spi, u8 reg, u8 *buf)
{
    int ret;
    struct spi_message m;
    struct spi_transfer *t;
    u8 rxdata[2];
    u8 txdata[2];

    t = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);
    txdata[0] = reg | SPI_READ;    //第8位为1,代表读操作
    txdata[1] = 0xff;              //全双工，返回无效数据

    t->tx_buf = txdata;
    t->rx_buf = rxdata;
    t->len = 2;

    spi_message_init(&m);
    spi_message_add_tail(t, &m);

    ret = spi_sync(spi, &m);

    memcpy(buf, &rxdata[1], 1);

    kfree(t);

    return ret;
}

static int icm20608_write_reg(struct spi_device* spi, u8 reg, u8 buf)
{
    int ret;
    struct spi_message m;
    struct spi_transfer *t;
    u8 txdata[2];

    t = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);
    txdata[0] = reg & SPI_WRITE;    //第8位为0,代表写操作
    txdata[1] = buf;

    t->tx_buf = txdata;
    t->len = 2;

    spi_message_init(&m);
    spi_message_add_tail(t, &m);

    ret = spi_sync(spi, &m);

    kfree(t);

    return ret;
}

static void icm20608_read_data(struct char_dev *dev)
{
    u8 data[2];
    struct spi_device *spi = (struct spi_device *)dev->private_data;

    //GYRO DATA
    icm20608_read_reg(spi, ICM20_GYRO_XOUT_H, &data[0]);
    icm20608_read_reg(spi, ICM20_GYRO_XOUT_L, &data[1]);
    dev->gyro_x = (u16)((data[0] << 8) | data[1]);
    icm20608_read_reg(spi, ICM20_GYRO_YOUT_H, &data[0]);
    icm20608_read_reg(spi, ICM20_GYRO_YOUT_L, &data[1]);
    dev->gyro_y = (u16)((data[0] << 8) | data[1]);
    icm20608_read_reg(spi, ICM20_GYRO_ZOUT_H, &data[0]);
    icm20608_read_reg(spi, ICM20_GYRO_ZOUT_L, &data[1]);
    dev->gyro_z = (u16)((data[0] << 8) | data[1]);

    //ACCEL DATA
    icm20608_read_reg(spi, ICM20_ACCEL_XOUT_H, &data[0]);
    icm20608_read_reg(spi, ICM20_ACCEL_XOUT_L, &data[1]);
    dev->accel_x = (u16)((data[0] << 8) | data[1]);
    icm20608_read_reg(spi, ICM20_ACCEL_YOUT_H, &data[0]);
    icm20608_read_reg(spi, ICM20_ACCEL_YOUT_L, &data[1]);
    dev->accel_y = (u16)((data[0] << 8) | data[1]);
    icm20608_read_reg(spi, ICM20_ACCEL_ZOUT_H, &data[0]);
    icm20608_read_reg(spi, ICM20_ACCEL_ZOUT_L, &data[1]);
    dev->accel_z = (u16)((data[0] << 8) | data[1]);

    //TEMP data
    icm20608_read_reg(spi, ICM20_TEMP_OUT_H, &data[0]);
    icm20608_read_reg(spi, ICM20_TEMP_OUT_L, &data[1]);
    dev->temp = (u16)((data[0] << 8) | data[1]);
}

int icm20608_open(struct inode *nd, struct file *fp)
{
    fp->private_data = &icm20608_dev;

    return 0;
}

int icm20608_close(struct inode *nd, struct file *fp)
{
    return 0;
}

ssize_t icm20608_read(struct file *fp, char __user *buf, size_t size, loff_t *ppos)
{
    u16 data[7];
    int ret;
    struct char_dev *dev = (struct char_dev*)fp->private_data;

    icm20608_read_data(dev);
    data[0] = dev->gyro_x;
    data[1] = dev->gyro_y;
    data[2] = dev->gyro_z;
    data[3] = dev->accel_x;
    data[4] = dev->accel_y;
    data[5] = dev->accel_z;
    data[6] = dev->temp;

    ret = copy_to_user(buf, data, size);

    return ret;
}

ssize_t icm20608_write(struct file *fp, const char __user *buf, size_t size, loff_t *ppos)
{
    return 0;
}

static struct file_operations icm20608_ops = {
    .owner      =   THIS_MODULE,
    .open       =   icm20608_open,
    .release    =   icm20608_close,
    .read       =   icm20608_read,
    .write      =   icm20608_write,
};


static int setup_icm20608(struct char_dev* dev)
{
    u8 value = 0;
    struct spi_device *spi = (struct spi_device *)dev->private_data;

    //reset
    icm20608_write_reg(spi, ICM20_PWR_MGMT_1, DEVICE_RESET);
    spi_debug("restart icm20608\n");
    mdelay(50);
    //使能ICM20608
    icm20608_write_reg(spi, ICM20_PWR_MGMT_1, 0x01);
    mdelay(50);

    //chip ID
    icm20608_read_reg(spi, ICM20_WHO_AM_I, &value);
    if (ICM20608_CHIP_ID != value) {
        spi_debug("faile to read chipID %x\n", value);
        return -1;
    }

    //设置采样频率
    icm20608_write_reg(spi, ICM20_SMPLRT_DIV, 0x00);
    //设置gyro和accelerometer为1000dps，8g
    icm20608_write_reg(spi, ICM20_GYRO_CONFIG, 0x18);
    icm20608_write_reg(spi, ICM20_ACCEL_CONFIG, 0x18);
    //bypass DLPF
    icm20608_write_reg(spi, ICM20_CONFIG, 0x04);
    icm20608_write_reg(spi, ICM20_ACCEL_CONFIG2, 0x04);
    //关闭低功耗模式
    icm20608_write_reg(spi, ICM20_LP_MODE_CFG, 0x00);
    //关闭芯片FIFO功能
    icm20608_write_reg(spi, ICM20_FIFO_EN, 0x00);
    //是能gyro，accel，temp功能
    icm20608_write_reg(spi, ICM20_ACCEL_CONFIG2, 0x00);


    return 0;
}

static int icm20608_probe(struct spi_device *spi)
{
    int ret;

    //分配设备号
    icm20608_dev.major = 0;
    if (icm20608_dev.major) {
        icm20608_dev.devID = MKDEV(icm20608_dev.major, 0);
        icm20608_dev.minor = 0;
        register_chrdev_region(icm20608_dev.devID, DEVICE_NUM, DEVICE_NAME);
    } else {
        alloc_chrdev_region(&icm20608_dev.devID, 0, DEVICE_NUM, DEVICE_NAME);
        icm20608_dev.major = MAJOR(icm20608_dev.devID);
        icm20608_dev.minor = MINOR(icm20608_dev.devID);
    }

    //初始化并向系统添加字符设备
    cdev_init(&icm20608_dev.cdev, &icm20608_ops);
    ret = cdev_add(&icm20608_dev.cdev, icm20608_dev.devID, DEVICE_NUM);
    if (ret) {
        spi_debug("cdev add failed!\n");
        ret = -EINVAL;
        goto err_add;
    }

    //在文件系统中创建并注册一个设备
    icm20608_dev.class = class_create(THIS_MODULE, DEVICE_NAME);
    if (IS_ERR(icm20608_dev.class)) {
        spi_debug("create icm20608 class failed!");
        ret = -EINVAL;
        goto err_class;
    }
    icm20608_dev.device = device_create(icm20608_dev.class, NULL, icm20608_dev.devID, NULL, DEVICE_NAME);
    if (IS_ERR(icm20608_dev.device)) {
        spi_debug("create icm20608 device failed!");
        ret = -EINVAL;
        goto err_device;
    }

    spi->mode = SPI_MODE_0;
    spi_setup(spi);

    icm20608_dev.private_data = spi;

    ret = setup_icm20608(&icm20608_dev);
    if (ret) {
        spi_debug("init icm20608 error!\n");
        goto err_init;
    }

    return 0;

err_init:
err_device:
    class_destroy(icm20608_dev.class);
err_class:
    cdev_del(&icm20608_dev.cdev);
err_add:
    unregister_chrdev_region(icm20608_dev.devID, DEVICE_NUM);

    return ret;
}

static int icm20608_remove(struct spi_device *spi)
{
    device_destroy(icm20608_dev.class, icm20608_dev.devID);
    class_destroy(icm20608_dev.class);
    cdev_del(&icm20608_dev.cdev);
    unregister_chrdev_region(icm20608_dev.devID, DEVICE_NUM);

    return 0;
}

static struct of_device_id icm20608_of_table[] = {
    { .compatible = "icm,icm20608" },
    { /* Sentinel */ }
};

static struct spi_device_id icm20608_ids[] = {
    { "device,icm20608", 0 },
    { /* Sentinel */ }
};

static struct spi_driver icm20608_driver = {
    .probe      =   icm20608_probe,
    .remove     =   icm20608_remove,
    .driver     =   {
        .name           =   "icm20608",
        .owner          =   THIS_MODULE,
        .of_match_table =   icm20608_of_table,
    },
    .id_table   =   icm20608_ids,
};

static int __init icm20608_init(void)
{
    int ret;

    ret = spi_register_driver(&icm20608_driver);

    return ret;
}

static void __exit icm20608_exit(void)
{
    spi_unregister_driver(&icm20608_driver);
}

module_init(icm20608_init);
module_exit(icm20608_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Alex_min");
