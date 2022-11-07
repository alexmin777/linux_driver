#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>
#include <linux/types.h>

#define GPIOLED_CNT     1
#define GPIOLED_NAME    "gpioled"
#define LED_ON          1
#define LED_OFF         0

struct gpioled_dev {
    dev_t devID;
    int major;
    int minor;
    struct cdev cdev_t;
    struct class *class_t;
    struct device *device_t;
    struct device_node *nd;
    int led_gpio;
    struct semaphore sem;
};

struct gpioled_dev gpioled;

static int gpioled_open (struct inode * inode, struct file *filp)
{
    filp->private_data = &gpioled;

    down(&gpioled.sem);

    return 0;
}

static int gpioled_release (struct inode *inode, struct file *filp)
{
    struct gpioled_dev *dev;

    dev = (struct gpioled_dev*)filp->private_data;

    up(&dev->sem);

    return 0;
}

static ssize_t gpioled_write (struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
    return 0;
}

static const struct file_operations gpioled_ops = {
    .owner = THIS_MODULE,
    .write = gpioled_write,
    .open = gpioled_open,
    .release = gpioled_release,
};


static int __init gpioled_init(void)
{
    int ret = 0;

    sema_init(&gpioled.sem, 2);

    gpioled.major = 0;

    /* 分配设备号 */
    if (gpioled.major) {
        gpioled.devID = MKDEV(gpioled.major, 0);
        gpioled.minor = 0;
        register_chrdev_region(gpioled.devID, GPIOLED_CNT, GPIOLED_NAME);
    } else {
        alloc_chrdev_region(&gpioled.devID, 0, GPIOLED_CNT, GPIOLED_NAME);
        gpioled.major = MAJOR(gpioled.devID);
        gpioled.minor = MINOR(gpioled.devID);
    }
    printk("gpioled ID:%d %d\n", gpioled.major, gpioled.minor);

    /* 添加字符设备 */
    gpioled.cdev_t.owner = THIS_MODULE;
    cdev_init(&gpioled.cdev_t, &gpioled_ops);
    cdev_add(&gpioled.cdev_t, gpioled.devID, GPIOLED_CNT);

    /* 添加设备节点 */
    gpioled.class_t = class_create(THIS_MODULE, GPIOLED_NAME);
    if (IS_ERR(gpioled.class_t)) {
		ret = PTR_ERR(gpioled.class_t);
	}
    gpioled.device_t = device_create(gpioled.class_t, NULL, gpioled.devID, NULL, GPIOLED_NAME);
    if (IS_ERR(gpioled.device_t)) {
		ret = PTR_ERR(gpioled.device_t);
	}

    /* 获取设备节点 */
    gpioled.nd = of_find_node_by_path("/gpioled");
	if (!gpioled.nd) {
		printk("node 'gpioled' not found\n");
		return -ENODEV;
	}

    gpioled.led_gpio = of_get_named_gpio(gpioled.nd, "led-gpios", 0);
    if (gpioled.led_gpio < 0) {
		return -ENODEV;
	}

    ret = gpio_request(gpioled.led_gpio, "led-gpio");
    if (ret != 0) {
        gpio_free(gpioled.led_gpio);
        printk("Failed to request LED GPIO: %d\n", ret);
    } else {
        printk("request gpio num success:%d\n", gpioled.led_gpio);
    }

    ret = gpio_direction_output(gpioled.led_gpio, 1);
    if (ret) {
		printk("Failed to set LED GPIO direction\n");
	}

    gpio_set_value(gpioled.led_gpio, 1);

    return ret;
}

static void __exit gpioled_exit(void)
{
    gpio_set_value(gpioled.led_gpio, 1);
    gpio_free(gpioled.led_gpio);
    device_destroy(gpioled.class_t, gpioled.devID);
    class_destroy(gpioled.class_t);
    cdev_del(&gpioled.cdev_t);
    unregister_chrdev_region(gpioled.devID, GPIOLED_CNT);
}

module_init(gpioled_init);
module_exit(gpioled_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Alex_min");