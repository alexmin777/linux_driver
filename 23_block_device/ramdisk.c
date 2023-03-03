#include <linux/module.h>
#include <linux/fs.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/spinlock.h>

#define BLOCK_NAME     "ramdisk"
#define BLOCK_MINOR     3
#define RAMDISK_SIZE      (2*1024*1024)
#define SECTION_PRE_SIZE    512

struct  ramdisk_dev {
    int major;
    unsigned char *ramdisk_buff;
    struct gendisk *gendisk;
    spinlock_t lock;
    struct request_queue *request_queue;
};

static struct  ramdisk_dev ramdisk;

struct block_device_operations ramdisk_ops = {

};

static void ramdisk_request_fn_proc(struct request_queue *q)
{

}

static int __init ramdisk_init(void)
{
    int ret;

    ramdisk.ramdisk_buff = kzalloc(RAMDISK_SIZE, GFP_KERNEL);
    if(!ramdisk.ramdisk_buff) {
        printk("alloc ramdisk memory failed\n");
        return -EBUSY;
    }

    ramdisk.major = register_blkdev(0, BLOCK_NAME);
    if (ramdisk.major < 0)  {
        printk("register ramdisk failed!\r\n");
        ret = -EBUSY;
        goto regiser_blk_failed;
    }

    ramdisk.gendisk = alloc_disk(ramdisk.major);
    if (!ramdisk.gendisk) {
		printk("alloc gendisk failed\r\n");
        ret = ENOSPC;
        goto alloc_disk_failed;
	}

    spin_lock_init(&ramdisk.lock);
    ramdisk.request_queue = blk_init_queue(ramdisk_request_fn_proc, &ramdisk.lock);
    if (!ramdisk.request_queue) {
		printk("init block request queue failed\r\n");
        ret = EBUSY;
        goto init_queue_failed;
    }

    ramdisk.gendisk->major          = ramdisk.major;
    ramdisk.gendisk->first_minor    = 0;
    ramdisk.gendisk->minors         = BLOCK_MINOR;
    ramdisk.gendisk->queue          = ramdisk.request_queue;
    ramdisk.gendisk->private_data   = &ramdisk;
    ramdisk.gendisk->fops           = &ramdisk_ops;
    sprintf(ramdisk.gendisk->disk_name, BLOCK_NAME);

    set_capacity(ramdisk.gendisk, RAMDISK_SIZE / SECTION_PRE_SIZE);
    add_disk(ramdisk.gendisk);

    return 0;

init_queue_failed:
    del_gendisk(ramdisk.gendisk);
alloc_disk_failed:
    unregister_blkdev(ramdisk.major, BLOCK_NAME);
regiser_blk_failed:
    kfree(ramdisk.ramdisk_buff);

    return ret;
}

static void __exit ramdisk_exit(void)
{
    kfree(ramdisk.ramdisk_buff);
    blk_cleanup_queue(ramdisk.request_queue);
    del_gendisk(ramdisk.gendisk);
    unregister_blkdev(ramdisk.major, BLOCK_NAME);

    return;
}

module_init(ramdisk_init);
module_exit(ramdisk_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Alex_min");