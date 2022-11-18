KERNELDIR := /home/alex/alex/alientek
CURRENT_PATH := $(shell pwd)
obj-m := led_device.o led_driver.o

build:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
	arm-linux-gnueabihf-gcc led_app.c -o led_APP
clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
	rm led_APP