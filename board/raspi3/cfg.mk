EXEC_CHIPDIR    := chip/bcm2836
STYX_SRC_EXTRA  += getcallerpc.c

HOST            := aarch64-all-bros-
CPUFLAGS        := -mcpu=cortex-a53
CPUFLAGS        += -mno-outline-atomics
CPUFLAGS        += -mabi=lp64
# CPUFLAGS        += -march=armv8-a
# CPUFLAGS        += -mgeneral-regs-only

