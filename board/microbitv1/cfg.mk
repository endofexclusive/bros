EXEC_CHIPDIR    := chip/nrf51822
STYX_SRC_EXTRA  := setjmp-armv6m.S
STYX_SRC_EXTRA  += getcallerpc.c
STYX_INC_EXTRA  := include-arm

HOST            := arm-all-bros-
CPUFLAGS        := -mcpu=cortex-m0
CPUFLAGS        += -mthumb
# CPUFLAGS        += -fstack-protector
# CPUFLAGS        += -fstack-protector-strong
# CPUFLAGS        += -fstack-protector-all

