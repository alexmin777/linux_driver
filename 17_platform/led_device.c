#include <linux/module.h>
#include <linux/platform_device.h>

#define CCM_CCGR1_BASE              (0x020C406C)
#define SW_MUX_GPIO1_IO03_BASE      (0x020E0068)
#define SW_PAD_GPIO1_IO03_BASE      (0x020E02F4)
#define GPIO1_DR_BASE               (0x0209C000)
#define GPIO1_GDIR_BASE             (0x0209C004)
#define LED_REG_LENGTH              4

static void led_device_release(struct device *dev)
{
    printk("led_device_release\r\n");
}

static struct resource led_device_resources[] = {
	[0] = {
		.start = CCM_CCGR1_BASE,
		.end = CCM_CCGR1_BASE + LED_REG_LENGTH - 1,
		.flags = IORESOURCE_MEM,
	},
    [1] = {
		.start = SW_MUX_GPIO1_IO03_BASE,
		.end = SW_MUX_GPIO1_IO03_BASE + LED_REG_LENGTH - 1,
		.flags = IORESOURCE_MEM,
	},
    [2] = {
		.start = SW_PAD_GPIO1_IO03_BASE,
		.end = SW_PAD_GPIO1_IO03_BASE + LED_REG_LENGTH - 1,
		.flags = IORESOURCE_MEM,
	},
    [3] = {
		.start = GPIO1_DR_BASE,
		.end = GPIO1_DR_BASE + LED_REG_LENGTH - 1,
		.flags = IORESOURCE_MEM,
	},
    [4] = {
		.start = GPIO1_GDIR_BASE,
		.end = GPIO1_GDIR_BASE + LED_REG_LENGTH - 1,
		.flags = IORESOURCE_MEM,
	},
};

static struct platform_device led_device = {
	.name		= "alex-led",
    .id		= -1,
    .dev    = {
        .release = led_device_release,
    },
    .num_resources	= ARRAY_SIZE(led_device_resources),
	.resource	= led_device_resources,
};

static int __init led_device_init(void)
{
    int ret = 0;

    platform_device_register(&led_device);

    return ret;
}

static void __exit led_device_exit(void)
{
    platform_device_unregister(&led_device);
}

module_init(led_device_init);
module_exit(led_device_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Alex_min");