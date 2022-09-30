/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022 Martin Åberg */

#include <sparc.h>
.include "macros.i"
.include "defs.i"

/*
 * invariant registers
 * g5: nwinmin1
 * g6: PortCPU
 */

.macro RESET_TRAP
        mov     %g0, %g4
        sethi   %hi(trap_reset), %g4
        jmp     %g4 + %lo(trap_reset)
         nop
.endm
.macro TRAP_IN_TRAP
        ta      0x00
        nop
        nop
        nop
.endm
.macro JUMP_TRAP handler
        rd      %psr, %l0
        sethi   %hi(\handler), %l4
        jmp     %l4 + %lo(\handler)
         rd     %tbr, %l6
.endm
.macro DEFAULT_TRAP
        JUMP_TRAP trap_default
.endm
.macro INTERRUPT_TRAP
        rd      %psr, %l0
        sethi   %hi(trap_interrupt), %l4
        jmp     %l4 + %lo(trap_interrupt)
         rd     %tbr, %l3
.endm


        .text
.global _start
_start:

FUNC_BEGIN traptable
        RESET_TRAP
        DEFAULT_TRAP
        DEFAULT_TRAP
        DEFAULT_TRAP
        DEFAULT_TRAP

tt_wof:                                 ! 05 window_overflow
        /* Enter the window to be stored. */
        save
        /* Save local register set. */
        std     %l0, [%sp + 0x00]
        b       trap_window_overflow
         std    %l2, [%sp + 0x08]

tt_wuf:                                 ! 06 window_underflow
        rd      %wim, %l3
        /* l4 := WIM << 1 */
        sll     %l3, 1, %l4
        b       trap_window_underflow
        /* l5 := WIM >> (NWIN-1) */
         srl    %l3, %g5, %l5

        DEFAULT_TRAP
        DEFAULT_TRAP
        DEFAULT_TRAP
        DEFAULT_TRAP
        DEFAULT_TRAP
        DEFAULT_TRAP
        DEFAULT_TRAP
        DEFAULT_TRAP
        DEFAULT_TRAP

        DEFAULT_TRAP                    ! 10 reserved
.rept 15
        INTERRUPT_TRAP                  ! interrupt_level_[1..15]
.endr
.rept 16*6
        DEFAULT_TRAP                    ! 20..7F <various>
.endr
        TRAP_IN_TRAP                    ! 80
        DEFAULT_TRAP                    ! 81
        DEFAULT_TRAP                    ! 82
        JUMP_TRAP trap_flush_windows    ! 83
.rept 12
        DEFAULT_TRAP                    ! 84..8F trap_instruction
.endr
.rept 16*3
        DEFAULT_TRAP                    ! 90..BF trap_instruction
.endr
FUNC_END traptable

! return: previous PSR in g2
FUNC_BEGIN trap_disable_interrupts      ! C0
        rd      %psr, %l0
        or      %l0, 0xf00, %l3
        wr      %l3, %psr
        mov     %l0, %g2
        nop
        jmp     %l2
         rett   %l2 + 4
        unimp
FUNC_END trap_disable_interrupts

! g2: previous PSR
FUNC_BEGIN trap_enable_interrupts       ! C2
        rd      %psr, %l0
        andn    %l0, 0xf00, %l3
        and     %g2, 0xf00, %l4
        wr      %l3, %l4, %psr
        nop
        nop
        jmp     %l2
         rett   %l2 + 4
FUNC_END trap_enable_interrupts

! Set S, Clear EF
! return: previous PSR in g2
FUNC_BEGIN trap_enter_supervisor        ! C4
        rd      %psr, %l0
        sethi   %hi(PSR_EF | PSR_PS), %l4
        or      %l4, %lo(PSR_EF | PSR_PS), %l4
        andn    %l0, %l4, %l5
        wr      %l5, PSR_PS, %psr
        mov     %l0, %g2
        nop
        jmp     %l2
         rett   %l2 + 4
        unimp
        unimp
        unimp
FUNC_END trap_enter_supervisor

