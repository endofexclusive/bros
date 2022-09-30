/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

.include "macros.i"
.include "defs.i"

FUNC_BEGIN iInitIntLock
        str     wzr, [x1]
        add     x1, x1, #IntLock_now_serving
        str     wzr, [x1]
        ret
FUNC_END iInitIntLock

FUNC_BEGIN iObtainIntLockDisabled
.Lget_next:
        ldxr    w0, [x1]
        add     w2, w0, #1
        stxr    w3, w2, [x1]
        cbnz    w3, .Lget_next
        add     x1, x1, #IntLock_now_serving
.Lagain:
        ldar    w2, [x1]
        cmp     w2, w0
        b.ne    .Lagain
        ret
FUNC_END iObtainIntLockDisabled

FUNC_BEGIN iReleaseIntLockDisabled
        add     x1, x1, #IntLock_now_serving
        ldr     w0, [x1]
        add     w0, w0, #1
        stlr    w0, [x1]
        ret
FUNC_END iReleaseIntLockDisabled

