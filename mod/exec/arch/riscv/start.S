/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022 Martin Åberg */

XLEN = __riscv_xlen
#include <riscv.h>
.include "macros.i"
.include "defs.i"
.option arch, +zicsr
.option arch, +zifencei

/*
 * invariant registers
 * sscratch: PortCPU
 */

        .text
.global _start
_start:

# - primary finds static stack
# - secondary gets ExecCPU from SBI priv parameter in a1

# a0 is hartid
# a1 is dtb for first hart arriving to us
FUNC_BEGIN exc_reset
        csrw    sie, zero
        csrw    sip, zero
        # trap FPU instructions
        li      t0, RISCV_SSTATUS_FS
        csrc    sstatus, t0

        la      t0, exc_default
        csrw    stvec, t0

        la      a2, _bss_begin
        la      a3, _bss_end
1:
        SREG    zero, (a2)
        addi    a2, a2, REGBYTES
        bne     a2, a3, 1b

        la      sp, _stack_end

        j       kcstart
FUNC_END exc_reset

FUNC_BEGIN exc_sync
        # allocate more stack space for start of "fullframe"
        addi    sp, sp, -fullframe_excframe
        SREG    s2, fullframe_s2(sp)
        SREG    s3, fullframe_s3(sp)
        SREG    s4, fullframe_s4(sp)
        SREG    s5, fullframe_s5(sp)
        SREG    s6, fullframe_s6(sp)
        SREG    s7, fullframe_s7(sp)
        SREG    s8, fullframe_s8(sp)
        SREG    s9, fullframe_s9(sp)
        SREG    s10, fullframe_s10(sp)
        SREG    s11, fullframe_s11(sp)
        SREG    gp, fullframe_gp(sp)
        SREG    tp, fullframe_tp(sp)
        addi    t0, sp, sizeof_fullframe
        SREG    t0, fullframe_sp(sp)
        csrr    t1, stval
        SREG    t1, fullframe_stval(sp)
        SREG    a0, fullframe_scause(sp)

        mv      a1, sp
        la      a0, AbsExecBase
        LREG    a0, 0(a0)
        call    theexc

        LREG    s2, fullframe_s2(sp)
        LREG    s3, fullframe_s3(sp)
        LREG    s4, fullframe_s4(sp)
        LREG    s5, fullframe_s5(sp)
        LREG    s6, fullframe_s6(sp)
        LREG    s7, fullframe_s7(sp)
        LREG    s8, fullframe_s8(sp)
        LREG    s9, fullframe_s9(sp)
        LREG    s10, fullframe_s10(sp)
        LREG    s11, fullframe_s11(sp)
        LREG    gp, fullframe_gp(sp)
        LREG    tp, fullframe_tp(sp)
        LREG    t0, fullframe_sp(sp)
        LREG    t1, fullframe_stval(sp)
        csrw    stval, t1
        LREG    a0, fullframe_scause(sp)
        csrw    scause, a0
        addi    sp, t0, fullframe_excframe-sizeof_fullframe
        j       .Lexc_exit
FUNC_END exc_sync

FUNC_BEGIN exc_default
        /* Save the interrupted context. */
        addi    sp, sp, -sizeof_excframe
        SREG    s0, excframe_s0(sp)
        SREG    s1, excframe_s1(sp)
        SREG    a0, excframe_a0(sp)
        SREG    a1, excframe_a1(sp)
        SREG    a2, excframe_a2(sp)
        csrr    a0, scause
        csrr    a1, sstatus
        csrr    a2, sepc
        csrr    s0, sscratch
        SREG    a3, excframe_a3(sp)
        SREG    a4, excframe_a4(sp)
        SREG    a5, excframe_a5(sp)
        SREG    a6, excframe_a6(sp)
        SREG    a7, excframe_a7(sp)
        SREG    t0, excframe_t0(sp)
        SREG    t1, excframe_t1(sp)
        SREG    t2, excframe_t2(sp)
        SREG    t3, excframe_t3(sp)
        SREG    t4, excframe_t4(sp)
        SREG    t5, excframe_t5(sp)
        SREG    t6, excframe_t6(sp)
        SREG    ra, excframe_ra(sp)
        SREG    a1, excframe_sstatus(sp)
        SREG    a2, excframe_sepc(sp)
        bgez    a0, exc_sync

        /* Update switch_disable and isr_nest. */
        LREG    t0, PortCPU_isr_nest(s0)
        LREG    t1, PortCPU_switch_disable(s0)
        addi    t2, t0, 1
        addi    t1, t1, 1
        SREG    t2, PortCPU_isr_nest(s0)
        SREG    t1, PortCPU_switch_disable(s0)

        /* Forcibly invalidate any existing load reservations */
        add     t5, s0, PortCPU_isrstack
        sc.w    zero, zero, (t5)

        /* stack at exception time to non-scratch register */
        mv      s1, sp
        /* If isr_nest was 0 then switch to interrupt stack. */
        bnez    t0, .Lstack_switched
        /* isrstack is at end of PortCPU */
        li      t3, sizeof_PortCPU
        add     sp, s0, t3

