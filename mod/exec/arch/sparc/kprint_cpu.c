/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022 Martin Åberg */

#include <sparc.h>
#include <priv.h>
#include <arch_priv.h>

void kprint_cpu(Lib *lib) {
  unsigned int psr;
  int impl;
  int ver;

  psr = get_psr();
  impl = (psr & PSR_IMPL) >> PSR_IMPL_BIT;
  ver = (psr & PSR_VER) >> PSR_VER_BIT;
  kprintf(lib, "SPARC V8  impl %d  ver %d", impl, ver);
  if (impl == 15) {
    int v;

    v = '?';
    if (ver == 3 || ver == 5) {
      v = '0' + ver;
    }
    kprintf(lib, "  LEON%c", v);
  }
  kprintf(lib, "\n");
}

