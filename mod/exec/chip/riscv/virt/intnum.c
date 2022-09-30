/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022 Martin Åberg */

/*
 * GENERAL
 * -------
 * virt has 2 interrupt controllers:
 * - plic
 * - clint (really one per CPU)
 *
 * In this software, all interrupts are handled by CPU0 (except
 * IPI).
 *
 * INTNUM
 * ------
 * We use the following encoding for "intnum"
 * intnum  what
 * 0       supervisor timer
 * 1..     external interrupts connected to the PLIC
 *
 * IPI is RISCV_INTERRUPT_SOFTWARE_SUPERVISOR.
 */

#include <priv.h>
#include <port.h>
#include <arch_expects.h>
#include <arch_provides.h>
#include <riscv.h>

#define err(...)

#define MAX_NINTERRUPTS 1024
static const int NUMINTERRUPTS = 0x0b + 1;

struct plic_context_regs {
  unsigned priority_threshold;
  unsigned claim_complete;
  unsigned reserved_8[1022];
};

struct plic_regs {
  /* interrupt source priority (interrupt source 0 does not exist) */
  unsigned priority[MAX_NINTERRUPTS];
  /* interrupt pending bits 0-31, 32-63, etc. */
  unsigned pending[32];
  unsigned reserved0[MAX_NINTERRUPTS-32];
  /* enable bits for sources 0-32*32-1 on context i (0..16319)*/
  unsigned enable[16320][32];
  struct plic_context_regs context[15872];
};

volatile struct plic_regs *plic_regs;
volatile struct plic_context_regs *plic_context_regs;
volatile unsigned *enable_regs;

void port_enable_intnum(int intnum) {
  if (intnum == 0) {
    return;
  }
  enable_regs[intnum / 32] |= (1U << (intnum % 32));
}

void port_disable_intnum(int intnum) {
  if (intnum == 0) {
    return;
  }
  enable_regs[intnum / 32] &= ~(1U << (intnum % 32));
}

int port_get_numinterrupts(void) {
  return NUMINTERRUPTS;
}

void port_init_interrupt(Lib *lib, unsigned long id) {
  int myctx;

  plic_regs = (void *) 0x0c000000;
  myctx = 2 * id + 1;
  plic_context_regs = &plic_regs->context[myctx];
  enable_regs = &plic_regs->enable[myctx][0];
  plic_context_regs->priority_threshold = 0;
  for (int i = 0; i < NUMINTERRUPTS; i++) {
    plic_regs->priority[i] = 1;
    port_disable_intnum(i);
  }
  /* Enable external interrupts on local CPU. */
  riscv_csrs_sie(RISCV_SIE_SEIE);
  /* Enable time interrupts on local CPU. */
  riscv_csrc_sie(RISCV_SIE_STIE);
}

void any_interrupt(Lib *lib, ExecCPU *cpu, unsigned long scause) {
  scause <<= 1;
  scause >>= 1;
  if (scause == RISCV_INTERRUPT_SOFTWARE_SUPERVISOR) {
    /* Clear pending software interrupts on local CPU. */
    riscv_csrc_sip(RISCV_SIP_SSIP);
    announce_ipi();
  } else if (scause == RISCV_INTERRUPT_TIMER_SUPERVISOR) {
    runintservers(lib, 0);
  } else if (scause == RISCV_INTERRUPT_EXTERNAL_SUPERVISOR) {
    while (1) {
      unsigned claimed;

      claimed = plic_context_regs->claim_complete;
      if (claimed == 0) {
        break;
      }
      runintservers(lib, claimed);
      plic_context_regs->claim_complete = claimed;
    }
  } else {
    err("spurious interrupt on cpu%lu\n", cpu->id);
  }
}

