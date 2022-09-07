
KERNELBUILD = /home/emy/disk/source/linux-3.10-ltsi
CROSS_COMPILE = /home/emy/disk/cross-compiler/gcc-linaro-4.9.4-2017.01-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-


obj-m += kernel_timer.o

default:
	make -C $(KERNELBUILD) M=$(shell pwd)  ARCH=arm modules
clean:
	rm -rf *.o *.ko *.mod.c .*.cmd *.markers *.order *.symvers .tmp_versions

