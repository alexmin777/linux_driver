#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "ap3216c"
#define DEVICE_NUM  1

/*
 * ap3216c Register Address Definition
 */
#define AP3216C_SYSTEMCONG    0x00  /* Configuration register */
#define AP3216C_INTSTATUS     0x01  /* Interrupt status register */
#define AP3216C_INTCLEAR      0x02  /* Interrupt clear register */
#define AP3216C_IRDATALOW     0x0A  /* IR Data Low byte */
#define AP3216C_IRDATAHIGH    0x0B  /* IR Data High byte */
#define AP3216C_ALSDATALOW    0x0C  /* ALS Data Low byte */
#define AP3216C_ALSDATAHIGH   0x0D  /* ALS Data High byte */
#define AP3216C_PSDATALOW     0x0E  /* PS Data Low byte */
#define AP3216C_PSDATAHIGH    0x0F  /* PS Data High byte */

struct char_dev {
    int devID;
    int major;
    int minor;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    void *adapter;
};

struct dev_data {
    u16 ir;
    u16 als;
    u16 ps;
};

static struct char_dev ap3216c_dev;

static inline u8 ap3216c_read_reg(struct char_dev *dev, u8 reg, u8 *data)
{
    struct i2c_client *client = (struct i2c_client *)dev->adapter;
    struct i2c_msg msgs[2] = {
        [0] = {
            .addr   =   client->addr,
            .flags  =   0,    //写操作
            .len    =   1,
            .buf    =   &reg,
        },

        [1] = {
            .addr   =   client->addr,
            .flags  =   I2C_M_RD,    //写操作
            .len    =   1,
            .buf    =   data,
        },
    };

    if (2 == i2c_transfer(client->adapter, msgs, 2))   {
        return 0;
    } else {
        printk("ap3216c read reg:%02x failed!\n", reg);
        return -1;
    }

}

static inline u8 ap3216c_write_reg(struct char_dev *dev, u8 reg, u8 data)
{
    struct i2c_client *client = (struct i2c_client *)dev->adapter;
    u8 buf[2] = {reg, data};

    struct i2c_msg msg = {
        .addr   =   client->addr,
        .flags  =   0,    //写操作
        .len    =   2,
        .buf    =   buf,
    };

    if (1 == i2c_transfer(client->adapter, &msg, 1))   {
        return 0;
    } else {
        printk("ap3216c write reg:%02x data:%02x failed!\n", buf[0], buf[1]);
        return -1;
    }

}

int ap3216c_open(struct inode *nd, struct file *fp)
{
    u8 value;
    int ret;
    fp->private_data = &ap3216c_dev;

    ret = ap3216c_write_reg(fp->private_data, AP3216C_SYSTEMCONG, 0x04);  //复位设备
    mdelay(10);
    ret = ap3216c_write_reg(fp->private_data, AP3216C_SYSTEMCONG, 0x03);  //使能ir+ps+als功能

    ret = ap3216c_read_reg(fp->private_data, AP3216C_SYSTEMCONG, &value);  //验证设备是否工作
    if (0x03 == value) {
        printk("ap3216c work!\n");
    } else {
        printk("ap3216c open failed!\n");
    }

    return 0;
}

ssize_t ap3216c_read(struct file *fp, char __user *buf, size_t size, loff_t * ppos)
{
    struct char_dev *dev = (struct char_dev *)fp->private_data;
    u8 data[8] = {0};
    struct dev_data senseor_data = {0};
    int i;
    int ret;

    for (i = 0; i < 8; i++)
        ap3216c_read_reg(dev, AP3216C_IRDATALOW + i, &data[i]);

    //IR数据
    if (0x80 & data[0]) {   /* 检验IR数据是否有效 */
        senseor_data.ir = 0;
    } else {
        senseor_data.ir = ((u16)data[1] << 2) | (data[0] & 0x03);
    }
    
    //ALS数据
    senseor_data.als = ((u16)data[3] << 8) | data[2];

    //PS数据
    if (0x40 & data[4]) {   /* 检验PS数据是否有效 */
        senseor_data.ps = 0;
    } else {
        senseor_data.ps = ((u16)(data[5] & 0x3f) << 4) | (data[4] & 0x0f);
    }

    ret = copy_to_user((void *)buf, (void *)&senseor_data, sizeof(senseor_data));

    return ret;
}

