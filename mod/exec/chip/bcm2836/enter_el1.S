/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

#include <armv8a.h>
.include "macros.i"
        .text

FUNC_BEGIN enter_el1
        /* Fail if not currently in EL2 */
        mrs     x0, CurrentEL
        and     x0, x0, #CurrentEL_EL
        cmp     x0, #CurrentEL_EL_EL2
        beq     .Lwe_are_in_el2
        brk     #0

.Lwe_are_in_el2:
        /* enable CNTP for EL1 */
        mrs     x0, cnthctl_el2
        orr     x0, x0, #3
        msr     cnthctl_el2, x0
        msr     cntvoff_el2, xzr

        /*
         * Enable AArch64 state in EL1.  When an exception is
         * taken in EL1, it is the HCR_EL2.RW which controls the
         * execution state for the handler.
         */
        mrs     x0, hcr_el2
        mov     x1, #HCR_RW
        orr     x0, x0, x1
        msr     hcr_el2, x0

        /*
         * Change exception level to EL1.  SPSR is used to
         * restore the value of PSTATE when executing eret.
         * SPSR[9:6] is DAIF
         */
        mov     x2, #0x3c4
        msr     spsr_el2, x2
        adr     x2, .Lwe_are_in_el1
        msr     elr_el2, x2
        eret
.Lwe_are_in_el1:
        ret

FUNC_END enter_el1

