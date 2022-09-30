#!/bin/sh

# To generate .dtb:
# -machine virt,dumpdtb=virt.dtb

qemu-system-riscv32 \
        -nographic \
        -machine virt \
        ${COUNT} \
        -net none \
        -kernel rui.elf \
        $@

