#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>

#define BEEP_CNT     1
#define BEEP_NAME    "beep"
#define BEEP_ON          1
#define BEEP_OFF         0

struct beep_dev {
    dev_t devID;
    int major;
    int minor;
    struct cdev cdev_t;
    struct class *class_t;
    struct device *device_t;
    struct device_node *nd;
    int led_gpio;
};

struct beep_dev beep;

static int beep_open (struct inode * inode, struct file *filp)
{
    filp->private_data = &beep;

    return 0;
}

static int beep_release (struct inode *inode, struct file *filp)
{
    return 0;
}

static ssize_t beep_write (struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
    struct beep_dev *dev;
    char databuf[1];

    dev = (struct beep_dev*)filp->private_data;

    if (copy_from_user(databuf, buf, size))
        return -EFAULT;

    if (BEEP_ON == databuf[0]) {
        gpio_set_value(dev->led_gpio, 0);
        printk("turn on BEEP\r\n");
    } else if (BEEP_OFF == databuf[0]) {
        gpio_set_value(dev->led_gpio, 1);
        printk("turn off BEEP\r\n");
    } else {
        printk("please entry 1/0 to control BEEP\r\n");
    }


    return 0;
}

static const struct file_operations beep_ops = {
    .owner = THIS_MODULE,
    .write = beep_write,
    .open = beep_open,
    .release = beep_release,
};


static int __init beep_init(void)
{
    int ret = 0;

    beep.major = 0;

    /* 分配设备号 */
    if (beep.major) {
        beep.devID = MKDEV(beep.major, 0);
        beep.minor = 0;
        register_chrdev_region(beep.devID, BEEP_CNT, BEEP_NAME);
    } else {
        alloc_chrdev_region(&beep.devID, 0, BEEP_CNT, BEEP_NAME);
        beep.major = MAJOR(beep.devID);
        beep.minor = MINOR(beep.devID);
    }
    printk("beep ID:%d %d\n", beep.major, beep.minor);

    /* 添加字符设备 */
    beep.cdev_t.owner = THIS_MODULE;
    cdev_init(&beep.cdev_t, &beep_ops);
    cdev_add(&beep.cdev_t, beep.devID, BEEP_CNT);

    /* 添加设备节点 */
    beep.class_t = class_create(THIS_MODULE, BEEP_NAME);
    if (IS_ERR(beep.class_t)) {
		ret = PTR_ERR(beep.class_t);
	}
    beep.device_t = device_create(beep.class_t, NULL, beep.devID, NULL, BEEP_NAME);
    if (IS_ERR(beep.device_t)) {
		ret = PTR_ERR(beep.device_t);
	}

    /* 获取设备节点 */
    beep.nd = of_find_node_by_path("/beep");
	if (!beep.nd) {
		printk("node 'beep' not found\n");
		return -ENODEV;
	}

    beep.led_gpio = of_get_named_gpio(beep.nd, "beep-gpios", 0);
    if (beep.led_gpio < 0) {
		return -ENODEV;
	}

    ret = gpio_request(beep.led_gpio, "beep-gpio");
    if (ret != 0) {
        gpio_free(beep.led_gpio);
        printk("Failed to request LED GPIO: %d\n", ret);
    } else {
        printk("request gpio num success:%d\n", beep.led_gpio);
    }

    ret = gpio_direction_output(beep.led_gpio, 1);
    if (ret) {
		printk("Failed to set LED GPIO direction\n");
	}

    gpio_set_value(beep.led_gpio, 1);

    return ret;
}

static void __exit beep_exit(void)
{
    gpio_set_value(beep.led_gpio, 1);
    gpio_free(beep.led_gpio);
    device_destroy(beep.class_t, beep.devID);
    class_destroy(beep.class_t);
    cdev_del(&beep.cdev_t);
    unregister_chrdev_region(beep.devID, BEEP_CNT);
}

module_init(beep_init);
module_exit(beep_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Alex_min");