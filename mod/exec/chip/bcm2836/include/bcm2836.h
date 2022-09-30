/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

/* from "QA7 Rev 3.4, 18-08-2014" */
struct bcm2836_regs {
  unsigned control_register;
  unsigned _unused4;
  unsigned core_timer_prescaler;
  unsigned gpu_interrupts_routing;
  unsigned pmu_interrupts_routing_set;
  unsigned pmu_interrupts_routing_clear;
  unsigned _unused18;
  unsigned core_timer_access_ls;
  unsigned core_timer_access_ms;
  unsigned local_interrupt_0_routing;
  unsigned _unused28;
  unsigned axi_outstanding_counters;
  unsigned axi_outstanding_irq;
  unsigned local_timer_control_and_status;
  unsigned local_timer_write_flags;
  unsigned _unused3C;
  unsigned core_timers_interrupt_control[4];
  unsigned core_mailboxes_interrupt_control[4];
  unsigned core_irq_source[4];
  unsigned core_fiq_source[4];
  /* index 0: core (0..3), index 1: mailbox (0..3) */
  unsigned core_mailbox_write[4][4];
  unsigned core_mailbox_read[4][4];
};

#define CORE_TIMERS_IC_IRQ              (0x0FU << 0)
#define CORE_TIMERS_IC_CNTV_IRQ         (0x01U << 3)
#define CORE_TIMERS_IC_CNTHP_IRQ        (0x01U << 2)
#define CORE_TIMERS_IC_CNTPNS_IRQ       (0x01U << 1)
#define CORE_TIMERS_IC_CNTPS_IRQ        (0x01U << 0)

/* core_mailboxes_interrupt_control[target_cpu] */
#define CORE_MAILBOX_IC_MBOX3_IRQ       (1U << 3)
#define CORE_MAILBOX_IC_MBOX2_IRQ       (1U << 2)
#define CORE_MAILBOX_IC_MBOX1_IRQ       (1U << 1)
#define CORE_MAILBOX_IC_MBOX0_IRQ       (1U << 0)

/* core_irq_source[cpunum] */
#define CORE_IRQ_SOURCE_GPU             (1U << 8)
#define CORE_IRQ_SOURCE_MBOX3           (1U << 7)
#define CORE_IRQ_SOURCE_MBOX2           (1U << 6)
#define CORE_IRQ_SOURCE_MBOX1           (1U << 5)
#define CORE_IRQ_SOURCE_MBOX0           (1U << 4)
#define CORE_IRQ_SOURCE_CNTV            (1U << 3)

