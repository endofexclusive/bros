/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022 Martin Åberg */

#include <priv.h>
#include <port.h>
#include <arch_priv.h>

/* Supervisor cause register values in to RISC-V Volume II. */
static const struct {
  int tt;
  const char *desc;
} TTDESC[] = {
  {  0, "Instruction address misaligned"},
  {  1, "Instruction access fault"},
  {  2, "Illegal instruction"},
  {  3, "Breakpoint"},
  {  4, "Load address misaligned"},
  {  5, "Load access fault"},
  {  6, "Store/AMO address misaligned"},
  {  7, "Store/AMO access fault"},
  {  8, "Environment call from U-mode"},
  {  9, "Environment call from S-mode"},
  { 11, "Environment call from M-mode"},
  { 12, "Instruction page fault"},
  { 13, "Load page fault"},
  { 15, "Store/AMO page fault"},
};

static void why(Lib *lib, unsigned long type) {
  kprintf(lib, "cpu%d  ", port_get_cpu()->id);

  const int tt = type;
  const char *desc = "unknown";

  if (1) {
    for (int i = 0; i < (int) NELEM(TTDESC); i++) {
      if (TTDESC[i].tt == tt) {
        desc = TTDESC[i].desc;
        break;
      }
    }
  }
  kprintf(lib, "cause = 0x%02x, %s\n", (unsigned) tt, desc);
}

static const char *fmt    = "%8s: " PR_REG "%6s: " PR_REG "%6s: " PR_REG "\n";
static const char *fmt001 = "%8s  " NO_REG "%6s  " NO_REG "%6s: " PR_REG "\n";
static const char *fmt101 = "%8s: " PR_REG "%6s  " NO_REG "%6s: " PR_REG "\n";

static void print_integer_registers(
  Lib *lib,
  const struct fullframe *ff
)
{
  const struct excframe *ef = &ff->excframe;
  kprintf(lib, fmt,    "a0", ef->a[0], "t0", ef->t[0], "s0", ef->s [0]);
  kprintf(lib, fmt,    "a1", ef->a[1], "t1", ef->t[1], "s1", ef->s [1]);
  kprintf(lib, fmt,    "a2", ef->a[2], "t2", ef->t[2], "s2", ff->s2[0]);
  kprintf(lib, fmt,    "a3", ef->a[3], "t3", ef->t[3], "s3", ff->s2[1]);
  kprintf(lib, fmt,    "a4", ef->a[4], "t4", ef->t[4], "s4", ff->s2[2]);
  kprintf(lib, fmt,    "a5", ef->a[5], "t5", ef->t[5], "s5", ff->s2[3]);
  kprintf(lib, fmt,    "a6", ef->a[6], "t6", ef->t[6], "s6", ff->s2[4]);
  kprintf(lib, fmt101, "a7", ef->a[7], "",       "s7", ff->s2[5]);
  kprintf(lib, fmt001, "",       "",       "s8", ff->s2[6]);
  kprintf(lib, fmt,    "ra", ef->ra,   "sp", ff->sp,   "s9", ff->s2[7]);
  kprintf(lib, fmt,    "pc", ef->sepc,   "tp", ff->tp, "s10", ff->s2[8]);
  kprintf(lib, fmt, "status", ef->sstatus, "gp", ff->gp,"s11",ff->s2[9]);
}

static void print_special_registers(
  Lib *lib,
  const struct fullframe *ff
) {
}

void default_trapcode(Lib *lib, void *data, void *info) {
  const struct fullframe *ff = info;

  kprintf(lib, "\n");
  why(lib, ff->scause);
  kprintf(lib, "\n");
  print_integer_registers(lib, ff);
  kprintf(lib, "\n");
  print_special_registers(lib, ff);
  kprintf(lib, "\n");
  lAlert(lib, Alert_Exception | (ff->scause & 0xff));
}

