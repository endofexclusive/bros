EXEC_CHIPDIR    := chip/riscv/virt
STYX_SRC_EXTRA  += getcallerpc.c

HOST            := riscv-all-bros-

# Use one of the CPUFLAGS sets below
CPUFLAGS        := -march=rv32ima   -mabi=ilp32
# CPUFLAGS        := -march=rv64ima   -mabi=lp64  -mcmodel=medany
# CPUFLAGS        := -march=rv64imafd -mabi=lp64d -mcmodel=medany

