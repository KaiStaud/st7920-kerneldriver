# About
the repository provides source-code for building a linux kernel module to interface a st7920-based graphic lcd.
Alongside the kernelmodule, a simple c demo is provided to write and clear the screen.

the driver expects, that the device-tree contains a corresponding node:
>  
    &spi1 {
    pinctrl-names = "default";
    pinctrl-0 = <&spi1_pins>;
    status = "okay";

    st7920: st7920@0 {
			compatible = "glcd,st7920";
			reg = <0x0>;
			spi-max-frequency = <40000>;
			spi-bits-per-word = <8>;
			status = "okay";
			spi-cs-high;
			};
    };


# Building Software
git clone https://github.com/KaiStaud/st7920-kerneldriver \
cd st7920-kerneldriver \
make -C module all \
gcc -o demo -C demo/demo.c 

# Using Module

Load the module with "insmod st7920.ko"
your linux system should contain a new char-device /dev/glcd.
If the char-device is not enumerating, follow the "FAQ".




# Connections

| SPI-Signal | LCD-Signal | BBB-Pin | Color | | 
|------------|------------|---------|---|---|
| MISO       | /          |         |   |   |
| MOSI       | RW         | P9_30 (D0)| Y|   |
| SCK        | E          | P9_31   |  O |   |
| CS         | RS         | P9_28   |  G |   |


lkm (Linux loadable Kernel Module) source for building a st7920 hardware driver. 
tested with following Targets:
- Raspberry Pi 4a (Linux 6.1.0)
- Beaglebone Black (Linux 5.10.0)

# capabilities
- char device functions: read , write
- Draw monochrome bitmaps 
- clear
- userspace Control applikation (lcdctl)
- spi interface
- cs with gpio or hardware cs
 
# FAQ / Common Issues
- Device is enumerated correctly, but display does not display
	the chip-select signal is not driven. Build the module with soft-cs (by defininig your gpionumber and setting #define USE_SOFTCS 1 in st7920.c) to drive the cs as a regular gpio:
	CS or Soft-CS should toggle 1-0-1 before and after each 3 bytes

- Kernel-Log shows "unable to send data /cmd"
	the device-tree does not contain a glcd-node,therefore the driver cannot attach to the spi-master.

# To-Do:
- Docs 
- brightness
- on/off
- contrast
- flip Display
- Font size
- invert full rows
## packaging and deployment
- build as dkms-debian package
- provide as yocto-layer

# t.b.d:
- c-style library top wrap ioctl
- dma sometime bugged
- Split in middle (two pseudeo Displays)