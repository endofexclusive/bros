/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022 Martin Åberg */

#include <sparc.h>
.include "macros.i"
.include "defs.i"

FUNC_BEGIN sparc_get_cpuid
        rd      %asr17, %o3
        set     0xf0000000, %o2
        and     %o3, %o2, %o3
        retl
         srl    %o3, 28, %o0
FUNC_END sparc_get_cpuid

FUNC_BEGIN sparc_get_nwinmin1
        rd      %asr17, %o3
        set     0x1f, %o2
        retl
         and    %o3, %o2, %o0
FUNC_END sparc_get_nwinmin1

FUNC_BEGIN sparc_sync_instructions
        retl
         flush  %g0
FUNC_END sparc_sync_instructions

FUNC_BEGIN port_idle
        ta      TT_ENTER_SUPERVISOR
1:
        wr      %g0, %asr19
        ba      1b
         nop
        ta      TT_LEAVE_SUPERVISOR
FUNC_END port_idle

