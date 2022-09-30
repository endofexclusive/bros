#!/bin/sh

qemu-system-riscv32 \
        -nographic \
        -machine virt \
        -icount shift=7,align=off,sleep=off \
        -rtc clock=vm \
        -net none \
        -kernel rui.elf \
        -S -gdb tcp::3334 \
        $@

