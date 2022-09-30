/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022 Martin Åberg */

.ifndef _MACROS_I_
_MACROS_I_ = 1

REGBYTES = XLEN / 8

.if XLEN == 32
 .macro SREG a b
        sw \a, \b
 .endm
 .macro LREG a b
        lw \a, \b
 .endm
.else
 .macro SREG a b
        sd \a, \b
 .endm
 .macro LREG a b
        ld \a, \b
 .endm
.endif

.macro FUNC_BEGIN name
        .global \name
        .type \name, @function
        \name:
.endm

.macro FUNC_END name
        .size \name, .-\name
.endm

.macro STRUCTDEF sname
        sizeof_\sname = 0
        .macro ADDR fname
                .equiv \sname\()_\fname, sizeof_\sname
                .equ sizeof_\sname, sizeof_\sname + REGBYTES
        .endm
        .macro LONG fname
                .equiv \sname\()_\fname, sizeof_\sname
                .equ sizeof_\sname, sizeof_\sname + REGBYTES
        .endm
        .macro INT fname
                .equiv \sname\()_\fname, sizeof_\sname
                .equ sizeof_\sname, sizeof_\sname + 4
        .endm
        .macro SHORT fname
                .equiv \sname\()_\fname, sizeof_\sname
                .equ sizeof_\sname, sizeof_\sname + 2
        .endm
        .macro BYTE fname
                .equiv \sname\()_\fname, sizeof_\sname
                .equ sizeof_\sname, sizeof_\sname + 1
        .endm
        .macro STRUCT ftype fname
                .equiv \sname\()_\fname, sizeof_\sname
                .equ sizeof_\sname, sizeof_\sname + sizeof_\ftype
        .endm
        .macro ARRAY size fname
                .equiv \sname\()_\fname, sizeof_\sname
                .equ sizeof_\sname, sizeof_\sname + \size
        .endm
.endm

.macro ENDSTRUCT
        .purgem ADDR
        .purgem LONG
        .purgem INT
        .purgem SHORT
        .purgem BYTE
        .purgem STRUCT
        .purgem ARRAY
.endm

.endif

