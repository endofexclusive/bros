#!/bin/sh

qemu-system-arm \
        -nographic \
        -machine microbit \
        -kernel rui.elf \
        -S -gdb tcp::3333 \
        $@

