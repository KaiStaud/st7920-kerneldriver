obj-m += st7920.o

all: module dt
	echo Builded Device Tree Overlay and kernel module

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
	