.Lstack_switched:
        /* Call interrupt handlers */
        mv      a2, a0
        mv      a1, s0
        la      a0, AbsExecBase
        LREG    a0, 0(a0)
        call    any_interrupt

        /* Go back to interrupted (possibly task) stack. */
        mv      sp, s1

        LREG    t0, PortCPU_isr_nest(s0)
        LREG    t1, PortCPU_switch_disable(s0)
        addi    t2, t0, -1
        addi    s1, t1, -1
        SREG    t2, PortCPU_isr_nest(s0)
        /* branch if new switch_disable is not 0 */
        bnez    s1, .Ldispatch_done
        /* cpu->switch_disable is still 1 */

        /* Switch the tasks. */
        mv      a1, s0
        la      a0, AbsExecBase
        LREG    a0, 0(a0)
        call    switch_tasks_if_needed
        /* switch_tasks() may return on another CPU. */
        mv      s0, a0

.Ldispatch_done:
        SREG    s1, PortCPU_switch_disable(s0)

.Lexc_exit:
        /* Restore the interrupted context. */
        LREG    a0, excframe_sstatus(sp)
        LREG    a1, excframe_sepc(sp)
        csrw    sstatus, a0
        csrw    sepc, a1
        LREG    s0, excframe_s0(sp)
        LREG    s1, excframe_s1(sp)
        LREG    a0, excframe_a0(sp)
        LREG    a1, excframe_a1(sp)
        LREG    a2, excframe_a2(sp)
        LREG    a3, excframe_a3(sp)
        LREG    a4, excframe_a4(sp)
        LREG    a5, excframe_a5(sp)
        LREG    a6, excframe_a6(sp)
        LREG    a7, excframe_a7(sp)
        LREG    t0, excframe_t0(sp)
        LREG    t1, excframe_t1(sp)
        LREG    t2, excframe_t2(sp)
        LREG    t3, excframe_t3(sp)
        LREG    t4, excframe_t4(sp)
        LREG    t5, excframe_t5(sp)
        LREG    t6, excframe_t6(sp)
        LREG    ra, excframe_ra(sp)
        addi    sp, sp, sizeof_excframe
        sret
FUNC_END exc_default

/*
 * executes with interrupts enable
 * a0: thistask
 * a1: heir
 */
FUNC_BEGIN port_switch_tasks
        /* get PortCPU */
        csrr    a2, sscratch

        /* Save task context to task stack. */
        addi    sp, sp, -sizeof_taskarch
        SREG    s0,  taskarch_s0 (sp)
        SREG    s1,  taskarch_s1 (sp)
        SREG    s2,  taskarch_s2 (sp)
        SREG    s3,  taskarch_s3 (sp)
        SREG    s4,  taskarch_s4 (sp)
        SREG    s5,  taskarch_s5 (sp)
        SREG    s6,  taskarch_s6 (sp)
        SREG    s7,  taskarch_s7 (sp)
        SREG    s8,  taskarch_s8 (sp)
        SREG    s9,  taskarch_s9 (sp)
        SREG    s10, taskarch_s10(sp)
        SREG    s11, taskarch_s11(sp)
        SREG    gp,  taskarch_gp (sp)
        SREG    ra,  taskarch_ra (sp)

        /* Set stack location in Task structure. */
        SREG    sp, Task_arch(a0)

        /* temporary stack for interrupt when CPU has released task */
        addi    sp, a2, PortCPU_tmpstack + sizeof_excframe

        /* prevent re-ordering of the detach store */
        /* memory barrier is implied by the amoswap */

        /* detach task from CPU */
        add     a3, a0, Task_attached
        amoswap.w.rl    zero, zero, (a3)

        li      a5, 1
