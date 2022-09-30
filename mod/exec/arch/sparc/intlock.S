/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022 Martin Åberg */

.include "macros.i"

/* SPARC V8, Appendix J.4 */
FUNC_BEGIN iInitIntLock
        retl
         clr    [%o1]
FUNC_END iInitIntLock

FUNC_BEGIN iObtainIntLockDisabled
.Lretry:
        ldstub  [%o1], %o2
        tst     %o2
        be      .Lout
         nop
.Lloop:
        ldub    [%o1], %o2
        tst     %o2
        bne     .Lloop
         nop
        ba,a    .Lretry
.Lout:
        retl
         nop
FUNC_END iObtainIntLockDisabled

FUNC_BEGIN iReleaseIntLockDisabled
        stbar
        retl
         stub   %g0, [%o1]
FUNC_END iReleaseIntLockDisabled

