/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022 Martin Åberg */

/* Processor State Register */
#define PSR_IMPL_BIT  28
#define PSR_VER_BIT   24
#define PSR_PIL_BIT    8
#define PSR_IMPL      (0xf << PSR_IMPL_BIT)
#define PSR_VER       (0xf << PSR_VER_BIT)
#define PSR_EF        (1 << 12)
#define PSR_S         (1 <<  7)
#define PSR_PS        (1 <<  6)
#define PSR_ET        (1 <<  5)
#define PSR_PIL       (0xf << PSR_PIL_BIT)
#define PSR_CWP       0x1f


/* Trap Base Register */
#define TBR_TT_BIT     4
#define TBR_TBA       0xfffff000
#define TBR_TT        0x00000ff0

