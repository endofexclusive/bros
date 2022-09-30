/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022 Martin Åberg */

/*
 * GENERAL
 * -------
 * IRQMP interrupt controller
 *
 * In this software, all interrupts are handled by CPU0 (except
 * IPI).
 *
 * INTNUM
 * ------
 * We use the following encoding for "intnum"
 * intnum  what
 *  1..15  SPARC interrupt
 * 16..31  IRQMP extended interrupt
 *
 * IPI is intnum 14.
 */

#include <priv.h>
#include <port.h>
#include <arch_expects.h>
#include <sparc.h>

static const int INTNUM_IPI = 14;

#define IRQMP_NCPU_MAX 16
struct irqmp_regs {
  unsigned ilevel;
  unsigned ipend;
  unsigned iforce0;
  unsigned iclear;
  unsigned mpstat;
  unsigned brdlst;
  unsigned errstat;
  unsigned wdogctrl;
  unsigned asmpctrl;
  unsigned icselr[2];
  unsigned reserved2c;
  unsigned reserved30;
  unsigned reserved34;
  unsigned reserved38;
  unsigned reserved3c;
  unsigned pimask[IRQMP_NCPU_MAX];  /* 0x40 */
  unsigned piforce[IRQMP_NCPU_MAX]; /* 0x80 */
  unsigned pextack[IRQMP_NCPU_MAX]; /* 0xc0 */
};

/* FIXME: probe/discover/explore irqmp */
static volatile struct irqmp_regs *const regs =
 (void *) 0x80000200;

/* FIXME: Make it work for boot CPU other than 0. */
void port_enable_intnum(int intnum) {
  regs->pimask[0] |= (1U << intnum);
}

void port_disable_intnum(int intnum) {
  regs->pimask[0] &= ~(1U << intnum);
}

void port_enable_ipi(unsigned int target_cpu) {
  regs->pimask[target_cpu] = 1U << INTNUM_IPI;
}

void port_send_ipi(unsigned int target_cpu) {
  regs->piforce[target_cpu] = 1 << INTNUM_IPI;
}

int port_get_ncpu(Lib *lib) {
  return (regs->mpstat >> 28) + 1;
}

static int get_eirq(void) {
  return (regs->mpstat & 0xf0000) >> 16;
}

int port_get_numinterrupts(void) {
  return get_eirq() ? 32 : 16;
}

/* This is called only from the boot CPU. */
/* FIXME: Make it work for boot CPU other than 0. */
void port_init_interrupt(Lib *lib, unsigned long id) {
  int ncpu;

  ncpu = regs->mpstat >> 28;
  /* Assume boot loader has initialized ilevel, ipend, iforce */
  regs->pimask[0] = 0;
  if (ncpu) {
    regs->piforce[0] = 0xfffe0000;
  } else {
    regs->iforce0 = 0;
  }
}

void port_start_other_processors(Lib *lib) {
  /* try start all 16 supported by irqmp */
  regs->mpstat = 0xffff;
}

void any_interrupt(Lib *lib, char *ipi_command, unsigned int tbr) {
  int intnum;
  int eirq;

  intnum = (tbr & TBR_TT) >> TBR_TT_BIT;
  KASSERT((intnum & 0xf0) == 0x10);
  intnum &= 0xf;
  KASSERT(intnum != 0);
  if (intnum == INTNUM_IPI) {
    if (ipi_command[0]) {
      ipi_command[0] = 0;
      sparc_sync_instructions();
    }
    /* FIXME: also condition this on a "command" */
    announce_ipi();
    return;
  }
  eirq = get_eirq();
  if (intnum == eirq) {
    eirq = regs->pextack[0] & 0x1f;
    if (eirq) {
      intnum = eirq;
    }
  }
  KASSERT(0 < intnum);
  KASSERT(intnum <= 31);
  runintservers(lib, intnum);
}

