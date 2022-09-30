/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

#include <priv.h>
#include "arch_priv.h"
#include "scs.h"

void kprint_cpu(Lib *lib) {
  unsigned id;
  const char *desc;
  int val;

  kprintf(lib, "CPUID:");
  id = scb->cpuid;

  /* Implementer */
  val = id >> 24 & 0xff;
  switch (val) {
    case 0x41:  desc = "ARM"; break;
    default:    desc = "???"; break;
  }
  kprintf(lib, " %s (%02x)", desc, (unsigned) val);

  /* Architecture */
  val = id >> 16 & 0xf;
  switch (val) {
    case 0xc:   desc = "ARMv6-M"; break;
    case 0xf:   desc = "ARMv7-M"; break;
    default:    desc = "???"; break;
  }
  kprintf(lib, "  %s (%1x)", desc, (unsigned) val);

  /* Partno */
  val = id >> 4 & 0xfff;
  switch (val) {
    case 0xc20: desc = "0"; break;
    case 0xc21: desc = "1"; break;
    case 0xc23: desc = "3"; break;
    case 0xc24: desc = "4"; break;
    case 0xc60: desc = "0+"; break;
    case 0xd21: desc = "33"; break;
    default:    desc = "???"; break;
  }
  kprintf(lib, "  Cortex-M%s (%03x)", desc, (unsigned) val);

  /* Variant */
  val = id >> 20 & 0xf;
  kprintf(lib, "  r%d", val);
  /* Revision */
  val = id >>  0 & 0xf;
  kprintf(lib, "p%d\n", val);
}

