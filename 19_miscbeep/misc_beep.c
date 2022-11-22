#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/of_gpio.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>


#define MISC_BEEP_MINOR 255 //255代表自适应次设备号
#define MISC_BEEP_NAME "misc_beep"
#define BEEP_ON         0
#define BEEP_OFF        1

static struct of_device_id misc_beep_match[] = {
    {.compatible = "alientekn,beep",},
    { /* sentinel */ },
};

struct misc_beep {
    int gpio_num;
    struct device_node *nd;
};

static struct misc_beep beep_dev;

static int misc_beep_open(struct inode *inode, struct file *filp)
{
    filp->private_data = &beep_dev;

    return 0;
}

static ssize_t misc_beep_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
    int ret;
    unsigned char ctl_buf[1];
    unsigned char status;
    struct misc_beep *dev = (struct misc_beep *)filp->private_data;

    ret = copy_from_user(ctl_buf, buf, size);
    if (ret < 0) {
        printk("kernel write failed!\r\n");
        return -EFAULT;
    }
    status = ctl_buf[0];

    if (BEEP_OFF == status)
        gpio_set_value(dev->gpio_num, 1);
    else
        gpio_set_value(dev->gpio_num, 0);
    
    return 0;
}

static int misc_beep_release(struct inode *inode, struct file *filp)
{
    struct misc_beep *dev = (struct misc_beep *)filp->private_data;

    //gpio_set_value(dev->gpio_num, BEEP_OFF);

    return 0;
}

static const struct file_operations misc_beep_fops = {
    .owner      =   THIS_MODULE,
    .open       =   misc_beep_open,
    .write      =   misc_beep_write,
    .release    =   misc_beep_release,
};

static struct miscdevice misc_beep_device = {
    .minor  =   MISC_BEEP_MINOR,
    .name   =   MISC_BEEP_NAME,
    .fops   =   &misc_beep_fops,
};

int misc_beep_probe(struct platform_device *dev)
{
    int ret;

    //设备节点
    beep_dev.nd = dev->dev.of_node;
	if (!beep_dev.nd) {
		printk("failed to find misc beep node!\r\n");
		return -EINVAL;
	}

    //获取GPIO号
    beep_dev.gpio_num = of_get_named_gpio(beep_dev.nd, "beep-gpios", 0);
    if (!gpio_is_valid(beep_dev.gpio_num)) {
        printk("failed to get misc beep gpio!\r\n");
        return -ENODEV;
    }

    ret = gpio_request(beep_dev.gpio_num, "misc_beep");
	if (ret < 0) {
		printk("failed to request misc beep gpio!\r\n");
		goto failed_request;
	}

    ret = gpio_direction_output(beep_dev.gpio_num, BEEP_OFF);
	if (ret < 0) {
		printk("failed to set misc beep gpio direction!\r\n");
		goto failed_request;
	}

    //MISC驱动注册
    ret = misc_register(&misc_beep_device);
	if (ret) {
	    printk("failed to register misc beep\r\n");
        return -EFAULT;
	}

    return ret;

failed_request:
    gpio_free(beep_dev.gpio_num);

    return ret;
}

int misc_beep_remove(struct platform_device *dev)
{
    misc_deregister(&misc_beep_device);
    gpio_free(beep_dev.gpio_num);

    return 0;
}

static struct platform_driver misc_beep_dev = {
    .driver = {
        .name = "misc_beep",
        .of_match_table = misc_beep_match,
    },
    .probe  = misc_beep_probe,
    .remove = misc_beep_remove,
};

static int __init misc_beep_init(void)
{
    int ret;

    ret = platform_driver_register(&misc_beep_dev);

    return ret;
}

static void __exit misc_beep_exit(void)
{
    platform_driver_unregister(&misc_beep_dev);
}

module_init(misc_beep_init);
module_exit(misc_beep_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Alex_min");
