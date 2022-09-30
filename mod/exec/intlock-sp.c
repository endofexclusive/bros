/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2021-2022 Martin Åberg */

#include <priv.h>
#include <port.h>

void iInitIntLock(Lib *lib, IntLock *lock) {
}

void iObtainIntLockDisabled(Lib *lib, IntLock *lock) {
}

void iReleaseIntLockDisabled(Lib *lib, IntLock *lock) {
}

void iObtainIntLock(Lib *lib, IntLock *lock) {
  lock->plevel = port_disable_interrupts();
}

void iReleaseIntLock(Lib *lib, IntLock *lock) {
  port_enable_interrupts(lock->plevel);
}