! Restore S and EF from previous PSR in g2
FUNC_BEGIN trap_leave_supervisor        ! C7
        rd      %psr, %l0
        sethi   %hi(PSR_EF | PSR_PS), %l4
        or      %l4, %lo(PSR_EF | PSR_PS), %l4
        andn    %l0, %l4, %l5
        and     %g2, %l4, %l3
        wr      %l5, %l3, %psr
        nop
        nop
        jmp     %l2
         rett   %l2 + 4
        unimp
        unimp
FUNC_END trap_leave_supervisor

FUNC_BEGIN trap_unused
.rept 6
        DEFAULT_TRAP                    ! CA..CF trap_instruction
.endr

.rept 16*3
        DEFAULT_TRAP                    ! D0..FF trap_instruction
.endr
FUNC_END trap_unused

FUNC_BEGIN trap_window_overflow
        std     %l4, [%sp + 0x10]
        rd      %wim, %l3
        std     %l6, [%sp + 0x18]
        /* l2 := WIM << (NWIN-1) */
        sll     %l3, %g5, %l2
        /* Save input register set. */
        std     %i0, [%sp + 0x20]
        /* l3 := WIM >> 1 */
        srl     %l3, 1, %l3
        std     %i2, [%sp + 0x28]
        /* WIM := (WIM >> 1) ^ (WIM << (NWIN-1)) */
        wr      %l3, %l2, %wim
        /* 3 instruction before restore (delayed write instruction) */
        std     %i4, [%sp + 0x30]
        nop
        std     %i6, [%sp + 0x38]
        /* Go back to trap window. */
        restore
        /* Re-execute save. */
        jmp     %l1
         rett   %l2
FUNC_END trap_window_overflow

FUNC_BEGIN trap_window_underflow
        /* WIM := (WIM << 1) ^ (WIM >> (NWIN-1)) */
        wr      %l4, %l5, %wim
        /* WIM is implicitly read so nops are needed. */
        nop
        nop
        nop

        /* Enter the window to restore. */
        restore
        restore
        ldd     [%sp + 0x00], %l0
        ldd     [%sp + 0x08], %l2
        ldd     [%sp + 0x10], %l4
        ldd     [%sp + 0x18], %l6
        ldd     [%sp + 0x20], %i0
        ldd     [%sp + 0x28], %i2
        ldd     [%sp + 0x30], %i4
        ldd     [%sp + 0x38], %i6
        /* Go back to the trap window. */
        save
        save
        /* Re-execute restore. */
        jmp     %l1
         rett   %l2
FUNC_END trap_window_underflow

/*
 * Handler for SPARC trap 0x83: trap_instruction, defined as
 * "Flush windows" by SPARC-ABI:
 *   "By executing a type 3 trap, a process asks the system to
 *   flush all its register windows to the stack."
 *
 * On entry:
 * - l0: psr
 * - l1: pc
 * - l2: npc
 */
FUNC_BEGIN trap_flush_windows
        mov     %g1, %l5
        mov     %g2, %l6
        mov     %g3, %l3
        mov     %g4, %l4
        mov     %l0, %g1
        restore

        rd      %psr, %g3
        and     %g3, PSR_CWP, %g3 /* g3 = CWP */
        rd      %wim, %g2         /* g2 = wim */
        set     1, %g4
        sll     %g4, %g3, %g4     /* g4 = wim mask for CW invalid */

.Lsave_frame_loop:
        sll     %g4, 1, %g3       /* rotate twim left by 1 */
        srl     %g4, %g5, %g4
        or      %g4, %g3, %g4     /* g4 = wim if we do one restore */

        /* If restore would underflow then we are done. */
        andcc   %g4, %g2, %g0
        bne    .Ldone_flushing
         nop
        restore                   /* go one window back */

        std     %l0, [%sp + 0x00]
        std     %l2, [%sp + 0x08]
        std     %l4, [%sp + 0x10]
        std     %l6, [%sp + 0x18]
        std     %i0, [%sp + 0x20]
        std     %i2, [%sp + 0x28]
        std     %i4, [%sp + 0x30]
        std     %i6, [%sp + 0x38]
        ba      .Lsave_frame_loop
         nop

