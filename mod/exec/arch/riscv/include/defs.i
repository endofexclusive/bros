/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022 Martin Åberg */

STRUCTDEF excframe
        LONG    s0
        LONG    s1
        LONG    a0
        LONG    a1
        LONG    a2
        LONG    a3
        LONG    a4
        LONG    a5
        LONG    a6
        LONG    a7
        LONG    t0
        LONG    t1
        LONG    t2
        LONG    t3
        LONG    t4
        LONG    t5
        LONG    t6
        LONG    ra
        LONG    sstatus
        LONG    sepc
ENDSTRUCT

STRUCTDEF fullframe
        LONG    s2
        LONG    s3
        LONG    s4
        LONG    s5
        LONG    s6
        LONG    s7
        LONG    s8
        LONG    s9
        LONG    s10
        LONG    s11
        LONG    gp
        LONG    tp
        LONG    sp
        LONG    stval
        LONG    scause
        LONG    pad0
        STRUCT  excframe excframe
ENDSTRUCT

STRUCTDEF taskarch
        LONG    s0
        LONG    s1
        LONG    s2
        LONG    s3
        LONG    s4
        LONG    s5
        LONG    s6
        LONG    s7
        LONG    s8
        LONG    s9
        LONG    s10
        LONG    s11
        LONG    gp
        LONG    ra
ENDSTRUCT

STRUCTDEF Node
        ADDR    succ
        ADDR    pred
        ADDR    name
        SHORT   pri
        SHORT   type
.if XLEN == 64
        INT     _pad1
.endif
ENDSTRUCT

STRUCTDEF Task
        STRUCT Node     node
        ADDR            arch
        INT             attached
ENDSTRUCT

STRUCTDEF PortCPU
        STRUCT  Node node
        LONG    switch_disable
        LONG    isr_nest
        LONG    switch_needed
        LONG    id
        ADDR    thistask
        ADDR    removing
        ADDR    heir
        ADDR    idle
        STRUCT  excframe tmpstack
        ARRAY   4096 isrstack
ENDSTRUCT

STRUCTDEF IntLock
        INT     next_ticket
        INT     now_serving
        INT     plevel
ENDSTRUCT

