#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of_gpio.h>
#include <linux/uaccess.h>

#define KEY_NAME    "key"
#define KEY_CNT     1
#define KEY_VALUE   0xf0
#define INVALID_VALUE   0x00

struct key_dev {
    dev_t dev_id;
    int major;
    int minor;
    struct cdev cdev_t;
    struct class *class_t;
    struct device *device_t;
    struct device_node *nd;
    int led_gpio;
    atomic_t value;
};

static struct key_dev key;

static int key_open (struct inode * inode, struct file *filp)
{
    filp->private_data = &key;

    return 0;
}

static ssize_t key_read (struct file *filp, char __user * buf, size_t size, loff_t *ppos)
{
    int value;
    struct key_dev *dev = (struct key_dev *)filp->private_data;
    int ret = 0;

    if (gpio_get_value(dev->key_gpio) == 0) {
        while (!gpio_get_value(dev->key_gpio));
        atomic_set(&dev->value, KEY_VALUE);
    } else {
        atomic_set(&dev->value, INVALID_VALUE);
    }

    value = atomic_read(&dev->value);

    ret = copy_to_user(buf, &value, sizeof(value));

    return ret;
}

static int key_release (struct inode *inode, struct file *filp)
{
    return 0;
}

static const struct file_operations key_ops = {
    .owner = THIS_MODULE,
    .read = key_read,
    .open = key_open,
    .release = key_release,
};

static int __init key_init(void)
{
    int ret;

    key.major = 0;

    //分配设备ID
    if (key.major) {
        key.dev_id = MKDEV(key.major, 0);
        key.minor = 0;
        register_chrdev_region(key.dev_id, KEY_CNT, KEY_NAME);
    } else {
        alloc_chrdev_region(&key.dev_id, 0, KEY_CNT, KEY_NAME);
        key.major = MAJOR(key.dev_id);
        key.minor = MINOR(key.dev_id);
    }
    printk("key id:%d major:%d minor:%d\n", key.dev_id, key.major, key.minor);

    //添加字符设备
    key.cdev_t.owner = THIS_MODULE;
    cdev_init(&key.cdev_t, &key_ops);
    ret = cdev_add(&key.cdev_t, key.dev_id, KEY_CNT);
	if (ret < 0) {
		printk("Could not add cdev (err %d)\n", -ret);
		goto err_cdev;
	}

    //添加设备节点
    key.class_t = class_create(THIS_MODULE, KEY_NAME);
	if (IS_ERR(key.class_t)) {
		printk(KERN_ERR "Error creating key class.\n");
        ret = PTR_ERR(key.class_t);
		goto err_class;
	}
    key.device_t = device_create(key.class_t, NULL, key.dev_id, NULL, KEY_NAME);
	if (IS_ERR(key.class_t)) {
		printk(KERN_ERR "Error creating key device.\n");
        ret = PTR_ERR(key.device_t);
		goto err_device;
	}

    key.nd = of_find_node_by_path("/key");
	if (!key.nd) {
		printk("node 'beep' not found\n");
		goto err_node;
	}

    key.key_gpio = of_get_named_gpio(key.nd, "key-gpios", 0);
    if (key.key_gpio < 0) {
		goto err_gpio;
	}

    ret = gpio_request(key.key_gpio, "key");
    if (ret != 0) {
        printk("Failed to request KEY GPIO: %d\n", ret);
        goto err_request;
    } else {
        printk("request gpio num success:%d\n", key.key_gpio);
    }

    ret = gpio_direction_input(key.key_gpio);
    if (ret) {
		printk("Failed to set KEY GPIO direction\n");
	}

    return 0;

err_request:
    gpio_free(key.key_gpio);
err_gpio:
err_node:
err_device:
    device_destroy(key.class_t, key.dev_id);
err_class:
    class_destroy(key.class_t);
err_cdev:
    cdev_del(&key.cdev_t);

    return ret;
}

static void __exit key_exit(void)
{
    gpio_free(key.key_gpio);
    device_destroy(key.class_t, key.dev_id);
    class_destroy(key.class_t);
    cdev_del(&key.cdev_t);
    unregister_chrdev_region(key.dev_id, KEY_CNT);
}

module_init(key_init);
module_exit(key_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Alex_min");