.Ldone_flushing:
        /* Go back to trap window so we can restore the globals. */
        wr      %g1, %psr

        ! Calculate new wim to align with psr.
        and     %g5, 0x1f, %g4
        add     %g4, 1, %g4       ! g4: number of windows

        and     %g1, 0x1f, %g1
        add     %g1, 2, %g1       ! g1: future invalid window
        cmp     %g4, %g1
        bleu,a  1f
         sub    %g1, %g4, %g1     ! rotate
1:
        set     1, %g4
        sll     %g4, %g1, %g4
        wr      %g4, %wim

        mov     %l5, %g1
        mov     %l6, %g2
        mov     %l3, %g3
        mov     %l4, %g4

        jmp     %l2
         rett   %l2 + 4
FUNC_END trap_flush_windows

FUNC_BEGIN trap_reset
        mov     %sp, %g4
        wr      PSR_S, %psr
        nop
        nop
        nop
        mov     %g4, %l4

        mov     0, %g6
        mov     0, %g7
        call    sparc_get_cpuid
         nop
        mov     %o0, %l2

        call    sparc_get_nwinmin1
         nop
        mov     %o0, %g5

        set     traptable, %g1
        wr      %g1, %tbr
        wr      2, %wim

        ! FIXME: support first CPU /= 0
        subcc   %l2, %g0, %l1
        bne     .Lsecondary
         clr    %i7

        set     _bss_begin, %g2
        set     _bss_end, %g3
        clr     %g1
1:
        std     %g0, [%g2]
        add     %g2, 8, %g2
        cmp     %g2, %g3
        bleu,a  1b
         nop

        set     _stack_end, %sp
        sub     %sp, 96, %sp
        clr     %fp
        wr      PSR_PIL | PSR_S | PSR_ET, %psr
        nop
        nop
        nop

        mov     %l1, %o1
        call    kcstart         ! ramtop, id
         mov    %l4, %o0

.Lsecondary:
        set     tasklockp, %o1
        ld      [%o1], %o1
        call    iObtainIntLockDisabled
         mov    %g0, %o0

        set     AbsExecBase, %o0
        ld      [%o0], %o0
        ! findcpu() is an ABI function and may use stack.
        set     -1, %sp
        call    findcpu         /* lib, id */
         mov    %l1, %o1
        mov     %o0, %l0

        set     tasklockp, %o1
        ld      [%o1], %o1
        call    iReleaseIntLockDisabled
         mov    %g0, %o0

        cmp     %l0, %g0
        teq     0x00

        set     sizeof_PortCPU, %g3
        add     %l0, %g3, %sp

        sub     %sp, 96, %sp
        clr     %fp
        wr      PSR_PIL | PSR_S | PSR_ET, %psr
        nop
        nop
        nop

        mov     %l1, %o1
        set     AbsExecBase, %o0
        call    start_on_secondary    ! execbase, id
         ld     [%o0], %o0
FUNC_END trap_reset

/*
 * Default non-interrupt trap handler
 *
 * Non-interrupt traps run with highest interrupt level.  If a
 * non-interrupt trap happens during interrupt processing, then
 * the non-interrupt trap will overwrite part of the interrupt
 * processing stack in which case this trap handler can not
 * resume.
 * In other words:
 * - interrupt traps can not happen during non-interrupt
 *   processing.
 * - A non-interrupt trap during interrupt trap is fatal.
 * - A non-interrupt trap during task execution non-supervisor
 *   state is safe.
 *
 * On entry:
 * - l0: psr (set by trap code)
 * - l1: pc
 * - l2: npc
 * - l6: tbr (set by trap code)
 * - fp: %sp of current register window at trap time
 */
FUNC_BEGIN trap_default
        /* %g2, %g3 used at manual window overflow: save temporarily */
        mov     %g2, %l4
        mov     %g3, %l5

        /* If we trapped into the invalid window then make valid. */
        rd      %wim, %g2
        mov     %g2, %l3
        srl     %g2, %l0, %g3
        cmp     %g3, 1
        bne     1f
         sll    %g2, %g5, %g3
        srl     %g2, 1, %g2
        or      %g2, %g3, %g2

        /* Enter window to save. */
        save
        /* Install new wim calculated above. */
        mov     %g2, %wim
        nop
        nop
        nop
        /* Store to dedicated save area of the ABI stack frame. */
        std     %l0, [%sp + 0x00]
        std     %l2, [%sp + 0x08]
        std     %l4, [%sp + 0x10]
        std     %l6, [%sp + 0x18]
        std     %i0, [%sp + 0x20]
        std     %i2, [%sp + 0x28]
        std     %i4, [%sp + 0x30]
        std     %i6, [%sp + 0x38]
        /* Leave saved window. */
        restore

