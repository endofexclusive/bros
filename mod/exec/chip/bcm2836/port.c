/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

#include <priv.h>
#include <port.h>

void exc_reset(void);

static void boot_other(int i, void (*func)(void)) {
  unsigned long *startp = (void *) 0xD8;
  startp[i] = (unsigned long) func;
}

void port_start_other_processors(Lib *lib) {
  int ncpu = port_get_ncpu(lib);
  for (int i = 1; i < ncpu; i++) {
    boot_other(i, exc_reset);
  }
}

