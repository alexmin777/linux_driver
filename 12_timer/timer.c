#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of_gpio.h>
#include <linux/uaccess.h>
#include <linux/types.h>

#define TIMER_NAME    "led"
#define TIMER_CNT     1
#define TIMER_MAGIC_BASE    'T'
#define CMD_OPEN    (_IO(TIMER_MAGIC_BASE, 0x01))
#define CMD_CLOSE   (_IO(TIMER_MAGIC_BASE, 0x02))
#define CMD_MODIFY  (_IOW(TIMER_MAGIC_BASE, 0x03, int))

struct timer_dev {
    dev_t dev_id;
    int major;
    int minor;
    struct cdev cdev_t;
    struct class *class_t;
    struct device *device_t;
    struct device_node *nd;
    int led_gpio;
    struct timer_list dev_timer;
    int period;
    spinlock_t lock;
};

static struct timer_dev timer;

void timer_handle(unsigned long data)
{
    struct timer_dev * dev = (struct timer_dev *)data;
    static int led_status = 1;
    int timer_period;
    unsigned long flags;

    led_status = !led_status;
    gpio_set_value(dev->led_gpio, led_status);

    spin_lock_irqsave(&dev->lock, flags);
    timer_period = dev->period;
    spin_unlock_irqrestore(&dev->lock, flags);
    mod_timer(&dev->dev_timer, jiffies + msecs_to_jiffies(timer_period));
}

static int timer_open (struct inode * inode, struct file *filp)
{
    filp->private_data = &timer;

    return 0;
}

long timer_ctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    unsigned long flags;
    int timer_period;
    struct timer_dev * dev = filp->private_data;
    int ret = 0;
    printk("timer_ctl...........\r\n");


    switch (cmd) {
        case CMD_OPEN:
            spin_lock_irqsave(&dev->lock, flags);
            timer_period = dev->period;
            spin_unlock_irqrestore(&dev->lock, flags);
            mod_timer(&dev->dev_timer, jiffies + msecs_to_jiffies(timer_period));
            break;
        case CMD_CLOSE:
            del_timer_sync(&dev->dev_timer);
            break;
        case CMD_MODIFY:
            spin_lock_irqsave(&dev->lock, flags);
            ret = copy_from_user(&timer_period, (int *)arg, sizeof(int));
            dev->period = timer_period;
            spin_unlock_irqrestore(&dev->lock, flags);
            printk("ioctl receive data:%d\r\n", timer_period);
            mod_timer(&dev->dev_timer, jiffies + msecs_to_jiffies(timer_period));
            break;
        default:
            ret =  -EPERM;     
    }
    return ret;
}

static int timer_release (struct inode *inode, struct file *filp)
{
    return 0;
}

static const struct file_operations timer_ops = {
    .owner = THIS_MODULE,
    .open = timer_open,
    .unlocked_ioctl = timer_ctl,
    .release = timer_release,
};

static int led_gpio_init(struct timer_dev *dev)
{
    int ret = 0;

    dev->nd = of_find_node_by_path("/gpioled");
	if (!dev->nd) {
		printk("node 'gpioled' not found\n");
		goto err_node;
	}

    dev->led_gpio = of_get_named_gpio(dev->nd, "led-gpios", 0);
    if (dev->led_gpio < 0) {
		goto err_gpio;
	}

    ret = gpio_request(dev->led_gpio, "led");
    if (ret != 0) {
        printk("Failed to request led GPIO: %d\n", ret);
        goto err_request;
    } else {
        printk("request led gpio num success:%d\n", dev->led_gpio);
    }

    ret = gpio_direction_output(dev->led_gpio, 1);
    if (ret) {
		printk("Failed to set LED GPIO direction\n");
	}

    gpio_set_value(dev->led_gpio, 1);

    return 0;

err_request:
    gpio_free(dev->led_gpio);
err_gpio:
err_node:

    return ret;
}

static int __init timer_init(void)
{
    int ret;

    timer.period = 500;
    spin_lock_init(&timer.lock);

    //初始化定时器
    init_timer(&timer.dev_timer);
    timer.dev_timer.function = timer_handle;
    timer.dev_timer.data = (unsigned long)&timer;

    timer.major = 0;

    //分配设备ID
    if (timer.major) {
        timer.dev_id = MKDEV(timer.major, 0);
        timer.minor = 0;
        register_chrdev_region(timer.dev_id, TIMER_CNT, TIMER_NAME);
    } else {
        alloc_chrdev_region(&timer.dev_id, 0, TIMER_CNT, TIMER_NAME);
        timer.major = MAJOR(timer.dev_id);
        timer.minor = MINOR(timer.dev_id);
    }
    printk("timer id:%d major:%d minor:%d\n", timer.dev_id, timer.major, timer.minor);

    //添加字符设备
    timer.cdev_t.owner = THIS_MODULE;
    cdev_init(&timer.cdev_t, &timer_ops);
    ret = cdev_add(&timer.cdev_t, timer.dev_id, TIMER_CNT);
	if (ret < 0) {
		printk("Could not add cdev (err %d)\n", -ret);
		goto err_cdev;
	}

    //添加设备节点
    timer.class_t = class_create(THIS_MODULE, TIMER_NAME);
	if (IS_ERR(timer.class_t)) {
		printk(KERN_ERR "Error creating timer class.\n");
        ret = PTR_ERR(timer.class_t);
		goto err_class;
	}
    timer.device_t = device_create(timer.class_t, NULL, timer.dev_id, NULL, TIMER_NAME);
	if (IS_ERR(timer.class_t)) {
		printk(KERN_ERR "Error creating timer device.\n");
        ret = PTR_ERR(timer.device_t);
		goto err_device;
	}

    ret = led_gpio_init(&timer);
    if (ret == 0) {
        printk("init led gpio success\r\n");
    } else {
        printk("init led gpio error!\r\n");
        goto err_init;
    }

    return 0;

err_init:
err_device:
    device_destroy(timer.class_t, timer.dev_id);
err_class:
    class_destroy(timer.class_t);
err_cdev:
    cdev_del(&timer.cdev_t);

    return ret;
}

static void __exit timer_exit(void)
{
    del_timer_sync(&timer.dev_timer);
    gpio_set_value(timer.led_gpio, 1);
    gpio_free(timer.led_gpio);
    device_destroy(timer.class_t, timer.dev_id);
    class_destroy(timer.class_t);
    cdev_del(&timer.cdev_t);
    unregister_chrdev_region(timer.dev_id, TIMER_CNT);
}

module_init(timer_init);
module_exit(timer_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Alex_min");
