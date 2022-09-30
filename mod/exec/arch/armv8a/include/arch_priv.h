/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

/*
 * LP64 is the common Procedure Call Standard (PCS) for A64.
 *
 *  type      LP64   ILP32 
 * ------------------------
 *  char         8       8
 *  short       16      16
 *  int         32      32
 *  long        64      32
 *  long long   64      64
 *  pointer     64      32
 * ------------------------
 *
 * x0-x7: arguments
 * x0-x15: scratch
 * x8: indirect result (address to struct)
 * x16,x17: ip0,ip1 (used by call veneers, corruptible by a
 *  function)
 * x18: PR reserved for the use of platform ABIs
 * x29: FP
 * x30: LR
 */

/* Stacked on exception (interrupt) entry. */
struct intframe {
  /* x0..x17 "scratch" */
  unsigned long x[18];  /* x0..x17 */
  unsigned long x18;    /* pr "platform register" */
  unsigned long x30;    /* lr */
  unsigned long spsr;   /* used by eret */
  unsigned long elr;    /* used by eret */
  unsigned long x19;    /* used by exception assembly */
  unsigned long x20;    /* used by exception assembly */
};

/* Stacked on unhandled trap entry. */
struct fullframe {
  unsigned long x[31]; /* x0..x30 */
  unsigned long unused31;
  unsigned long spsr;
  unsigned long elr;
  unsigned long esr;
  unsigned long far;
};

/* Saved and restored by port_switch_tasks(). */
struct TaskArch {
  /* x19..x29 "not scratch" */
  unsigned long x19, x20, x21, x22, x23, x24, x25, x26, x27, x28;
  unsigned long x29; /* fp */
  unsigned long x30; /* lr */
};

void kcstart(unsigned long id);
void kprint_cpu(Lib *lib);
void theexc(Lib *lib, struct fullframe *ef);

unsigned long get_midr(void);

#define LINKER_SYMBOL(sym) extern char sym [];

LINKER_SYMBOL(_rom_begin)
LINKER_SYMBOL(_rom_end)
LINKER_SYMBOL(_addmem_bottom)
LINKER_SYMBOL(_addmem_top)

