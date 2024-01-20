cmd_/home/kai/kmod/st7920/module/st7920.mod := printf '%s\n'   st7920.o | awk '!x[$$0]++ { print("/home/kai/kmod/st7920/module/"$$0) }' > /home/kai/kmod/st7920/module/st7920.mod
