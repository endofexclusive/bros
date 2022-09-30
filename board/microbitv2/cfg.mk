EXEC_CHIPDIR    := chip/nrf52833
STYX_SRC_EXTRA  := setjmp-armv6m.S
STYX_SRC_EXTRA  += getcallerpc.c
STYX_INC_EXTRA  := include-arm

HOST            := arm-all-bros-
CPUFLAGS        := -mcpu=cortex-m4
CPUFLAGS        += -mthumb

