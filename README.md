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