1:
        /*
         * %l4: %g2 at trap time
         * %l5: %g3 at trap time
         */
        set     sizeof_PortCPU, %g3
        add     %g6, %g3, %sp

        /* Allocate an ABI stack frame and fullframe */
        sub     %sp, 96 + sizeof_fullframe, %sp
        /*
         * - fp: sp of trapped task
         * - sp: supervisor stack - ABI_frame - fullframe
         */

        /* Fill in the content of the exception stack frame */
        std     %i0, [%sp + 96 + fullframe_o0]
        std     %i2, [%sp + 96 + fullframe_o2]
        std     %i4, [%sp + 96 + fullframe_o4]
        std     %i6, [%sp + 96 + fullframe_o6]
        std     %g0, [%sp + 96 + fullframe_g0]
        std     %l4, [%sp + 96 + fullframe_g2]
        std     %g4, [%sp + 96 + fullframe_g4]
        std     %g6, [%sp + 96 + fullframe_g6]
        std     %l0, [%sp + 96 + fullframe_psr] /* psr pc */
        std     %l2, [%sp + 96 + fullframe_npc] /* npc wim */
        rd      %y, %l7
        std     %l6, [%sp + 96 + fullframe_tbr] /* tbr y */

        /* Enable traps, raise PIL to mask all maskable interrupts. */
        or      %l0, PSR_PIL, %l5
        wr      %l5, %g0, %psr
        wr      %l5, PSR_ET, %psr
        nop
        nop
        nop

        /* Flush all register windows to the stack. */
        ta      TT_FLUSH_WINDOWS

        set     AbsExecBase, %o0
        ld      [%o0], %o0
        call    theexc
         add    %sp, 96, %o1

        ldd     [%sp + 96 + fullframe_o0], %i0
        ldd     [%sp + 96 + fullframe_o2], %i2
        ldd     [%sp + 96 + fullframe_o4], %i4
        ldd     [%sp + 96 + fullframe_o6], %i6
        ldd     [%sp + 96 + fullframe_g0], %g0
        ldd     [%sp + 96 + fullframe_g2], %g2
        ldd     [%sp + 96 + fullframe_g4], %g4
        ldd     [%sp + 96 + fullframe_g6], %g6
        ld      [%sp + 96 + fullframe_pc], %l1
        ld      [%sp + 96 + fullframe_npc], %l2
        ld      [%sp + 96 + fullframe_y], %l7

        /* sync l0.CWP and perform wuf if needed */
        ba      .Ltrapexit
         wr     %l7, %y
FUNC_END trap_default

/*
 * Interrupt trap handler
 *
 * - IU state is saved
 *
 * On entry:
 * - l0: psr (set by trap code)
 * - l1: pc
 * - l2: npc
 * - l3: tbr (set by trap code)
 * - fp: %sp of current register window at trap time
 */
FUNC_BEGIN trap_interrupt
        /* %g2, %g3 used at manual window overflow: save temporarily */
        mov     %g2, %l4
        mov     %g3, %l5

        /* If we trapped into the invalid window then make valid. */
        rd      %wim, %g2
        srl     %g2, %l0, %g3
        cmp     %g3, 1
        bne     1f
         sll    %g2, %g5, %g3
        srl     %g2, 1, %g2
        or      %g2, %g3, %g2

        /* Enter window to save. */
        save
        /* Install new wim calculated above. */
        mov     %g2, %wim
        nop
        nop
        nop
        /* Store to dedicated save area of the ABI stack frame. */
        std     %l0, [%sp + 0x00]
        std     %l2, [%sp + 0x08]
        std     %l4, [%sp + 0x10]
        std     %l6, [%sp + 0x18]
        std     %i0, [%sp + 0x20]
        std     %i2, [%sp + 0x28]
        std     %i4, [%sp + 0x30]
        std     %i6, [%sp + 0x38]
        /* Leave saved window. */
        restore
