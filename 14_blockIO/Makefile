KERNELDIR := /home/alex/alex/alientek
CURRENT_PATH := $(shell pwd)
obj-m := blockIO.o

build:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
	arm-linux-gnueabihf-gcc block_app.c -o block_app
clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
	rm block_app