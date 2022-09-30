/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022 Martin Åberg */

XLEN = __riscv_xlen
.include "macros.i"
.include "defs.i"

FUNC_BEGIN iInitIntLock
        amoswap.w.rl    zero,zero, IntLock_next_ticket(a1)
        addi            a5, a1, IntLock_now_serving
        amoswap.w.rl    zero, zero, (a5)
        ret
FUNC_END iInitIntLock

FUNC_BEGIN iObtainIntLockDisabled
.Lget_next:
        lr.w    a2, IntLock_next_ticket(a1)
        addi    a3, a2, 1
        sc.w    a3, a3, IntLock_next_ticket(a1)
        bnez    a3, .Lget_next
.Lagain:
        lw      a4, IntLock_now_serving(a1)
        fence
        bne     a4, a2, .Lagain
        ret
FUNC_END iObtainIntLockDisabled

FUNC_BEGIN iReleaseIntLockDisabled
        lw      a5, IntLock_now_serving(a1)
        addi    a5, a5, 1
        addi    a4, a1, IntLock_now_serving
        amoswap.w.rl    zero, a5, (a4)
        ret
FUNC_END iReleaseIntLockDisabled