1:
        /*
         * %l4: %g2 at trap time
         * %l5: %g3 at trap time
         */
        /* Allocate stack for ISR context. */
        /* NOTE: can not enable traps with this sp! */
        sub     %fp, sizeof_intframe, %sp
        /*
         * - fp: sp of interrupted task
         * - sp: sp of interrupted task - sizeof_intframe
         */

        /* Save the interrupted context. */
        st      %g1, [%sp + intframe_g1]
        std     %l4, [%sp + intframe_g2]  /* g2  g3 */
        st      %g4, [%sp + intframe_g4]
        rd      %y, %l5
        st      %l5, [%sp + intframe_y]

        /* Update switch_disable and isr_nest. */
        ldd     [%g6 + PortCPU_switch_disable], %l6
        add     %l6, 1, %l6
        add     %l7, 1, %l7
        std     %l6, [%g6 + PortCPU_switch_disable]

        set     PSR_EF, %l5
        and     %l0, %l5, %l5   ! l5: EF at trap time
        subcc   %l7, 1, %l7

        /* If isr_nest was 0 then switch to interrupt stack. */
        bne     .Lstack_switched
         nop

        set     sizeof_PortCPU, %g3
        add     %g6, %g3, %sp

        cmp     %l5, %g0
        bne,a   1f
         st     %fsr, [%g6 + PortCPU_isrstack]
1:
.Lstack_switched:
        sub     %sp, 96, %sp

        /* Calculate new PIL for nesting and enable traps. */
        sll     %l3, 4, %l4       ! Move TBR.TT to PSR.PIL position.
        and     %l4, PSR_PIL, %l4 ! Keep interrupt level.
        andn    %l0, %l5, %o5     ! Clear PSR_EF
        andn    %o5, PSR_PIL, %o5
        or      %l4, %o5, %o5
        /* SPARC V8 Appendix B page 135 says 2 wrpsr should be used. */
        wr      %o5, %g0, %psr
        wr      %o5, PSR_ET, %psr
        nop
        nop
        nop

        /* Call interrupt handlers */
        set     AbsExecBase, %o0
        ld      [%o0], %o0
        mov     %l3, %o2
        call    any_interrupt
         add    %g6, PortCPU_ipi_command, %o1

        call    port_disable_interrupts
         nop

        /* Go back to interrupted (possibly task) stack. */
        sub     %fp, sizeof_intframe + 96, %sp

        /* branch if new switch_disable is not 0 */
        subcc   %l6, 1, %l6
        bne     .Ldispatch_done
         st     %l7, [%g6 + PortCPU_isr_nest]
        /* cpu->switch_disable is still 1 */

        /* Switch the tasks. */
        set     AbsExecBase, %o0
        ld      [%o0], %o0

        cmp     %l5, %g0
        beq     .Lnofp
         mov    %g6, %o1

        /* Set PSR_EF so we can access floating point registers */
        rd      %psr, %l3
        wr      %l3, %l5, %psr
        nop
        nop
        nop

        sub     %sp, 34 * 4, %sp
        std     %f0,  [%sp + 96 + 0x00]
        std     %f2,  [%sp + 96 + 0x08]
        std     %f4,  [%sp + 96 + 0x10]
        std     %f6,  [%sp + 96 + 0x18]
        std     %f8,  [%sp + 96 + 0x20]
        std     %f10, [%sp + 96 + 0x28]
        std     %f12, [%sp + 96 + 0x30]
        std     %f14, [%sp + 96 + 0x38]
        std     %f16, [%sp + 96 + 0x40]
        std     %f18, [%sp + 96 + 0x48]
        std     %f20, [%sp + 96 + 0x50]
        std     %f22, [%sp + 96 + 0x58]
        std     %f24, [%sp + 96 + 0x60]
        std     %f26, [%sp + 96 + 0x68]
        std     %f28, [%sp + 96 + 0x70]
        std     %f30, [%sp + 96 + 0x78]

        call    switch_tasks_if_needed
         st     %fsr, [%sp + 96 + 0x80]

        ldd     [%sp + 96 + 0x00], %f0
        ldd     [%sp + 96 + 0x08], %f2
        ldd     [%sp + 96 + 0x10], %f4
        ldd     [%sp + 96 + 0x18], %f6
        ldd     [%sp + 96 + 0x20], %f8
        ldd     [%sp + 96 + 0x28], %f10
        ldd     [%sp + 96 + 0x30], %f12
        ldd     [%sp + 96 + 0x38], %f14
        ldd     [%sp + 96 + 0x40], %f16
        ldd     [%sp + 96 + 0x48], %f18
        ldd     [%sp + 96 + 0x50], %f20
        ldd     [%sp + 96 + 0x58], %f22
        ldd     [%sp + 96 + 0x60], %f24
        ldd     [%sp + 96 + 0x68], %f26
        ldd     [%sp + 96 + 0x70], %f28
        ldd     [%sp + 96 + 0x78], %f30
        ld      [%sp + 96 + 0x80], %fsr
        ba      .Ldispatch_done
         add    %sp, 34 * 4, %sp
