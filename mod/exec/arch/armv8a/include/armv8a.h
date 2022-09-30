/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

#define SCTLR_I           (1 << 12)
#define SCTLR_SA0         (1 <<  4)
#define SCTLR_SA          (1 <<  3)
#define SCTLR_C           (1 <<  2)
#define SCTLR_A           (1 <<  1)
#define DAIFSET_D         (1 <<  3)
#define DAIFSET_A         (1 <<  2)
#define DAIFSET_I         (1 <<  1)
#define DAIFSET_F         (1 <<  0)
#define SPSR_DAIF_I       (1 <<  7)
#define SPSR_DAIF_F       (1 <<  6)

/*
 * AArch64 Exception level and selected Stack Pointer
 * M[3:2] PSTATE.EL
 * M[0]   PSTATE.SP (0: sp_el0, 1: sp_elx
 */
#define SPSR_M_EL0t       (0 <<  0)
#define SPSR_M_EL1t       (4 <<  0)
#define SPSR_M_EL1h       (5 <<  0)

/*
 * The Execution state for EL1 is AArch64. The Execution state
 * for EL0 is determined by the current value of PSTATE.nRW when
 * executing at EL0.
 */
#define HCR_RW            (1 << 31)

#define CurrentEL_EL      (3 <<  2)
#define CurrentEL_EL_EL2  (2 <<  2)

