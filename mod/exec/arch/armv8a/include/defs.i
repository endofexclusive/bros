/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

STRUCTDEF intframe
        LONG    x0
        LONG    x1
        LONG    x2
        LONG    x3
        LONG    x4
        LONG    x5
        LONG    x6
        LONG    x7
        LONG    x8
        LONG    x9
        LONG    x10
        LONG    x11
        LONG    x12
        LONG    x13
        LONG    x14
        LONG    x15
        LONG    x16
        LONG    x17
        LONG    x18
        LONG    x30
        LONG    spsr
        LONG    elr
        LONG    x19
        LONG    x20
ENDSTRUCT

STRUCTDEF fullframe
        LONG    x0
        LONG    x1
        LONG    x2
        LONG    x3
        LONG    x4
        LONG    x5
        LONG    x6
        LONG    x7
        LONG    x8
        LONG    x9
        LONG    x10
        LONG    x11
        LONG    x12
        LONG    x13
        LONG    x14
        LONG    x15
        LONG    x16
        LONG    x17
        LONG    x18
        LONG    x19
        LONG    x20
        LONG    x21
        LONG    x22
        LONG    x23
        LONG    x24
        LONG    x25
        LONG    x26
        LONG    x27
        LONG    x28
        LONG    x29
        LONG    x30
        LONG    unused31
        LONG    spsr
        LONG    elr
        LONG    esr
        LONG    far
ENDSTRUCT

STRUCTDEF taskarch
        LONG    x19
        LONG    x20
        LONG    x21
        LONG    x22
        LONG    x23
        LONG    x24
        LONG    x25
        LONG    x26
        LONG    x27
        LONG    x28
        LONG    x29
        LONG    x30
ENDSTRUCT

STRUCTDEF Node
        ADDR    succ
        ADDR    pred
        ADDR    name
        SHORT   pri
        SHORT   type
        INT     _pad1
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
        STRUCT  intframe tmpstack
        ARRAY   4096 isrstack
ENDSTRUCT

STRUCTDEF IntLock
        INT     next_ticket
        INT     now_serving
        INT     plevel
ENDSTRUCT

