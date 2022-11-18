#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/io.h>

#define NEWCHRLED_CNT			1
#define NEWCHRLED_NAME			"platform_led"
//LED switch status
#define LED_OFF 0
#define LED_ON 1

//寄存器虚拟地址
static void __iomem* CCM_CCGR1;
static void __iomem* SW_MUX_GPIO1_IO03;
static void __iomem* SW_PAD_GPIO1_IO03;
static void __iomem* GPIO1_DR;
static void __iomem* GPIO1_GDIR;

//LED设备结构体
struct newled_dev {
    dev_t devID;
    int major;
    int minor;
    struct cdev cdev_t;
    struct class *class_t;
    struct device *device_t;
};

static struct newled_dev new_led;

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

static int led_open (struct inode * inode, struct file *filp)
{
    return 0;
}

static int led_release (struct inode *inode, struct file *filp)
{
    return 0;
}

static ssize_t led_write (struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
    u8 ctl_buf[1] = {0};
    u8 led_status;
    int ret = 0;

    ret = copy_from_user(ctl_buf, buf, 1);
    if (ret < 0) {
        printk("led ctl failed!\r\n");
        return -EFAULT;
    }

    led_status = ctl_buf[0];

    led_switch(led_status); 

    return 0;
}

//LED设备操作
static const struct file_operations new_led_fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .release = led_release,
    .write = led_write,
};

//设备或驱动注销后，执行此函数
static int led_remove(struct platform_device *dev)
{
	printk("led_remove...\r\n");

	//退出前关闭LED
	led_switch(LED_OFF);
	//取消地址印射
    iounmap(CCM_CCGR1);
    iounmap(SW_MUX_GPIO1_IO03);
    iounmap(SW_PAD_GPIO1_IO03);
    iounmap(GPIO1_DR);
    iounmap(GPIO1_GDIR);

	//删除设备节点
    device_destroy(new_led.class_t, new_led.devID);
    class_destroy(new_led.class_t);

    //删除字符设备
    cdev_del(&new_led.cdev_t);

    //释放设备号
    unregister_chrdev_region(new_led.devID, NEWCHRLED_CNT);

	return 0;
}

//设备和驱动匹配后，执行此函数
static int led_probe(struct platform_device *dev)
{
	int i, val;
	int size_of_resource[5];
	struct resource *ledsource[5];

	printk("led_probe...\r\n");
	//获取资源
	for (i = 0; i < 5; i++) {
		ledsource[i] = platform_get_resource(dev, IORESOURCE_MEM, i);
		if (!ledsource[i]) {
			dev_err(&dev->dev, "No MEM resource for always on\n");
			return -ENXIO;
		}
		size_of_resource[i] = resource_size(ledsource[i]);
	}

	//寄存器地址映射
	CCM_CCGR1 = ioremap(ledsource[0]->start, size_of_resource[0]);
    SW_MUX_GPIO1_IO03 = ioremap(ledsource[1]->start, size_of_resource[1]);
    SW_PAD_GPIO1_IO03 = ioremap(ledsource[2]->start, size_of_resource[2]);
    GPIO1_DR = ioremap(ledsource[3]->start, size_of_resource[3]);
    GPIO1_GDIR = ioremap(ledsource[4]->start, size_of_resource[4]);

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

    //分配设备号
	new_led.major = 0;
    if (new_led.major) {
        new_led.devID = MKDEV(new_led.major, 0);
        register_chrdev_region(new_led.devID, NEWCHRLED_CNT, NEWCHRLED_NAME);
        new_led.minor = 0;
    }
    else {
        alloc_chrdev_region(&new_led.devID, 0, NEWCHRLED_CNT, NEWCHRLED_NAME);
        new_led.major = MAJOR(new_led.devID);
        new_led.minor  = MINOR(new_led.devID);
    }
    printk("Device ID:%x, major:%x, minor:%x\r\n", new_led.devID, new_led.major, new_led.minor);

    //注册字符设备
    cdev_init(&new_led.cdev_t, &new_led_fops);
    cdev_add(&new_led.cdev_t, new_led.devID, NEWCHRLED_CNT);

    //创建设备节点
    new_led.class_t = class_create(new_led.cdev_t.owner, NEWCHRLED_NAME);
    new_led.device_t = device_create(new_led.class_t, NULL, new_led.devID, NULL, NEWCHRLED_NAME);

	//默认关闭LED
	led_switch(LED_OFF);

	return 0;
}

static struct platform_driver led_driver = {
	.probe	= led_probe,
	.remove	= led_remove,
	.driver		= {
		.name	= "alex-led",
	},
};

static int __init led_driver_init(void)
{
    int ret = 0;

    platform_driver_register(&led_driver);

    return ret;
}

static void __exit led_driver_exit(void)
{
    platform_driver_unregister(&led_driver);
}

module_init(led_driver_init);
module_exit(led_driver_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Alex_min");