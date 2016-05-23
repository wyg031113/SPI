obj-m:=spi_master.o
KDIR:=/root/Documents/fh/linux-3.0.8
CROSS_COMPILE=arm-linux-
CC:=$(CROSS_COMPILE)gcc
LD:=$(CROSS_COMPILE)ld
PWD:=$(shell pwd)
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
clean:
	rm -fr *.o *.ko *.mod.c
