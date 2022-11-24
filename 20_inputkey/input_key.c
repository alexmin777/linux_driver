#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>
#include <linux/of_irq.h>
#include <linux/input.h>

struct input_des {
    int gpio_num;
    int irq_num;
    struct device_node *nd;
    struct timer_list timer_t;
    struct input_dev *key_input_dev;
};

static struct input_des input_key_dev;

static void input_key_timer_handle(unsigned long data)
{
    int value;
    struct input_des *dev = (struct input_des *)data;

    value = gpio_get_value(dev->gpio_num);
    printk("timer %d...\r\n", value);

    
    input_event(dev->key_input_dev, EV_KEY, BTN_0, !value);
    input_sync(dev->key_input_dev);
#if 0
    if (0 == value) {
        input_event(dev->key_input_dev, EV_KEY, BTN_0, 1);
        input_sync(dev->key_input_dev);
    } else {
        input_event(dev->key_input_dev, EV_KEY, BTN_0, 0);
        input_sync(dev->key_input_dev);
    }
#endif
}

static irqreturn_t input_key_irq_handle(int irq, void *data)
{
    struct input_des *dev = (struct input_des *)data;
    mod_timer(&dev->timer_t, jiffies + msecs_to_jiffies(20));

    return IRQ_RETVAL(IRQ_HANDLED);
}

static int key_io_init(void)
{
    int ret;

    input_key_dev.nd = of_find_node_by_path("/key");
    if (!input_key_dev.nd) {
		printk("could not find key node\n");
		ret = -ENOENT;
	}

    input_key_dev.gpio_num = of_get_named_gpio(input_key_dev.nd, "key-gpios", 0);
    if (!gpio_is_valid(input_key_dev.gpio_num)) {
		printk("no key input pin available\\r\n");
		return -ENODEV;
	}

	ret = gpio_request(input_key_dev.gpio_num, "input-key");
	if (ret) {
		printk("can not open intput key GPIO\r\n");
		return -EBUSY;
	}

    gpio_direction_input(input_key_dev.gpio_num);
    input_key_dev.irq_num = irq_of_parse_and_map(input_key_dev.nd, 0);
    ret = request_irq(input_key_dev.irq_num, input_key_irq_handle, IRQ_TYPE_EDGE_BOTH, "input-key-irq", (void*)&input_key_dev);
    if (ret) {
        printk("requset iqr failed\r\n");
        return -EINVAL;
    }

    return 0;
}

static int __init input_key_init(void)
{
    int ret;

    init_timer(&input_key_dev.timer_t);
    input_key_dev.timer_t.function = input_key_timer_handle;
    input_key_dev.timer_t.data = (unsigned long)&input_key_dev;

    ret = key_io_init();
    if (ret) {
        printk("init input key gpio failed!\r\n");
        return -EINVAL;
    }
    printk("key_io_init success!\r\n");

    input_key_dev.key_input_dev = input_allocate_device();
	if (!input_key_dev.key_input_dev)
		return -ENOMEM;
    input_key_dev.key_input_dev->name = "key-input";
    __set_bit(EV_KEY, input_key_dev.key_input_dev->evbit);
	__set_bit(EV_REP, input_key_dev.key_input_dev->evbit);
	__set_bit(BTN_0, input_key_dev.key_input_dev->keybit);

 	ret = input_register_device(input_key_dev.key_input_dev);
	if (ret)
		input_free_device(input_key_dev.key_input_dev);

    return 0;
}

static void __exit input_key_exit(void)
{
    //删除定时器
    del_timer_sync(&input_key_dev.timer_t);
    //释放中断
    free_irq(input_key_dev.irq_num, &input_key_dev);
    //注销并删除input设备
    input_unregister_device(input_key_dev.key_input_dev);
    input_free_device(input_key_dev.key_input_dev);

    gpio_free(input_key_dev.gpio_num);
}

module_init(input_key_init);
module_exit(input_key_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Alex_min");