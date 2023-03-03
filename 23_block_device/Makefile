KERNELDIR := /home/alex/alex/alientek
CURRENT_DIR := $(shell pwd)

obj-m := ramdisk.o

build:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_DIR) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_DIR) clean