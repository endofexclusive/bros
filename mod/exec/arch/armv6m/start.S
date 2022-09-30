/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

.macro FUNC_BEGIN name
        .global \name
        .syntax unified
        .thumb
        .thumb_func
        .type \name, %function
        \name:
.endm

.macro FUNC_END name
        .size \name, .-\name
.endm
        .text
        .global _start
        .global start_vector_table

/*
 * ARMv7-M Architecture Reference Manual (DDI 0403E.d) says:
 *   "On reset, the processor initializes the vector table base
 *   address to an IMPLEMENTATION DEFINED address. Software can
 *   find the current location of the table, or relocate the
 *   table, using the VTOR [...]."
 * The minimum alignment is 128 bytes.
 */
_start:
start_vector_table:
        .word   _stack_end      /* SP_main */
        .word   exc_reset       /* Reset */
        .word   exc_default     /* NMI */
        .word   exc_default     /* HardFault */
        .word   exc_default     /* MemManage */
        .word   exc_default     /* BusFault */
        .word   exc_default     /* UsageFault */
        .word   exc_default     /* Reserved */
        .word   exc_default     /* Reserved */
        .word   exc_default     /* Reserved */
        .word   exc_default     /* Reserved */
        .word   exc_default     /* SVCall */
        .word   exc_default     /* DebugMonitor */
        .word   exc_default     /* Reserved */
        .word   exc_default     /* PendSV */
        .word   exc_interrupt   /* SysTick */
        /*
         * Define 240 entries for external interrupt, which is
         * always enough for ARMv6-M, Cortex-M3, Cortex-M4 and
         * Cortex-M7.
         */
        .ifndef num_extint
        num_extint = 240
        .endif
        .rept num_extint
        .word   exc_interrupt   /* External interrupt #0 .. #239 */
        .endr

/*
 * Hardware (or bootloader?) put us in thread mode, privileged
 * level, using MSP. We start out by setting PRIMASK.PM=1 to
 * avoid spurious stuff.  MSP will be changed later.
 *
 * ARMv7-M Architecture Reference Manual (DDI 0403E.b) says:
 *   "When no exception is acitve, software executing in Thread
 *   or Handler mode is, effectively, executing at a priority
 *   value of (maximum supported exception priority value +1),
 *   see [...].  This corresponds to the lowest possible level
 *   of priority."
 *
 *   "BASPRI changes the priority level required for exception
 *   preemption."
 */
FUNC_BEGIN exc_reset
        /* Raise the execution priority to 0. */
        cpsid   i
        bl      kcstart

        /* Prepare MSP to be used as exception stack. */
        ldr     r1, =_stack_end
        mov     sp, r1

        /*
         * kcstart() has put the init task stack in r0. Install
         * it in PSP and switch to PSP.
         */
        msr     PSP, r0
        mrs     r0, CONTROL
        movs    r1, #2
        orrs    r0, r1
        msr     CONTROL, r0
        /*
         * "When changing the stack pointer, software must use
         * an ISB instruction immediately after the MSR
         * instruction."
         */
        isb

        /*
         * The PendSV exception has been pended earlier when
         * init was AddTask()'ed but the execution priority has
         * prevented the PendSV handler to execute.  Now lower
         * the execution priority to trig the PendSV handler and
         * start multitasking.
         */
        cpsie   i
        isb

        /* This is executed in task context. */
        ldr     r0, =AbsExecBase
        ldr     r0, [r0]
        bl      func0
FUNC_END exc_reset

FUNC_BEGIN exc_interrupt
        /* Get exception number and translate to exec "intnum". */
        mrs     r1, IPSR
        subs    r1, #15
        push    {lr}
        ldr     r0, =AbsExecBase
        ldr     r0, [r0]
        bl      runintservers
        pop     {pc}
FUNC_END exc_interrupt

/*
 * We enter here with interrupts enabled (PRIMASK.PM=0 and
 * BASEPRI=0). LR (r14) is prepared by hardware with EXC_RETURN.
 *
 * - copy from regs to task stack (psp)
 * - run exception handler (which may pick up new task)
 * - copy from new task stack to regs
 * - set psp
 *
 * This routine is compatible with ARMv6-M and ARMv7-M
 */
FUNC_BEGIN exc_default
        mrs     r1, PSP
        /* "stmia" is "store memory multiple, increment after". */
        subs    r1, #(8 * 4)
        stmia   r1!, {r4-r7}
        mov     r4, r8
        mov     r5, r9
        mov     r6, r10
        mov     r7, r11
        stmia   r1!, {r4-r7}
        subs    r1, #(8 * 4)

        mov     r4, lr
        ldr     r0, =AbsExecBase
        ldr     r0, [r0]
        bl      theexc
        mov     lr, r4

        /* New stack is in r0. Copy from memory to registers. */
        adds    r0, #(4 * 4)
        ldmia   r0!, {r4-r7}
        mov     r8, r4
        mov     r9, r5
        mov     r10, r6
        mov     r11, r7
        msr     PSP, r0
        isb
        subs    r0, #(8 * 4)
        ldmia   r0!, {r4-r7}
        bx      lr
FUNC_END exc_default

FUNC_BEGIN get_ipsr
     mrs        r0, IPSR
     bx         lr
FUNC_END get_ipsr

FUNC_BEGIN get_primask
     mrs        r0, PRIMASK
     bx         lr
FUNC_END get_primask

FUNC_BEGIN dsb_and_isb
        dsb
        isb
        bx      lr
FUNC_END dsb_and_isb

FUNC_BEGIN svc0
        svc     #0
        bx      lr
FUNC_END svc0

FUNC_BEGIN wfe
        wfe
        bx      lr
FUNC_END wfe

FUNC_BEGIN port_disable_interrupts
        mrs     r0, PRIMASK
        cpsid   i
        bx      lr
FUNC_END port_disable_interrupts

FUNC_BEGIN port_enable_interrupts
        cmp     r0, #0
        bne.n   1f
        cpsie   i
        isb
1:
        bx      lr
FUNC_END port_enable_interrupts

