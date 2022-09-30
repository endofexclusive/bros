/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

/* This file should be linked only if asserts are enabled */

#include <priv.h>

#define KASSERTSTR "exec assertion \"%s\" failed: %s:%d %s"

void kassert_hit(
  Lib *lib,
  const char *e,
  const char *file,
  int line,
  const char *func
) {
  kprintf(lib, KASSERTSTR, e, file, line, func);
  lAlert(lib, AT_DeadEnd | AN_ExecLib | AG_Assert);
}

