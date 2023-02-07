#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of_gpio.h>
#include <linux/uaccess.h>
#include <linux/types.h>

#define IRQ_NAME    "key_irq"
#define IRQ_CNT     1

struct key_describe {
    struct device_node *nd;
    int gpio_num;
    int iqr_num;
};

struct key {
    dev_t dev_id;
    int major;
    int minor;
    struct cdev cdev_t;
    struct class *class_t;
    struct device *device_t;
    struct key_describe key_des;
};

static struct key key_dev;

static int key_open (struct inode * inode, struct file *filp)
{
    filp->private_data = &key_dev;

    return 0;
}

long key_ctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    return 0;
}

static int key_release (struct inode *inode, struct file *filp)
{
    return 0;
}

static const struct file_operations timer_ops = {
    .owner = THIS_MODULE,
    .open = key_open,
    .unlocked_ioctl = key_ctl,
    .release = key_release,
};

static int key_gpio_init(struct key *dev)
{
    int ret;

    dev->key_des.nd = of_find_node_by_path("/key");
	if (!dev->key_des.nd) {
		printk("node 'key' not found\n");
        ret = -EINVAL;
		goto err_node;
	}

    dev->key_des.gpio_num = of_get_named_gpio(dev->key_des.nd, "key-gpios", 0);
    if (dev->key_des.gpio_num < 0) {
        ret = -EINVAL;
        goto err_gpio;
    }

    ret = gpio_request(dev->key_des.gpio_num, "key_irq");
	if (ret) {
		printk("failed to request GPIO for KEY \n");
        goto err_request;
	}

    ret = gpio_direction_input(dev->key_des.gpio_num);
    if (ret) {
		printk("Failed to set KEY GPIO direction\n");
        goto err_dir;
	}


    return 0;

err_dir:
err_request:
    gpio_free(dev->key_des.gpio_num);
err_gpio:
err_node:

    return ret;
}

static int __init irq_init(void)
{
    int ret;

    key_dev.major = 0;

    //分配设备ID
    if (key_dev.major) {
        key_dev.dev_id = MKDEV(key_dev.major, 0);
        key_dev.minor = 0;
        register_chrdev_region(key_dev.dev_id, IRQ_CNT, IRQ_NAME);
    } else {
        alloc_chrdev_region(&key_dev.dev_id, 0, IRQ_CNT, IRQ_NAME);
        key_dev.major = MAJOR(key_dev.dev_id);
        key_dev.minor = MINOR(key_dev.dev_id);
    }
    printk("key_dev id:%d major:%d minor:%d\n", key_dev.dev_id, key_dev.major, key_dev.minor);

    //添加字符设备
    key_dev.cdev_t.owner = THIS_MODULE;
    cdev_init(&key_dev.cdev_t, &timer_ops);
    ret = cdev_add(&key_dev.cdev_t, key_dev.dev_id, IRQ_CNT);
	if (ret < 0) {
		printk("Could not add cdev (err %d)\n", -ret);
		goto err_cdev;
	}

    //添加设备节点
    key_dev.class_t = class_create(THIS_MODULE, IRQ_NAME);
	if (IS_ERR(key_dev.class_t)) {
		printk(KERN_ERR "Error creating key_dev class.\n");
        ret = PTR_ERR(key_dev.class_t);
		goto err_class;
	}
    key_dev.device_t = device_create(key_dev.class_t, NULL, key_dev.dev_id, NULL, IRQ_NAME);
	if (IS_ERR(key_dev.class_t)) {
		printk(KERN_ERR "Error creating key_dev device.\n");
        ret = PTR_ERR(key_dev.device_t);
		goto err_device;
	}

    ret = key_gpio_init(&key_dev);
    if (ret == 0) {
        printk("init led gpio success\r\n");
    } else {
        printk("init led gpio error!\r\n");
        goto err_init;
    }

    return 0;

err_init:
err_device:
    device_destroy(key_dev.class_t, key_dev.dev_id);
err_class:
    class_destroy(key_dev.class_t);
err_cdev:
    cdev_del(&key_dev.cdev_t);

    return ret;
}

static void __exit irq_exit(void)
{
    gpio_free(key_dev.key_des.gpio_num);
    device_destroy(key_dev.class_t, key_dev.dev_id);
    class_destroy(key_dev.class_t);
    cdev_del(&key_dev.cdev_t);
    unregister_chrdev_region(key_dev.dev_id, IRQ_CNT);
}

module_init(irq_init);
module_exit(irq_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Alex_min");
