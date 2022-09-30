/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022 Martin Åberg */

#include <sparc.h>
#include <priv.h>
#include <port.h>
#include <arch_priv.h>

/*
 * Exception trap type (tt) values in to The SPARC V8 manual,
 * Table 7-1.
 */
static const struct {
  int tt;
  const char *desc;
} TTDESC[] = {
  { 0x02, "illegal_instruction", },
  { 0x07, "mem_address_not_aligned", },
  { 0x2B, "data_store_error", },
  { 0x29, "data_access_error", },
  { 0x09, "data_access_exception", },
  { 0x21, "instruction_access_error", },
  { 0x01, "instruction_access_exception", },
  { 0x04, "fp_disabled", },
  { 0x08, "fp_exception", },
  { 0x2A, "division_by_zero", },
  { 0x03, "privileged_instruction", },
  { 0x20, "r_register_access_error", },
  { 0x0B, "watchpoint_detected", },
  { 0x2C, "data_access_MMU_miss", },
  { 0x3C, "instruction_access_MMU_miss", },
  { 0x05, "window_overflow", },
  { 0x06, "window_underflow", },
  { 0x0A, "tag_overflow", },
};

static void why(Lib *lib, unsigned long tbr) {
  const char *desc;
  int tt;

  kprintf(lib, "cpu%d  ", port_get_cpu()->id);
  tt = (tbr & TBR_TT) >> TBR_TT_BIT;
  desc = "unknown";
  if (tt & 0x80) {
    desc = "trap_instruction";
  } else if (tt >= 0x11 && tt <= 0x1F) {
    desc = "interrupt";
  } else {
    for (int i = 0; i < (int) NELEM(TTDESC); i++) {
      if (TTDESC[i].tt == tt) {
        desc = TTDESC[i].desc;
        break;
      }
    }
  }
  kprintf(lib, "tt = 0x%02x, %s\n", (unsigned) tt, desc);
}

struct savearea {
  unsigned local[8];
  unsigned in[8];
};

static void print_integer_registers(
  Lib *lib,
  const struct fullframe *ff
) {
  const struct savearea *flushed = (struct savearea *) ff->o[6];

  kprintf(lib, "      INS        LOCALS     OUTS       GLOBALS\n");
  for (int i = 0; i < 8; i++) {
    kprintf(lib,
      "  %d:  %08x   %08x   %08x   %08x\n",
      i,
      flushed ? flushed->in[i] : 0,
      flushed ? flushed->local[i] : 0,
      ff->o[i],
      ff->g[i]
    );
  }
}

static void print_special_registers(
  Lib *lib,
  const struct fullframe *ff
) {
  kprintf(lib,
    "psr: %08x   wim: %08x   tbr: %08x   y: %08x\n",
    ff->psr, ff->wim, ff->tbr, ff->y
  );
  kprintf(lib, " pc: %08x   npc: %08x\n", ff->pc, ff->npc);
}

static void print_backtrace(Lib *lib, const struct fullframe *ff) {
  const int MAX_LOGLINES = 40;
  const struct savearea *s = (struct savearea *) ff->o[6];

  kprintf(lib, "      pc   sp\n");
  kprintf(lib, " #0   %08x   %08x\n", ff->pc, (unsigned int) s);
  for (int i = 1; s && i < MAX_LOGLINES; i++) {
    const unsigned pc = s->in[7];
    const unsigned sp = s->in[6];

    if (sp == 0U && pc == 0U) {
      break;
    }
    kprintf(lib, " #%-2d  %08x   %08x\n", i, pc, sp);
    if (sp == 0U || sp & 7U) {
      break;
    }
    s = (const struct savearea *) sp;
  }
}

/*
 * NOTE: The printout mix future and past. Window register state
 * is from before trap while special register state is from
 * after trap.
 */
void default_trapcode(Lib *lib, void *data, void *info) {
  const struct fullframe *ff = info;

  kprintf(lib, "\n");
  why(lib, ff->tbr);
  kprintf(lib, "\n");
  print_integer_registers(lib, ff);
  kprintf(lib, "\n");
  print_special_registers(lib, ff);
  kprintf(lib, "\n");
  print_backtrace(lib, ff);
  kprintf(lib, "\n");
  lAlert(lib, Alert_Exception |
   ((ff->tbr & TBR_TT) >> TBR_TT_BIT));
}

