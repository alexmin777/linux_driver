#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/io.h>
#include <linux/uaccess.h>

#define LED_MAJPR    200 //主设备号
#define LED_NAME "led"    //设备名字
//LED switch status
#define LED_OFF 0
#define LED_ON 1
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

static void led_switch(u8 status)
{
    u32 val = 0;

    printk("led status:%d\r\n", status);

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
    //printk("open led\r\n");

    return 0;
}

static int led_release (struct inode *inode, struct file *filp)
{
    //printk("close led\r\n");

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

static const struct file_operations led_fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .release = led_release,
    .write = led_write,
};

static int __init led_init(void)
{
    u32 val = 0;
    int ret = 0;

    printk("led init\r\n");

    //注册LED设备
    ret = register_chrdev(LED_MAJPR, LED_NAME, &led_fops);
    if (ret < 0) {
        printk("register led failed!\r\n");
        return -EIO;
    }

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

static void __exit led_exit(void)
{
    printk("led exit\n");

    led_address_unmap();
    //注销LED设备
    unregister_chrdev(LED_MAJPR, LED_NAME);
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Alex_min");
