#!/bin/sh

qemu-system-arm \
        -nographic \
        -machine stm32vldiscovery \
        -kernel rui.elf \
        -S -gdb tcp::3333 \
        $@

