#!/bin/sh

qemu-system-aarch64 \
  -machine raspi3b \
  -display none \
  -net none \
  -kernel kernel8.img \
  -serial stdio \
  -serial null \
  -S -gdb tcp::3333 \
  $@

