#!/bin/sh

# EXAMPLE
# ./run.sh -singlestep -d in_asm,int

#        -icount shift=7,align=off,sleep=off
#        -rtc clock=vm
#        -icount shift=7

qemu-system-arm \
        -nographic \
        -machine microbit \
        -kernel rui.elf \
        $@