ssize_t ap326c_write(struct file *fp, const char __user *buf, size_t size, loff_t *ppos)
{
    printk("write ap3216c\n");

    return 0;
}

int ap326c_close(struct inode *nd, struct file *fp)
{
    printk("close ap3216c\n");

    return 0;
}

static const struct file_operations ap3216c_ops = {
    .owner      =   THIS_MODULE,
    .open       =   ap3216c_open,
    .read       =   ap3216c_read,
    .write      =   ap326c_write,
    .release    =   ap326c_close,
};

static int ap3216c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{

    int ret;

    printk("ap3216c_probe!\n");

    ap3216c_dev.major = 0;
    //分配设备号
    if (ap3216c_dev.major) {
        ap3216c_dev.devID = MKDEV(ap3216c_dev.major, 0);
        ap3216c_dev.minor = 0;
        register_chrdev_region(ap3216c_dev.devID, DEVICE_NUM, DEVICE_NAME);
    } else {
        alloc_chrdev_region(&ap3216c_dev.devID, 0, DEVICE_NUM, DEVICE_NAME);
        ap3216c_dev.major = MAJOR(ap3216c_dev.devID);
        ap3216c_dev.minor = MINOR(ap3216c_dev.devID);
    }

    //初始化并向系统添加字符设备
    cdev_init(&ap3216c_dev.cdev, &ap3216c_ops);
    ret = cdev_add(&ap3216c_dev.cdev, ap3216c_dev.devID, DEVICE_NUM);
    if (ret) {
        printk("cdev add failed!\n");
        ret = -EINVAL;
        goto err_add;
    }

    //在文件系统中创建并注册一个设备
    ap3216c_dev.class = class_create(THIS_MODULE, DEVICE_NAME);
    if (IS_ERR(ap3216c_dev.class)) {
        printk("create ap3216c class failed!");
        ret = -EINVAL;
        goto err_class;
    }
    ap3216c_dev.device = device_create(ap3216c_dev.class, NULL, ap3216c_dev.devID, NULL, DEVICE_NAME);
    if (IS_ERR(ap3216c_dev.device)) {
        printk("create ap3216c device failed!");
        ret = -EINVAL;
        goto err_device;
    }

    ap3216c_dev.adapter = client;

    return 0;

err_device:
    class_destroy(ap3216c_dev.class);
err_class:
    cdev_del(&ap3216c_dev.cdev);
err_add:
    unregister_chrdev_region(ap3216c_dev.devID, DEVICE_NUM);

    return ret;
}

static int ap3216c_remove(struct i2c_client *client)
{
    printk("ap3216c_remove!\n");

    device_destroy(ap3216c_dev.class, ap3216c_dev.devID);
    class_destroy(ap3216c_dev.class);
    cdev_del(&ap3216c_dev.cdev);
    unregister_chrdev_region(ap3216c_dev.devID, DEVICE_NUM);
    return 0;
}

static const struct i2c_device_id ap3216c_ids[] = {
	{ "lsc,ap3216c", 0 },
	{ /* Sentinel */ },
};

static const struct of_device_id ap3216c_of_match_table[] = {
	{ .compatible = "lsc,ap3216c" },
	{ /* Sentinel */ },
};

static struct i2c_driver ap3216c_driver = {
	.probe = ap3216c_probe,
	.remove = ap3216c_remove,
	.driver = {
		.name = "ap3216c",
		.owner = THIS_MODULE,
        .of_match_table = ap3216c_of_match_table,
	},
    .id_table = ap3216c_ids,
};

static int __init ap3216c_init(void)
{
    i2c_add_driver(&ap3216c_driver);

    return 0;
}

static void __exit ap3216c_exit(void)
{
    i2c_del_driver(&ap3216c_driver);
}

module_init(ap3216c_init);
module_exit(ap3216c_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Alex_min");