.Lnofp:
        call    switch_tasks_if_needed
         nop
        /* switch_tasks() may return on another CPU. */

.Ldispatch_done:
        st      %l6, [%g6 + PortCPU_switch_disable]
        /* Restore the interrupted context. */
        ld      [%sp + 96 + intframe_g1], %g1
        ldd     [%sp + 96 + intframe_g2], %g2
        ld      [%sp + 96 + intframe_g4], %g4
        ld      [%sp + 96 + intframe_y], %l5
        wr      %l5, %y

.Ltrapexit:
        /*
         * Sync l0.CWP with current PSR.CWP.  l0 is the PSR at
         * interrupt trap time. If we switched tasks, that trap
         * could have happened on another CPU which means l0.CWP
         * is not in sync with the CPU we execute on now.
         * Independent of this, the any_interrupt() function
         * could have called longjmp() which also makes l0.CPW
         * out of sync with current PSR.CWP.
         */
        rd      %psr, %l3
        and     %l3, PSR_CWP, %l3 ! l3: CPU CWP
        andn    %l0, PSR_CWP, %l0 ! l0: trapped PSR without CWP
        or      %l3, %l0, %l0     ! l0: trapped PSR with CPU CWP

        wr      %l0, %psr
        nop
        nop
        nop

        /* Check window_underflow condition and maybe wuf. */
        set     1, %l4
        sll     %l4, %l0, %l4   ! 1<<(cwp)
        /* rotate left */
        srl     %l4, %g5, %l5   ! 1<<(cwp-(nwin-1)) = 1<<(cwp+1-nwin)
        sll     %l4, 1, %l4     ! 1<<(cwp+1)
        or      %l4, %l5, %l4
        rd      %wim, %l7
        andcc   %l4, %l7, %g0
        beq     1f
         /* rotate left */
         srl     %l7, %g5, %l5
        sll     %l7, 1, %l4
        wr      %l4, %l5, %wim
        nop
        nop
        nop

        restore
        ldd     [%sp + 0x00], %l0
        ldd     [%sp + 0x08], %l2
        ldd     [%sp + 0x10], %l4
        ldd     [%sp + 0x18], %l6
        ldd     [%sp + 0x20], %i0
        ldd     [%sp + 0x28], %i2
        ldd     [%sp + 0x30], %i4
        ldd     [%sp + 0x38], %i6
        save
1:
        wr      %l0, %psr
        nop
        nop
        jmp     %l1
         rett   %l2
FUNC_END trap_interrupt

/*
 * executes with interrupts enable
 * enters in user or supervisor (from interrupt)
 * PSR fields changed are only S and EF (never PIL)
 * o0: thistask
 * o1: heir
 */
