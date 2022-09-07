
KDIR:=/lib/modules/$(shell uname -r)/build
PWD:=$(shell pwd)
obj-m:= lan743x_main.o lan743x_ethtool.o lan743x_ptp.o
modules:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
modules_install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install
clean:
	rm -rf *.o *.ko .depend *.mod.o *.mod.c Module.* modules.*
.PHONY:modules modules_install clean
