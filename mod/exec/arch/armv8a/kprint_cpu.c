/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

#include <armv8a.h>
#include <priv.h>
#include <arch_priv.h>

void kprint_cpu(Lib *lib) {
  unsigned long id;
  const char *desc;
  int val;

  kprintf(lib, "CPUID:");
  id = get_midr();

  /* Implementer */
  val = id >> 24 & 0xff;
  switch (val) {
    case 0x41:  desc = "Arm Limited"; break;
    case 0x42:  desc = "Broadcom Corporation"; break;
    default:    desc = "???"; break;
  }
  kprintf(lib, " %s (%02x)", desc, (unsigned) val);

  /* Architecture */
  val = id >> 16 & 0xf;
  switch (val) {
    case 0xf:   desc = "ARMv8-A"; break;
    default:    desc = "???"; break;
  }
  kprintf(lib, "  %s (%1x)", desc, (unsigned) val);

  /* Partnum */
  val = id >> 4 & 0xfff;
  switch (val) {
    case 0xd03: desc = "Cortex-A53"; break;
    default:    desc = "???"; break;
  }
  kprintf(lib, "  %s (%03x)", desc, (unsigned) val);

  /* Variant */
  val = id >> 20 & 0xf;
  kprintf(lib, "  r%d", val);
  /* Revision */
  val = id >>  0 & 0xf;
  kprintf(lib, "p%d\n", val);
}

