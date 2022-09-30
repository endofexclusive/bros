EXEC_CHIPDIR := chip/stm32f1
STYX_SRC_EXTRA  := setjmp-armv6m.S
STYX_SRC_EXTRA  += getcallerpc.c
STYX_INC_EXTRA  := include-arm

HOST            := arm-all-bros-
CPUFLAGS        := -mcpu=cortex-m3
CPUFLAGS        += -mthumb

