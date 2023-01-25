KERNELDIR := /home/alex/alex/alientek
CURRENT_PATH := $(shell pwd)

obj-m := ap3216c.o

build:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
	arm-linux-gnueabihf-gcc ap3216c_app.c -o ap3216c_app

clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
	rm ap3216c_app