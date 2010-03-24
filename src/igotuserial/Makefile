obj-m := igotu.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
#EXTRA_CFLAGS = -DDEBUG
default:
	$(MAKE) -C $(KDIR) M=$(PWD)
