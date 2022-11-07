#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <asm/io.h>
#include <linux/uaccess.h>

#define DEVICE_CNT  1
#define DEVICE_NAME "dtsled"

//寄存器虚拟地址
static void __iomem* CCM_CCGR1;
static void __iomem* SW_MUX_GPIO1_IO03;
static void __iomem* SW_PAD_GPIO1_IO03;
static void __iomem* GPIO1_DR;
static void __iomem* GPIO1_GDIR;
//LED switch status
#define LED_OFF 0
#define LED_ON 1

struct dtsled_dev
{
    dev_t dev_id;
    int major;
    int minor;
    struct cdev cdev_t;
    struct class *class_t;
    struct device *device_t;
    struct device_node *nd;
};

static struct dtsled_dev dtsled;

static void led_switch(u8 status)
{
    u32 val = 0;

    if (status == LED_ON) {
        val = readl(GPIO1_DR);
        val &= ~(1 << 3);
        writel(val, GPIO1_DR);
    }
    else if (status == LED_OFF) {
        val = readl(GPIO1_DR);
        val|= (1 << 3);
        writel(val, GPIO1_DR);
    }

}

static int dtsled_open (struct inode * inode, struct file *filp)
{
    filp->private_data = &dtsled;

    return 0;
}

static int dtsled_release (struct inode *inode, struct file *filp)
{
    return 0;
}

static ssize_t dtsled_write (struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
    u8 led_status;
    u8 ctl_buf[1];
    int ret = 0;

    ret = copy_from_user(ctl_buf, buf, size);
    if (ret < 0) {
        printk("led ctl failed!\r\n");
        return -EFAULT;
    }

    led_status = ctl_buf[0];

    led_switch(led_status);

    return 0;
}

static const struct file_operations dtsled_ops = {
    .owner = THIS_MODULE,
    .open = dtsled_open,
    .release = dtsled_release,
    .write = dtsled_write,
};

static inline void led_address_unmap(void)
{
    iounmap(CCM_CCGR1);
    iounmap(SW_MUX_GPIO1_IO03);
    iounmap(SW_PAD_GPIO1_IO03);
    iounmap(GPIO1_DR);
    iounmap(GPIO1_GDIR);
}


static int __init dtsled_init(void)
{
    struct property *proper;
    const char *status_name;
    u32 reg_data[14];
    u32 val = 0;
    int i;
    int ret = 0;
    dtsled.major = 0;

    /* 获取设备节点 */
    dtsled.nd = of_find_node_by_path("/alphaled");
	if (dtsled.nd == NULL) {
		printk("find dtsled node failed!\r\n");
		ret = -EINVAL;
	} else {
		printk("%s node has been found!\r\n", dtsled.nd->name);
	}

    /* 获取compatible属性内容 */
    proper = of_find_property(dtsled.nd, "compatible", NULL);
    if (proper == NULL) {
		printk("can't find compatible property\r\n");
		ret = -ENOENT;
	} else {
		printk("%s compatible is %s\r\n", dtsled.nd->name, (char *)proper->value);
    }

    /* 获取status属性内容 */
    ret = of_property_read_string(dtsled.nd, "status", &status_name);
    if (ret < 0) {
		printk("read status string failed\r\n");
	} else {
		printk("read status string:%s\r\n", status_name);
	}

    /* 获取reg属性内容 */
    ret = of_property_read_u32_array(dtsled.nd, "reg", reg_data, 10);
    if (ret < 0) {
        printk("read reg data failed\r\n");
    } else {
        printk("reg data:");
        for (i = 0; i < 10; i++) {
            printk("%#x ", reg_data[i]);
        }
        printk("\r\n");
    }

    /* 寄存器地址映射 */
#if 1
    CCM_CCGR1 = ioremap(reg_data[0], reg_data[1]);
    SW_MUX_GPIO1_IO03 = ioremap(reg_data[2], reg_data[3]);
    SW_PAD_GPIO1_IO03 = ioremap(reg_data[4], reg_data[5]);
    GPIO1_DR = ioremap(reg_data[6], reg_data[7]);
    GPIO1_GDIR = ioremap(reg_data[8], reg_data[9]);
#else
    CCM_CCGR1 = of_iomap();
#endif
    //enable gpio1 clk
    val = readl(CCM_CCGR1);
    val &= ~(3 << 26);
    val |= (3 << 26);
    writel(val, CCM_CCGR1);
    //multiplexed as gpio
    writel(5, SW_MUX_GPIO1_IO03);
    //config gpio
    writel(0x10B0, SW_PAD_GPIO1_IO03);
    //config gpio1 as out put
    val = readl(GPIO1_GDIR);
    val &= ~(1 << 3);
    val |= (1 << 3);
    writel(val, GPIO1_GDIR);
    //turn off led
    val = readl(GPIO1_DR);
    val |= (1 << 3);
    writel(val, GPIO1_DR);

    /* 向内核申请设备号 */
    if (dtsled.major) {     /* 指定主设备号 */
        dtsled.dev_id = MKDEV(dtsled.major, 0);
        ret = register_chrdev_region(dtsled.dev_id, DEVICE_CNT, DEVICE_NAME);
        dtsled.minor = 0;
    } else {        /* 未指定主设备号 */
        ret = alloc_chrdev_region(&dtsled.dev_id, 0, DEVICE_CNT, DEVICE_NAME);
        dtsled.major = MAJOR(dtsled.dev_id);
        dtsled.minor = MINOR(dtsled.dev_id);
    }
    if (ret < 0)
        goto fail_dev;
    
    /* 向内核注册设备 */
    dtsled.cdev_t.owner = THIS_MODULE;
    cdev_init(&dtsled.cdev_t, &dtsled_ops);
    ret = cdev_add(&dtsled.cdev_t, dtsled.dev_id, DEVICE_CNT);
    if (ret < 0) {
        printk("add cdev error\r\n");
        goto fail_cdev;
    }

    /* 自动创建设备节点 */
    dtsled.class_t = class_create(dtsled.cdev_t.owner, DEVICE_NAME);
    if (IS_ERR(dtsled.class_t)) {
        printk("add create class error\r\n");
        goto fail_class;
    }
    dtsled.device_t = device_create(dtsled.class_t, NULL, dtsled.dev_id, NULL, DEVICE_NAME);
    if (IS_ERR(dtsled.device_t)) {
        printk("add create device node error\r\n");
        goto fail_node;
    }

    return 0;

fail_node:
    class_destroy(dtsled.class_t);
fail_class:
    cdev_del(&dtsled.cdev_t);
fail_cdev:
    unregister_chrdev_region(dtsled.dev_id, DEVICE_CNT);
fail_dev:

    return ret;
}

static void __exit dtsled_exit(void)
{
    //取消LED地址映射
    led_address_unmap();

    /* 删除设备节点 */
    device_destroy(dtsled.class_t, dtsled.dev_id);
    class_destroy(dtsled.class_t);

    /* 删除字符设备 */
    cdev_del(&dtsled.cdev_t);

    /* 释放设备号 */
    unregister_chrdev_region(dtsled.dev_id, DEVICE_CNT);
}

module_init(dtsled_init);
module_exit(dtsled_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Alex_min");