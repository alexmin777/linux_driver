#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/spi/spi.h>
#include <linux/delay.h>
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
    struct device_node *nd;
    int cs_gpio;
    void *private_data;
};
static struct char_dev icm20608_dev;

int icm20608_open(struct inode *nd, struct file *fp)
{
    return 0;
}

int icm20608_close(struct inode *nd, struct file *fp)
{
    return 0;
}

ssize_t icm20608_read(struct file *fp, char __user *buf, size_t size, loff_t *ppos)
{
    return 0;
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

static int icm20608_read_reg(struct char_dev* dev, u8 reg, u8 *buf)
{
    int ret;
    struct spi_message m;
    struct spi_transfer *t;
    u8 rxdata[2];
    u8 txdata[2];
    struct spi_device *spi = (struct spi_device *)dev->private_data;

    if (reg < 0x80) {
        spi_debug("reg is invalid\n");
        return -ENXIO;
    }

    t = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);
    txdata[0] = reg | SPI_READ;    //第8位为1,代表读操作
    txdata[1] = 0xff;              //全双工，返回无效数据

    t->tx_buf = txdata;
    t->rx_buf = rxdata;
    t->len = 2;

    spi_message_init(&m);
    spi_message_add_tail(t, &m);

    //拉低片选信号
    gpio_set_value(dev->cs_gpio, 0);
    //同步传输，会阻塞，等待数据传输完成
    ret = spi_sync(spi, &m);
    //拉高片选信号
    gpio_set_value(dev->cs_gpio, 0);

    kfree(t);

    memcpy(buf, &rxdata[1], 1);

    return ret;
}

static int icm20608_write_reg(struct char_dev* dev, u8 reg, u8 buf)
{
    int ret;
    struct spi_message m;
    struct spi_transfer *t;
    u8 txdata[2];
    struct spi_device *spi = (struct spi_device *)dev->private_data;

    if (reg < 0x80) {
        spi_debug("reg is invalid\n");
        return -ENXIO;
    }

    t = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);
    txdata[0] = reg & SPI_WRITE;    //第8位为0,代表写操作
    txdata[1] = buf;              //全双工，返回无效数据

    t->tx_buf = txdata;
    t->len = 2;

    spi_message_init(&m);
    spi_message_add_tail(t, &m);

    //拉低片选信号
    gpio_set_value(dev->cs_gpio, 0);
    //同步传输，会阻塞，等待数据传输完成
    ret = spi_sync(spi, &m);
    //拉高片选信号
    gpio_set_value(dev->cs_gpio, 0);

    kfree(t);

    return ret;
}

static void setup_icm20608(struct char_dev* dev)
{
    u8 value = 0;
    //struct spi_device *spi = (struct spi_device *)dev->private_data;

    //reset
    icm20608_write_reg(dev, POWER_MANAGEMENT1, DEVICE_RESET);
    spi_debug("restart icm20608\n");
    mdelay(50);
    //chip ID
    icm20608_read_reg(dev, WHO_AM_I, &value);
    if (CHIP_ID != value) {
        spi_debug("faile to read chipID\n");
    }
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

    icm20608_dev.nd = of_get_parent(spi->dev.of_node);
    if (!icm20608_dev.nd)
        goto error_node;

    icm20608_dev.cs_gpio = of_get_named_gpio(icm20608_dev.nd, "cs-gpios", 0);
    if (icm20608_dev.cs_gpio < 0) {
        spi_debug("get cs-gpios failed\n");
        goto err_gpio;
    }

    ret = gpio_request(icm20608_dev.cs_gpio, "cs");
    if (ret) {
        spi_debug("cs-gpio request failled\n");
        goto err_gpio;
    }
    ret = gpio_direction_output(icm20608_dev.cs_gpio, 1);
    if (ret) {
        spi_debug("set cs output failed\n");
        goto err_out;
    }

    spi->mode = SPI_MODE_0;
    spi_setup(spi);

    icm20608_dev.private_data = spi;

    setup_icm20608(&icm20608_dev);

    return 0;

err_out:
    gpio_free(icm20608_dev.cs_gpio);
err_gpio:
error_node:
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
    gpio_free(icm20608_dev.cs_gpio);
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