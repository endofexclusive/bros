/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

#ifndef SCS_H
#define SCS_H

/* ARMv7-M System Control Space */

/* System Control Block */
struct scb_regs {
  /* CPUID Base */
  unsigned cpuid;
  /* Interrupt Control and State */
  unsigned icsr;
  /* Vector Table Offset */
  unsigned vtor;
  /* Application Interrupt and Reset Control */
  unsigned aircr;
  /* System Control */
  unsigned scr;
  /* Configuration and Control */
  unsigned ccr;
  /* System Handler Priority Register 1 (not in ARMv6-M) */
  unsigned shpr1;
  /* System Handler Priority Register 2 */
  unsigned shpr2;
  /* System Handler Priority Register 3 */
  unsigned shpr3;
};

#define SCB_ICSR_NMIPENDSET   (1U << 31)
#define SCB_ICSR_PENDSVSET    (1U << 28)
#define SCB_ICSR_PENDSVCLR    (1U << 27)
#define SCB_ICSR_PENDSTSET    (1U << 26)
#define SCB_ICSR_PENDSTCLR    (1U << 25)

#define SCB_CCR_STKALIGN      (1U <<  9)
#define SCB_CCR_UNALIGN_TRP   (1U <<  3)

struct systick_regs {
  unsigned csr;   /* Control and Status */
  unsigned rvr;   /* Reload Value */
  unsigned cvr;   /* Current Value */
  unsigned calib; /* Calibration value */
};

#define SYSTICK_CSR_COUNTFLAG (1U << 16)
#define SYSTICK_CSR_CLKSOURCE (1U <<  2)
#define SYSTICK_CSR_TICKINT   (1U <<  1)
#define SYSTICK_CSR_ENABLE    (1U <<  0)

#define SYSTICK_RVR_RELOAD    0x00FFFFFFU

#define SYSTICK_CALIB_NOREF   (1U << 31)
#define SYSTICK_CALIB_SKEW    (1U << 30)
#define SYSTICK_CALIB_TENMS   0x00FFFFFF

struct nvic_regs {
  unsigned iser[16];
  unsigned reserved_0[16];
  unsigned icer[16];
  unsigned reserved_1[16];
  unsigned ispr[16];
  unsigned reserved_2[16];
  unsigned icpr[16];
  unsigned reserved_3[16];
  unsigned iabr[16];
  unsigned reserved_4[48];
  /*
   * ipr is 8-bit and 16-bit accessible in ARMv7-M but not
   * ARMv6-M
   */
  unsigned ipr[124];
};

#define ICTR_INTLINESNUM      0xf


static volatile unsigned *const ictr =
 (void *) 0xe000e004;
static volatile struct systick_regs *const systick =
 (void *) 0xe000e010;
static volatile struct nvic_regs *const nvic =
 (void *) 0xe000e100;
static volatile struct scb_regs *const scb =
 (void *) 0xe000ed00;

/*
 * Exception numbers according to ARMv7-M Architecture Reference
 * Manual (DDI 0403E.d), section B1.5.2.
 */
enum armv7m_exception {
  ARMV7M_EXCEPTION_Reserved0,
  ARMV7M_EXCEPTION_Reset,
  ARMV7M_EXCEPTION_NMI,
  ARMV7M_EXCEPTION_HardFault,
  ARMV7M_EXCEPTION_MemManage,
  ARMV7M_EXCEPTION_BusFault,
  ARMV7M_EXCEPTION_UsageFault,
  ARMV7M_EXCEPTION_Reserved7,
  ARMV7M_EXCEPTION_Reserved8,
  ARMV7M_EXCEPTION_Reserved9,
  ARMV7M_EXCEPTION_Reserved10,
  ARMV7M_EXCEPTION_SVCall,
  ARMV7M_EXCEPTION_DebugMonitor,
  ARMV7M_EXCEPTION_Reserved13,
  ARMV7M_EXCEPTION_PendSV,
  ARMV7M_EXCEPTION_SysTick,
  ARMV7M_EXCEPTION_External_interrupt0,
};

#endif

