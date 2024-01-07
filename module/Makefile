obj-m += st7920.o
ccflags-y := -std=gnu99 -Wno-declaration-after-statement
KDIR := /hdd/pbuilding/kernel/bbb/linux-5.10.168-ti-rt-r71
CC=arm-linux-gnueabihf-

all: module dt
	echo Builded Device Tree Overlay and kernel module

cross:
	make -C $(KDIR) M=$(PWD) ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- modules

module:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	sudo dmesg --clear
	sudo rmmod st7920.ko
	sudo insmod st7920.ko
dt: testoverlay.dts
	dtc -@ -I dts -O dtb -o testoverlay.dtbo testoverlay.dts
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -rf testoverlay.dtbo

