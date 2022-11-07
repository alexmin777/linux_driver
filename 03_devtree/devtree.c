#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/slab.h>

// backlight {
// 	compatible = "pwm-backlight";
// 	pwms = <&pwm1 0 5000000>;
// 	brightness-levels = <0 4 8 16 32 64 128 255>;
// 	default-brightness-level = <7>;
// 	status = "okay";
// };
    
static int __init device_tree_init(void)
{
    int ret = 0;
	int num = 0;
	int i;
    struct device_node *dev_node = NULL;
    struct property *dev_pro = NULL;
	u32 out_value = 0;
	u32 *out_array;
	const char *out_string;

    //找到指定节点
    dev_node = of_find_node_by_path("/backlight");
	if (dev_node == NULL) {
		printk("find node failed!\r\n");
		ret = -ENOENT;
		goto fail_dir;
	} else {
		printk("find %s!\r\n", dev_node->name);
	}

    //获取节点属性
    dev_pro = of_find_property(dev_node, "compatible", NULL);
	if (dev_pro == NULL) {
		printk("can't find device property\r\n");
		ret = -ENOENT;
		goto fail_pro;
	} else {
		printk("%s compatible is %s\r\n", dev_node->name, (char *)dev_pro->value);
    }

	//获取节点属性元素数量
	num = of_property_count_elems_of_size(dev_node, "brightness-levels", 4);
	if (num == 0) {
		printk("none element in compatible\r\n");
		ret = -ENOENT;
		goto faile_num;
	} else {
		printk("%d elements in compatible\r\n", num);
    }

	//获取节点属性特定元素值
	ret = of_property_read_u32_index(dev_node, "brightness-levels", 6, &out_value);
	if (ret < 0) {
		printk("read brightness-levels element failed\r\n");
		goto fail_index;
	} else {
		printk("read brightness-levels value=%d\r\n", out_value);
	}

	//读取属性中的数组数据
	out_array = kmalloc(num * sizeof(u32), GFP_KERNEL);
	if (!out_array) {
		ret = -ENOMEM;
		goto fail_mem;
	}
	ret = of_property_read_u32_array(dev_node, "brightness-levels", out_array, num);
	if (ret < 0) {
		printk("read brightness-levels array failed\r\n");
		goto fail_array;
	} else {
		printk("read brightness-levels array=");
		for (i = 0; i < num; i++) {
			printk("%d ", out_array[i]);
		}
		printk("\r\n");
	}

	//读取属性中的字符串值
	ret = of_property_read_string(dev_node, "status", &out_string);
	if (ret < 0) {
		printk("read status string failed\r\n");
		goto fail_string;
	} else {
		printk("read status string:%s\r\n", out_string);
	}

fail_string:

fail_array:

fail_mem:
	kfree(out_array);
fail_index:

faile_num:

fail_pro:

fail_dir:

    return ret;
}

static void __exit device_tree_exit(void)
{
	printk("Goodby device tree module!\r\n");

    return;
}

module_init(device_tree_init);
module_exit(device_tree_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Alex_min");