.Lattach_heir:
        add     a3, a1, Task_attached
        amoswap.w.aq    a4, a5, (a3)
        beqz    a4, .Lattached
.Lreload_heir:
        LREG    a4, PortCPU_thistask(a2)
        LREG    a1, PortCPU_heir(a2)
        beq     a4, a1, .Lattach_heir
        SREG    a1, PortCPU_thistask(a2)
        j       .Lattach_heir

.Lattached:
        /* Get stack location from Task structure. */
        LREG    sp,  Task_arch(a1)
        /* Restore task context from task stack. */
        LREG    s0,  taskarch_s0 (sp)
        LREG    s1,  taskarch_s1 (sp)
        LREG    s2,  taskarch_s2 (sp)
        LREG    s3,  taskarch_s3 (sp)
        LREG    s4,  taskarch_s4 (sp)
        LREG    s5,  taskarch_s5 (sp)
        LREG    s6,  taskarch_s6 (sp)
        LREG    s7,  taskarch_s7 (sp)
        LREG    s8,  taskarch_s8 (sp)
        LREG    s9,  taskarch_s9 (sp)
        LREG    s10, taskarch_s10(sp)
        LREG    s11, taskarch_s11(sp)
        LREG    gp,  taskarch_gp (sp)
        LREG    ra,  taskarch_ra (sp)

        /* clear load reservations made by old task */
        sc.w    zero, zero, (sp)
        addi    sp, sp, sizeof_taskarch
        ret
FUNC_END port_switch_tasks

FUNC_BEGIN port_get_cpu
        csrr    a0, sscratch
        ret
FUNC_END port_get_cpu

FUNC_BEGIN port_set_cpu
        csrw    sscratch, a0
        ret
FUNC_END port_set_cpu

FUNC_BEGIN port_disable_interrupts
        csrrci  a0, sstatus, RISCV_SSTATUS_SIE
        and     a0, a0, RISCV_SSTATUS_SIE
        ret
FUNC_END port_disable_interrupts

FUNC_BEGIN port_enable_interrupts
        csrs    sstatus, a0
        ret
FUNC_END port_enable_interrupts

FUNC_BEGIN port_really_enable_interrupts
        csrs    sstatus, RISCV_SSTATUS_SIE
        ret
FUNC_END port_really_enable_interrupts

FUNC_BEGIN port_idle
        wfi
        j       port_idle
FUNC_END port_idle

FUNC_BEGIN port_enable_ipi
        li      a5, RISCV_SIE_SSIE
        csrs    sie, a5
        ret
FUNC_END port_enable_ipi

FUNC_BEGIN riscv_ecall
        ecall
        ret
FUNC_END riscv_ecall

FUNC_BEGIN riscv_fence_rw
        fence   rw,rw
        ret
FUNC_END riscv_fence_rw

FUNC_BEGIN riscv_fence_i
        fence.i
        ret
FUNC_END riscv_fence_i

FUNC_BEGIN riscv_csrs_sie
        csrs    sie, a0
        ret
FUNC_END riscv_csrs_sie

FUNC_BEGIN riscv_csrc_sie
        csrc    sie, a0
        ret
FUNC_END riscv_csrc_sie

FUNC_BEGIN riscv_csrc_sip
        csrc    sip, a0
        ret
FUNC_END riscv_csrc_sip

# a0 is hartid
# a1 is ExecCPU
FUNC_BEGIN entry_for_secondary
        csrw    sie, zero
        csrw    sip, zero
        # trap FPU instructions
        li      t0, RISCV_SSTATUS_FS
        csrc    sstatus, t0

        la      t0, exc_default
        csrw    stvec, t0

        li      t0, sizeof_PortCPU
        add     sp, a1, t0

        mv      a1, a0
        la      a0, AbsExecBase
        LREG    a0, 0(a0)
        j       start_on_secondary
FUNC_END entry_for_secondary

