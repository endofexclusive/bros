/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2021-2022 Martin Åberg */

#include <priv.h>
#include <port.h>

void iObtainIntLock(Lib *lib, IntLock *lock) {
  int plevel;

  plevel = port_disable_interrupts();
  iObtainIntLockDisabled(lib, lock);
  lock->plevel = plevel;
}

void iReleaseIntLock(Lib *lib, IntLock *lock) {
  int plevel;

  plevel = lock->plevel;
  iReleaseIntLockDisabled(lib, lock);
  port_enable_interrupts(plevel);
}

