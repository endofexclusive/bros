#!/bin/sh

qemu-system-riscv64 \
        -nographic \
        -machine virt \
        ${COUNT} \
        -net none \
        -kernel rui.elf \
        $@

