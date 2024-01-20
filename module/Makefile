obj-m += st7920.o

all: module 
	echo Builded Device Tree Overlay and kernel module

module:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean	
	rm modules.order st7920.ko st7920.mod* st7920.o  testoverlay.dts
