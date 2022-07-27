#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define CHRDEVBASE_MAJPR    200 //主设备号
#define CHRDEVBASE_NAME "chrdevbase"    //设备名字

MODULE_LICENSE("Dual BSD/GPL");

static char DeviceName[100] = {"kernel device"};

static int chrdevbase_open (struct inode * inode, struct file *filp)
{
    printk("chrdevbase_open\r\n");

    return 0;
}

static int chrdevbase_release (struct inode *inode, struct file *filp)
{
    printk("chrdevbase_release\r\n");

    return 0;
}

static ssize_t chrdevbase_read (struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
    int ret = 0;

    printk("chrdevbase_read\r\n");

    ret = copy_to_user(buf, DeviceName, size);
    if (ret == 0) {
        printk("copy to user success\r\n");
    }
    else {
        printk("copy to user failed\r\n");
    }

    return 0;
}

static ssize_t chrdevbase_write (struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
    int ret = 0;

    printk("chrdevbase_write\r\n");

    ret = copy_from_user(DeviceName, buf, size);
    if (ret == 0) {
        printk("copy from user success\r\n");
    }
    else {
        printk("copy from user failed\r\n");
    }

    printk("now device name is %s\r\n", DeviceName);

    return 0;
}


static struct file_operations chrdevbase_fops = 
{
    .owner = THIS_MODULE,
    .open = chrdevbase_open,
    .release = chrdevbase_release,
    .read = chrdevbase_read,
    .write = chrdevbase_write,
};


static int hello_init(void)
{
    printk(KERN_ALERT "Hello, world\n");

    //注册字符设备
    register_chrdev(CHRDEVBASE_MAJPR, CHRDEVBASE_NAME, &chrdevbase_fops);

    return 0;
}

static void hello_exit(void)
{
    printk(KERN_ALERT "Goodbye, cruel world\n");

    //注销字符设备
    unregister_chrdev(CHRDEVBASE_MAJPR, CHRDEVBASE_NAME);
}


module_init(hello_init);
module_exit(hello_exit);
