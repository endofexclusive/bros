/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

#include <priv.h>
#include <port.h>
#include <arch_priv.h>

/* EC: Exception class in to ARM DDI 0487F.c, Table D1-6 */
static const struct {
  int tt;
  const char *desc;
} TTDESC[] = {
  { 0x00, "Unknown reason" },
  { 0x01, "Trapped WFI or WFE instruction execution" },
  { 0x07, "Access to SVE, Advanced SIMD, or "
    "floating-point functionality trapped" },
  { 0x0e, "Illegal Execution state" },
  { 0x15, "SVC instruction execution in AArch64 state" },
  { 0x18, "Trapped MSR, MRS or System instruction execution, "
    "that is not reported using EC 0x00, 0x01 or 0x07" },
  { 0x19, "Trapped access to SVE functionality, that is not reported "
    "using EC 0b000000" },
  { 0x20, "Instruction Abort from a lower Exception level" },
  { 0x21, "Instruction Abort taken without a change in Exception "
    "level" },
  { 0x22, "PC alignment fault" },
  { 0x24, "Data Abort from a lower Exception level" },
  { 0x25, "Data Abort taken without a change in Exception level" },
  { 0x26, "SP alignment fault" },
  { 0x2c, "Trapped floating-point exception taken from AArch64 state" },
  { 0x2f, "SError interrupt" },
  { 0x30, "Breakpoint exception from a lower Exception level" },
  { 0x31, "Breakpoint exception taken without a change in "
    "Exception level" },
  { 0x32, "Software Step exception from a lower Exception level" },
  { 0x33, "Software Step exception taken without a change in "
    "Exception level" },
  { 0x34, "Watchpoint exception from a lower Exception level" },
  { 0x35, "Watchpoint exception taken without a change in "
    "Exception level" },
  { 0x3c, "BRK instruction execution in AArch64 state" },
};

static void why(Lib *lib, unsigned long esr) {
  const char *desc;
  int tt;

  kprintf(lib, "cpu%d  ", port_get_cpu()->id);
  tt = esr >> 26;
  desc = "unknown";
  if (1) {
    for (int i = 0; i < (int) NELEM(TTDESC); i++) {
      if (TTDESC[i].tt == tt) {
        desc = TTDESC[i].desc;
        break;
      }
    }
  }
  kprintf(lib, "EC = 0x%02x, %s\n", (unsigned) tt, desc);
}

static void print_integer_registers(
  Lib *lib,
  const struct fullframe *ff
) {
  for (int i = 0; i < 30; i+=2) {
    kprintf(lib, " x[%2d] %016lX  x[%2d] %016lX\n",
     i/2, ff->x[i/2], 15+i/2, ff->x[15+i/2]);
  }
  kprintf(lib, " x[30] %016lX\n", ff->x[30]);
}

static void print_special_registers(
  Lib *lib,
  const struct fullframe *ff
) {
  kprintf(lib, " ESR_EL1 %016lx  ELR_EL1 %016lx\n", ff->esr, ff->elr);
  kprintf(lib, "SPSR_EL1 %016lx  FAR_EL1 %016lx\n", ff->spsr, ff->far);
}

void default_trapcode(Lib *lib, void *data, void *info) {
  const struct fullframe *ff = info;

  kprintf(lib, "\n");
  why(lib, ff->esr);
  kprintf(lib, "\n");
  print_integer_registers(lib, ff);
  kprintf(lib, "\n");
  print_special_registers(lib, ff);
  kprintf(lib, "\n");
  lAlert(lib, Alert_Exception | (ff->esr >> 26 & 0xff));
}

