/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022 Martin Åberg */

TT_FLUSH_WINDOWS        = 0x83
TT_DISABLE_INTERRUPTS   = 0xc0
TT_ENABLE_INTERRUPTS    = 0xc2
TT_ENTER_SUPERVISOR     = 0xc4
TT_LEAVE_SUPERVISOR     = 0xc7

STRUCTDEF intframe
        INT     unused0
        INT     g1
        INT     g2
        INT     g3
        INT     g4
        INT     y
ENDSTRUCT

STRUCTDEF fullframe
        INT     g0
        INT     g1
        INT     g2
        INT     g3
        INT     g4
        INT     g5
        INT     g6
        INT     g7
        INT     o0
        INT     o1
        INT     o2
        INT     o3
        INT     o4
        INT     o5
        INT     o6
        INT     o7
        INT     psr
        INT     pc
        INT     npc
        INT     wim
        INT     tbr
        INT     y
ENDSTRUCT

STRUCTDEF taskarch
        INT     l0
        INT     l1
        INT     l2
        INT     l3
        INT     l4
        INT     l5
        INT     l6
        INT     l7

        INT     i0
        INT     i1
        INT     i2
        INT     i3
        INT     i4
        INT     i5
        INT     i6
        INT     i7

        INT     psr
        INT     o7
ENDSTRUCT

STRUCTDEF Node
        ADDR    succ
        ADDR    pred
        ADDR    name
        SHORT   pri
        SHORT   type
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
        ARRAY   8 ipi_command
        STRUCT  intframe tmpstack_intframe
        ARRAY   64 tmpstack_il
        ARRAY   4096 isrstack
ENDSTRUCT

