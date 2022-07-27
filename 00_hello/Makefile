KERNELDIR := /home/alex/alex/alientek
CURRENT_PATH := $(shell pwd)
obj-m := hello.o

build:kernel_modules

kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
	arm-linux-gnueabihf-gcc hello_app.c -o hello_APP
clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
	rm hello_APP
