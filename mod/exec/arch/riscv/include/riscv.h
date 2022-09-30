/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022 Martin Åberg */

#define RISCV_SIE_STIE      0x0020
#define RISCV_SIE_SEIE      0x0200
#define RISCV_SIE_SSIE      0x0002

#define RISCV_SIP_SSIP      0x0002

#define RISCV_SSTATUS_SIE   0x0002
#define RISCV_SSTATUS_FS    (3<<13)

#define RISCV_INTERRUPT_SOFTWARE_SUPERVISOR    1
#define RISCV_INTERRUPT_TIMER_SUPERVISOR       5
#define RISCV_INTERRUPT_EXTERNAL_SUPERVISOR    9

