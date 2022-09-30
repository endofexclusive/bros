/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2021-2022 Martin Åberg */

#ifndef EXEC_MUTEX_H
#define EXEC_MUTEX_H

#include <exec/lists.h>

typedef unsigned int AtomicInteger;

struct IntLock {
  AtomicInteger  next_ticket;
  AtomicInteger  now_serving;
  int            plevel; /* local interrupt level */
};

struct Mutex {
  struct List      waitqueue;
  struct Task     *owner;
  struct IntLock   lock;
  int              nest;
};

#endif

