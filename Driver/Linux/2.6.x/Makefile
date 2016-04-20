
# The path to the kernel source (should have been compiled first)
KERNELDIR ?= ../atmark-dist/linux-2.6.x

ifneq ($(KERNELRELEASE),)

obj-m := ht2020.o

else

PWD := $(shell pwd)

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

endif

clean:
	rm -rf *.o *~ core .*.cmd *.ko *.mod.c .tmp_versions

