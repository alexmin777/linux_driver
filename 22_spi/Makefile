KERNELDIR := /home/alex/alex/alientek
CURRENT_PATH := $(shell pwd)

obj-m := icm20608.o

build:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
	arm-linux-gnueabihf-gcc icm20608_app.c -o icm20608_app

clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
	rm icm20608_app