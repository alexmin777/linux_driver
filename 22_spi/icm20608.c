#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/spi/spi.h>

#define DEVICE_NAME     "icm20608"
#define DEVICE_NUM      1

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

static int icm20608_read_reg(struct char_dev* dev, u8 reg, void *buf, int len)
{
    struct spi_message m;
    struct spi_transfer *t;
    struct spi_device *spi = (struct spi_device *)dev->private_data;

    t = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);
    t.tx_buf = reg;
    t.len = len;

    //拉低片选信号
    gpio_set_value(dev->cs_gpio, 0);


    //拉高片选信号
    gpio_set_value(dev->cs_gpio, 0);

    kfree(t);
}

static void setup_icm20608(struct char_dev* dev)
{
    struct spi_device *spi = (struct spi_device *)dev->private_data;


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
        printk("cdev add failed!\n");
        ret = -EINVAL;
        goto err_add;
    }

    //在文件系统中创建并注册一个设备
    icm20608_dev.class = class_create(THIS_MODULE, DEVICE_NAME);
    if (IS_ERR(icm20608_dev.class)) {
        printk("create icm20608 class failed!");
        ret = -EINVAL;
        goto err_class;
    }
    icm20608_dev.device = device_create(icm20608_dev.class, NULL, icm20608_dev.devID, NULL, DEVICE_NAME);
    if (IS_ERR(icm20608_dev.device)) {
        printk("create icm20608 device failed!");
        ret = -EINVAL;
        goto err_device;
    }

    icm20608_dev.nd = of_get_parent(spi->dev.of_node);
    if (!icm20608_dev.nd)
        goto error_node;

    icm20608_dev.cs_gpio = of_get_named_gpio(icm20608_dev.nd, "cs-gpios", 0);
    if (icm20608_dev.cs_gpio < 0) {
        printk("get cs-gpios failed\n");
        goto err_gpio;
    }

    ret = gpio_request(icm20608_dev.cs_gpio, "cs");
    if (ret) {
        printk("cs-gpio request failled\n");
        goto err_gpio;
    }
    ret = gpio_direction_output(icm20608_dev.cs_gpio, 1);
    if (ret) {
        printk("set cs output failed\n");
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