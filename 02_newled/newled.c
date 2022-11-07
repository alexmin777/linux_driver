#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/io.h>

#define NEWCHRLED_CNT			1
#define NEWCHRLED_NAME			"newled"
//寄存器物理地址
#define CCM_CCGR1_BASE (0x020C406C)
#define SW_MUX_GPIO1_IO03_BASE (0x020E0068)
#define SW_PAD_GPIO1_IO03_BASE (0x020E02F4)
#define GPIO1_DR_BASE (0x0209C000)
#define GPIO1_GDIR_BASE (0x0209C004)
//寄存器虚拟地址
static void __iomem* CCM_CCGR1;
static void __iomem* SW_MUX_GPIO1_IO03;
static void __iomem* SW_PAD_GPIO1_IO03;
static void __iomem* GPIO1_DR;
static void __iomem* GPIO1_GDIR;
//LED switch status
#define LED_OFF 0
#define LED_ON 1


struct newled_dev {
    dev_t devID;
    int major;
    int minor;
    struct cdev cdev_t;
    struct class *class_t;
    struct device *device_t;
};

struct newled_dev new_led;

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

static const struct file_operations new_led_fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .release = led_release,
    .write = led_write,
};

static inline void led_address_map(void)
{
    CCM_CCGR1 = ioremap(CCM_CCGR1_BASE, 4);
    SW_MUX_GPIO1_IO03 = ioremap(SW_MUX_GPIO1_IO03_BASE, 4);
    SW_PAD_GPIO1_IO03 = ioremap(SW_PAD_GPIO1_IO03_BASE, 4);
    GPIO1_DR = ioremap(GPIO1_DR_BASE, 4);
    GPIO1_GDIR = ioremap(GPIO1_GDIR_BASE, 4);
}

static inline void led_address_unmap(void)
{
    iounmap(CCM_CCGR1);
    iounmap(SW_MUX_GPIO1_IO03);
    iounmap(SW_PAD_GPIO1_IO03);
    iounmap(GPIO1_DR);
    iounmap(GPIO1_GDIR);
}

static int __init new_led_init(void)
{
    u32 val = 0;

    printk("new_led_init\r\n");

    //分配设备号
    if (new_led.major > 0) {
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

    //地址映射
    led_address_map();
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

    return 0;
}

static void __exit new_led_exit(void)
{
    printk("new_led_exit");

    //取消LED地址映射
    led_address_unmap();

    //删除设备节点
    device_destroy(new_led.class_t, new_led.devID);
    class_destroy(new_led.class_t);

    //删除字符设备
    cdev_del(&new_led.cdev_t);

    //释放设备号
    unregister_chrdev_region(new_led.devID, NEWCHRLED_CNT);
}

module_init(new_led_init);
module_exit(new_led_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Alex_min");