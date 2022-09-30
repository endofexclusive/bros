/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

/*
 * GENERAL
 * -------
 * BCM2836 has 5 interrupt controllers:
 * - One for each processor, accessed via "bcm2836_regs"
 * - One procesor-independent, accessed via
 *   "bcm2835_interrupt_regs"
 *
 * The BCM2835 controller FIQ and IRQ outputs are connected to
 * the BCM2836 controller. They are named the "GPU" interrupts.
 *
 * In this software, all interrupts are handled by CPU0 (except
 * IPI).
 *
 * INTNUM
 * ------
 * We use the following encoding for "intnum"
 * intnum  what
 * 0       cntv for PE0
 * 1       AUX (incl. "mini UART"
 * 2       UART0 (ARM PL011)
 *
 * IPI
 * ---
 * IPI is not part of the intnum thing.  We use mailbox 3 for
 * IPI.  A mailbox generates an interrupt as long as its 32-bit
 * value is not 0.
 */

#include <priv.h>
#include <port.h>
#include <arch_expects.h>
#include <bcm2835.h>
#include <bcm2836.h>

#define err(...)

static const int NUMINTERRUPTS = 3;

static volatile struct bcm2836_regs *const bcm2836_regs =
 (void *) 0x40000000;

void port_enable_intnum(int intnum) {
  static volatile struct bcm2835_interrupt_regs *const regs =
   (void *) 0x3f00b200;
  unsigned val;

  switch (intnum) {
    case 0:
      val = bcm2836_regs->core_timers_interrupt_control[0];
      val &= CORE_TIMERS_IC_IRQ;
      val |= CORE_TIMERS_IC_CNTV_IRQ;
      bcm2836_regs->core_timers_interrupt_control[0] = val;
      break;
    case 1:
      regs->enable_gpu1 = 1U << BCM2835_INTNUM_AUX;
      break;
    case 2:
      regs->enable_gpu2 = 1U << (BCM2835_INTNUM_UART-32);
      break;
    default:
      break;
  }
}

void port_disable_intnum(int intnum) {
  static volatile struct bcm2835_interrupt_regs *const regs =
   (void *) 0x3f00b200;
  unsigned val;

  switch (intnum) {
    case 0:
      val = bcm2836_regs->core_timers_interrupt_control[0];
      val &= CORE_TIMERS_IC_IRQ;
      val &= ~CORE_TIMERS_IC_CNTV_IRQ;
      bcm2836_regs->core_timers_interrupt_control[0] = val;
      break;
    case 1:
      regs->disable_gpu1 = 1U << BCM2835_INTNUM_AUX;
      break;
    case 2:
      regs->disable_gpu2 = 1U << (BCM2835_INTNUM_UART-32);
      break;
    default:
      break;
  }
}

void port_init_interrupt(Lib *lib, unsigned long id) {
  static volatile struct bcm2835_interrupt_regs *const regs =
   (void *) 0x3f00b200;

  /* disable all of the BCM2835 interrupts */
  regs->disable_gpu1 = 0xffffffff;
  regs->disable_gpu2 = 0xffffffff;
  regs->disable_basic = 0xffffffff;
  regs->fiq_control = 0;

  /* GPU FIQ and GPU IRQ routed to CPU0 */
  bcm2836_regs->gpu_interrupts_routing = 0;
  bcm2836_regs->pmu_interrupts_routing_clear = 0xff;
  bcm2836_regs->local_interrupt_0_routing = 0;
  bcm2836_regs->axi_outstanding_irq = 0;
  bcm2836_regs->local_timer_control_and_status = 0;
  for (int i = 0; i < 4; i++) {
    bcm2836_regs->core_timers_interrupt_control[i] = 0;
    bcm2836_regs->core_mailboxes_interrupt_control[i] = 0;
  }
  for (int i = 0; i < NUMINTERRUPTS; i++) {
    port_disable_intnum(i);
  }
}

int port_get_numinterrupts(void) {
  return NUMINTERRUPTS;
}

void port_enable_ipi(unsigned int target_cpu) {
  /* clear any pending interrupt */
  bcm2836_regs->core_mailbox_read[target_cpu][3] = 0xffffffff;
  bcm2836_regs->core_mailboxes_interrupt_control[target_cpu] =
   CORE_MAILBOX_IC_MBOX3_IRQ;
}

void port_send_ipi(unsigned int target_cpu) {
  bcm2836_regs->core_mailbox_write[target_cpu][3] = 1;
}

void any_interrupt(Lib *lib, unsigned int cpunum) {
  int local_source;

  local_source = bcm2836_regs->core_irq_source[cpunum] & 0xfff;
  if (local_source & CORE_IRQ_SOURCE_MBOX3) {
    bcm2836_regs->core_mailbox_read[cpunum][3] = 0xffffffff;
    announce_ipi();
  }
  local_source &= ~CORE_IRQ_SOURCE_MBOX3;
  if (cpunum != 0) {
    if (local_source) {
      err("spurious interrupt on cpu%u\n", cpunum);
    }
    return;
  }

  /* The only thing we support at the moment */
  if (local_source & CORE_IRQ_SOURCE_CNTV) {
    runintservers(lib, 0);
    local_source &= ~CORE_IRQ_SOURCE_CNTV;
  }
  if (local_source & CORE_IRQ_SOURCE_GPU) {
    static volatile struct bcm2835_interrupt_regs *const regs =
     (void *) 0x3f00b200;
    if (regs->pend_gpu1 & (1U << BCM2835_INTNUM_AUX)) {
      runintservers(lib, 1);
      local_source &= ~CORE_IRQ_SOURCE_GPU;
    }
    if (regs->pend_gpu2 & (1U << (BCM2835_INTNUM_UART-32))) {
      runintservers(lib, 2);
      local_source &= ~CORE_IRQ_SOURCE_GPU;
    }
  }
  if (local_source) {
    err("spurious interrupt on cpu%u\n", cpunum);
  }
}

int port_get_ncpu(Lib *lib) {
  return 4;
}

