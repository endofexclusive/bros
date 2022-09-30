#!/bin/sh

# EXAMPLE
# ./run.sh -singlestep -d in_asm,int

# qemu-system-aarch64 -icount 6 -net none -machine raspi3b -kernel kernel8.img -serial stdio -display none -S -gdb tcp::3333 $@
# /opt/qemu-6.0.0/bin/qemu-system-aarch64 -icount 6 -net none -machine raspi3b -kernel kernel8.img -serial stdio -display none $@
#  -icount 6 \

qemu-system-aarch64 \
  -machine raspi3b \
  -display none \
  -net none \
  -kernel kernel8.img \
  -serial stdio \
  -serial null \
  $@
# -d int

# The -initrd image will end up at address 0x08000000