FUNC_BEGIN port_switch_tasks
        /* get PortCPU */
        /* PortCPU is already in g6 */

        /* Save task context to task stack. */
        std     %l0, [%sp + taskarch_l0]
        std     %l2, [%sp + taskarch_l2]
        std     %l4, [%sp + taskarch_l4]
        std     %l6, [%sp + taskarch_l6]
        std     %i0, [%sp + taskarch_i0]
        std     %i2, [%sp + taskarch_i2]
        std     %i4, [%sp + taskarch_i4]
        std     %i6, [%sp + taskarch_i6]
        ta      TT_ENTER_SUPERVISOR
        mov     %o7, %g3
        /* Borrow ABI aggregate return and callee save area. */
        std     %g2, [%sp + taskarch_psr]  /* psr o7 */
        ta      TT_FLUSH_WINDOWS

        /* Set stack location in Task structure. */
        st      %sp, [%o0 + Task_arch]

        /* temporary stack for interrupt when CPU has released task */
        set     PortCPU_tmpstack_il, %g3
        add     %g6, %g3, %sp

        /* prevent re-ordering of the detach store */
        stbar

        /* detach task from CPU */
        st      %g0, [%o0 + Task_attached]

        set     1, %o5
.Lattach_heir:
        swap    [%o1 + Task_attached], %o5
        cmp     %o5, %g0
        beq     .Lattached
         nop
.Lreload_heir:
        ld      [%g6 + PortCPU_thistask], %o4
        ld      [%g6 + PortCPU_heir], %o1
        cmp     %o4, %o1
        bne,a   .Lattach_heir
         st     %o1, [%g6 + PortCPU_thistask]
        ba,a    .Lattach_heir

.Lattached:
        /* Get stack location from Task structure. */
        /* We can get an interrupt here which stores to [%o6] */
        ld      [%o1 + Task_arch], %g4
        /* Restore task context from task stack. */
        ldd     [%g4 + taskarch_l0], %l0
        ldd     [%g4 + taskarch_l2], %l2
        ldd     [%g4 + taskarch_l4], %l4
        ldd     [%g4 + taskarch_l6], %l6
        ldd     [%g4 + taskarch_i0], %i0
        ldd     [%g4 + taskarch_i2], %i2
        ldd     [%g4 + taskarch_i4], %i4
        ldd     [%g4 + taskarch_i6], %i6
        ldd     [%g4 + taskarch_psr], %g2 /* psr o7 */
        mov     %g3, %o7
        mov     %g4, %sp

        ta      TT_LEAVE_SUPERVISOR
        /* clear load reservations made by old task */
        retl
         nop
FUNC_END port_switch_tasks

/*
 * A task enters here the first time it is activated by
 * port_switch_tasks.  The environment is supervisor mode with
 * interrupts enabled and switch_disable 1. PSR.EF is 1 if FPU
 * is available.
 *
 * - If CPU has an FPU, then init FSR according to SPARC ABI.
 * - Enter user mode and do architecture independent task entry.
 */
FUNC_BEGIN trampo
        add     %sp, sizeof_taskarch - 96, %sp
        rd      %psr, %g3
        set     PSR_EF, %g2
        andcc   %g3, %g2, %g0
        st      %g0, [%sp + 0x40]
        bne,a   .Ltrampo_with_fpu
         ld     [%sp + 0x40], %fsr
.Ltrampo_with_fpu:
        ta      TT_LEAVE_SUPERVISOR
        sethi   %hi(AbsExecBase), %g1
        call    task_entry
         ld     [%g1 + %lo(AbsExecBase)], %o0
FUNC_END trampo

FUNC_BEGIN port_get_cpu
        retl
         mov    %g6, %o0
FUNC_END port_get_cpu

FUNC_BEGIN port_set_cpu
        retl
         mov    %o0, %g6
FUNC_END port_set_cpu

FUNC_BEGIN port_disable_interrupts
        ta      TT_DISABLE_INTERRUPTS
        retl
         mov    %g2, %o0
FUNC_END port_disable_interrupts

FUNC_BEGIN port_enable_interrupts
        mov     %o0, %g2
        ta      TT_ENABLE_INTERRUPTS
        retl
         nop
FUNC_END port_enable_interrupts

FUNC_BEGIN port_really_enable_interrupts
        mov     %g0, %g2
        ta      TT_ENABLE_INTERRUPTS
        retl
         nop
FUNC_END port_really_enable_interrupts

FUNC_BEGIN port_halt
        mov     %g1, %g0
        set     1, %g1
        ta      0x00
FUNC_END port_halt

FUNC_BEGIN get_psr
        retl
         rd     %psr, %o0
FUNC_END get_psr

