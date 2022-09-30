#!/bin/sh

# EXAMPLE
# ./run.sh -singlestep -d in_asm,int

#        -icount shift=7,align=off,sleep=off
#        -rtc clock=vm

# COUNT="-icount shift=7"
qemu-system-sparc \
        -machine leon3_generic \
        -nographic \
        -net none \
        ${COUNT} \
        -kernel rui.elf \
        $@

