KERNELDIR := /home/alex/alex/alientek
CURRENT_PATH := $(shell pwd)
obj-m := async.o

build:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
	arm-linux-gnueabihf-gcc async_app.c -o async_app
clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
	rm async_app