/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

#include <priv.h>
#include "arch_priv.h"

void kprint_regs(Lib *lib, TaskArch *arch, unsigned ipsr) {
  kprintf(lib, "\n ipsr: %u\n", ipsr);
  /*             r[0-3,12-15] r[4-11]\n */
  kprintf(lib, "   a1: %08x    v1: %08x\n", arch->a1, arch->v1);
  kprintf(lib, "   a2: %08x    v2: %08x\n", arch->a2, arch->v2);
  kprintf(lib, "   a3: %08x    v3: %08x\n", arch->a3, arch->v3);
  kprintf(lib, "   a4: %08x    v4: %08x\n", arch->a4, arch->v4);
  kprintf(lib, "       %7s     v5: %08x\n", "",       arch->v5);
  kprintf(lib, "   ip: %08x    v6: %08x\n", arch->ip, arch->v6);
  kprintf(lib, "   sp: %08x    v7: %08x\n", (unsigned) arch, arch->v7);
  kprintf(lib, "   lr: %08x    v8: %08x\n", arch->lr, arch->v8);
  kprintf(lib, "   pc: %08x\n", arch->pc);
  kprintf(lib, " xpsr: %08x\n\n", arch->xpsr);
}

