KERNELDIR := /home/alex/alex/alientek
CURRENT_PATH := $(shell pwd)

obj-m := input_key.o

build:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean