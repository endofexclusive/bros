/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

#include <armv8a.h>
.include "macros.i"
.include "defs.i"

/*
 * invariant registers
 * ctx_el1: PortCPU
 */

        .text
.global _start
_start:

/*
 * Vector table must be 2 KiB aligned and each vector is 128
 * bytes. We use EL1 only.
 */
.macro JUMP_EXC handler
        b       \handler
        .rept 31
        brk     #0xbad
        .endr
.endm
        .align 11
vector_table:
        /* Exception from the current EL while using SP_EL0 */
        JUMP_EXC exc_reset      /* Synchronous */
        JUMP_EXC exc_default    /* IRQ */
        JUMP_EXC exc_default    /* FIQ */
        JUMP_EXC exc_default    /* SError */
        /* Exception from the current EL while using SP_ELx */
        JUMP_EXC exc_default    /* Synchronous */
        JUMP_EXC exc_interrupt  /* IRQ */
        JUMP_EXC exc_default    /* FIQ */
        JUMP_EXC exc_default    /* SError */

FUNC_BEGIN exc_reset
        bl      enter_el1
        /* We are in EL1 */
        ldr     x2, =vector_table
        msr     vbar_el1, x2
        /* EL1, EL0 can access advanced SIMD registers and FP */
        mov     x2, #(3 << 20)
        msr     cpacr_el1, x2
        /* icache, dcache, SP alignment check, alignment check  */
#         mov     x1, #(SCTLR_SA0|SCTLR_I|SCTLR_SA|SCTLR_C|SCTLR_A)
        /* Allow unaligned memory access. Stack alignment check. */
        mov     x1, #(SCTLR_SA0 | SCTLR_SA)
        mrs     x0, sctlr_el1
        orr     x0, x0, x1
        msr     sctlr_el1, x0
        isb
        /* Enable the SError interrupt */
        /* Exception not taken when corresponding bit is set. */
        msr     daifset, #(DAIFSET_I | DAIFSET_F)
        msr     daifclr, #(DAIFSET_D | DAIFSET_A)
        /* sp=sp_el1: SP is the current exception level stack pointer */
        msr     spsel, #1
        mrs     x0, mpidr_el1
        /* Keep aff0 and aff1. If not *.*.0.0, then go to sleep. */
        and     x0, x0, #0xffff

        ldr     x4, =_stack_end
        mov     x1, 4096
        mul     x1, x1, x0
        sub     sp, x4, x1
        cbnz    x0, .Lsecondary

        adr     x2, _bss_begin
        adr     x1, _bss_end
        b       2f
1:
        stp     xzr, xzr, [x2], #16
2:
        cmp     x2, x1
        b.cc    1b

        bl      kcstart
        brk     #0
.Lsecondary:
        mov     x1, x0
        ldr     x0, =AbsExecBase
        ldr     x0, [x0]
        bl      start_on_secondary
.Lsleep:
        wfe
        b       .Lsleep
FUNC_END exc_reset

/*
 * Exception entry is described in ARMv8-A, section D1.10.
 * - SPSR_EL1 bits are copied from PSTATE
 * - ELR_EL1 is set
 * - PSTATE.DAIF := "1111"
 * - ESR_EL1 describes exception reason (for sync. exc. or
 *   SError interrupt)
 * - SP is SP_EL1 (ss=1)
 */
FUNC_BEGIN exc_default
        sub     sp,  sp,  #sizeof_fullframe

        stp     x0,  x1,  [sp, #fullframe_x0]
        stp     x2,  x3,  [sp, #fullframe_x2]
        stp     x4,  x5,  [sp, #fullframe_x4]
        stp     x6,  x7,  [sp, #fullframe_x6]
        stp     x8,  x9,  [sp, #fullframe_x8]
        stp     x10, x11, [sp, #fullframe_x10]
        stp     x12, x13, [sp, #fullframe_x12]
        stp     x14, x15, [sp, #fullframe_x14]
        stp     x16, x17, [sp, #fullframe_x16]
        stp     x18, x19, [sp, #fullframe_x18]
        stp     x20, x21, [sp, #fullframe_x20]
        stp     x22, x23, [sp, #fullframe_x22]
        stp     x24, x25, [sp, #fullframe_x24]
        stp     x26, x27, [sp, #fullframe_x26]
        stp     x28, x29, [sp, #fullframe_x28]
        str     x30,      [sp, #fullframe_x30]

        mrs     x0, spsr_el1
        mrs     x1, elr_el1
        mrs     x2, esr_el1
        mrs     x3, far_el1
        stp     x0,  x1,  [sp, #fullframe_spsr]
        stp     x2,  x3,  [sp, #fullframe_esr]

        mov     x1, sp
        ldr     x0, =AbsExecBase
        ldr     x0, [x0]
        bl      theexc
        brk     #0
FUNC_END exc_default

FUNC_BEGIN exc_interrupt
        /* Save the interrupted context. */
        sub     sp,  sp,  #sizeof_intframe
        stp     x0,  x1,  [sp, #intframe_x0]
        stp     x2,  x3,  [sp, #intframe_x2]
        stp     x4,  x5,  [sp, #intframe_x4]
        stp     x6,  x7,  [sp, #intframe_x6]
        stp     x8,  x9,  [sp, #intframe_x8]
        stp     x10, x11, [sp, #intframe_x10]
        stp     x12, x13, [sp, #intframe_x12]
        stp     x14, x15, [sp, #intframe_x14]
        stp     x16, x17, [sp, #intframe_x16]
        stp     x18, x30, [sp, #intframe_x18]
        mrs     x0, spsr_el1
        mrs     x1, elr_el1
        stp     x0,  x1,  [sp, #intframe_spsr]
        stp     x19, x20, [sp, #intframe_x19]

        mrs     x15, tpidr_el1
        /* Update switch_disable and isr_nest. */
        ldr     x0, [x15, PortCPU_isr_nest]
        ldr     x1, [x15, PortCPU_switch_disable]
        add     x2, x0, #1
        add     x1, x1, #1
        str     x2, [x15, PortCPU_isr_nest]
        str     x1, [x15, PortCPU_switch_disable]

        /* stack at exception time to non-scratch register */
        mov     x20, sp
        /* If isr_nest was 0 then switch to interrupt stack. */
        cbnz    x0, .Lstack_switched
        /* isrstack is at end of PortCPU */
        mov     x3, sizeof_PortCPU
        add     sp, x15, x3

.Lstack_switched:
        /* Call interrupt handlers */
        mrs     x2, mpidr_el1
        and     x1, x2, #0xf
        ldr     x0, =AbsExecBase
        ldr     x0, [x0]
        bl      any_interrupt
        /* bl updates x30/lr */

        /* Go back to interrupted (possibly task) stack. */
        mov     sp, x20

        mrs     x15, tpidr_el1
        ldr     x0, [x15, PortCPU_isr_nest]
        ldr     x1, [x15, PortCPU_switch_disable]
        sub     x2, x0, #1
        sub     x19, x1, #1
        str     x2, [x15, PortCPU_isr_nest]
        /* branch if new switch_disable is not 0 */
        cbnz    x19, .Ldispatch_done
        /* cpu->switch_disable is still 1 */

        /* Switch the tasks. */
        mov     x1, x15
        ldr     x0, =AbsExecBase
        ldr     x0, [x0]
        bl      switch_tasks_if_needed
        /* switch_tasks() may return on another CPU. */
        mov     x15, x0

.Ldispatch_done:
        str     x19, [x15, PortCPU_switch_disable]

        /* Restore the interrupted context. */
        ldp     x19, x20, [sp, #intframe_x19]
        ldp     x0, x1,   [sp, #intframe_spsr]
        msr     spsr_el1, x0
        msr     elr_el1, x1
        ldp     x18, x30, [sp, #intframe_x18]
        ldp     x16, x17, [sp, #intframe_x16]
        ldp     x14, x15, [sp, #intframe_x14]
        ldp     x12, x13, [sp, #intframe_x12]
        ldp     x10, x11, [sp, #intframe_x10]
        ldp     x8,  x9,  [sp, #intframe_x8]
        ldp     x6,  x7,  [sp, #intframe_x6]
        ldp     x4,  x5,  [sp, #intframe_x4]
        ldp     x2,  x3,  [sp, #intframe_x2]
        ldp     x0,  x1,  [sp, #intframe_x0]
        add     sp,  sp,  #sizeof_intframe
        eret
FUNC_END exc_interrupt

/*
 * executes with interrupts enable
 * x0: thistask
 * x1: heir
 */
FUNC_BEGIN port_switch_tasks
        /* get PortCPU */
        mrs     x2, tpidr_el1

        /* Save task context to task stack. */
        sub     sp, sp, sizeof_taskarch
        stp     x19, x20, [sp, taskarch_x19]
        stp     x21, x22, [sp, taskarch_x21]
        stp     x23, x24, [sp, taskarch_x23]
        stp     x25, x26, [sp, taskarch_x25]
        stp     x27, x28, [sp, taskarch_x27]
        stp     x29, x30, [sp, taskarch_x29]

        /* Set stack location in Task structure. */
        mov     x4, sp
        str     x4,       [x0, Task_arch]

        /* temporary stack for interrupt when CPU has released task */
        add     sp, x2, PortCPU_tmpstack + sizeof_intframe

        /* prevent re-ordering of the detach store */
        /* memory barrier is implied by the stlr */

        /* detach task from CPU */
        add     x3, x0, Task_attached
        stlr    wzr, [x3]

        mov     x5, 1
.Lattach_heir:
        add     x3, x1, Task_attached
        ldaxr   w4, [x3]
        cbnz    w4, .Lreload_heir
        stlxr   w4, w5, [x3]
        cbz     w4, .Lattached
.Lreload_heir:
        ldr     x4, [x2, PortCPU_thistask]
        ldr     x1, [x2, PortCPU_heir]
        cmp     x4, x1
        beq     .Lattach_heir
        str     x1, [x2, PortCPU_thistask]
        b       .Lattach_heir

.Lattached:
        /* Get stack location from Task structure. */
        ldr     x4, [x1, Task_arch]
        mov     sp, x4
        /* Restore task context from task stack. */
        ldp     x19, x20, [sp, taskarch_x19]
        ldp     x21, x22, [sp, taskarch_x21]
        ldp     x23, x24, [sp, taskarch_x23]
        ldp     x25, x26, [sp, taskarch_x25]
        ldp     x27, x28, [sp, taskarch_x27]
        ldp     x29, x30, [sp, taskarch_x29]
        add     sp, sp, sizeof_taskarch

        /* clear load reservations made by old task */
        clrex
        ret
FUNC_END port_switch_tasks

FUNC_BEGIN port_get_cpu
        mrs     x0, tpidr_el1
        ret
FUNC_END port_get_cpu

FUNC_BEGIN port_set_cpu
        msr     tpidr_el1, x0
        ret
FUNC_END port_set_cpu

FUNC_BEGIN port_disable_interrupts
        mrs     x0, daif
        msr     daifset, DAIFSET_I
        isb
1:
        ret
FUNC_END port_disable_interrupts

FUNC_BEGIN port_enable_interrupts
        tbnz    w0, #7, 1f
        msr     daifclr, DAIFSET_I
        isb
1:
        ret
FUNC_END port_enable_interrupts

FUNC_BEGIN port_really_enable_interrupts
        msr     daifclr, DAIFSET_I
        isb
        ret
FUNC_END port_really_enable_interrupts

FUNC_BEGIN port_idle
        wfi
        b       port_idle
FUNC_END port_idle

FUNC_BEGIN get_midr
        mrs     x0, midr_el1
        ret
FUNC_END get_midr

/* FIXME: Is this the right thing? */
FUNC_BEGIN iSyncInstructions
        ic      ialluis
        ic      iallu
        isb
        ret
FUNC_END iSyncInstructions

