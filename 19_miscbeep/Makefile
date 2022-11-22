KERNELDIR := /home/alex/alex/alientek
CURRENT_PATH := $(shell pwd)
obj-m := misc_beep.o

build:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
	arm-linux-gnueabihf-gcc beep_app.c -o beep_APP
clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
	rm beep_APP