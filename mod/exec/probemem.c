/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2021-2022 Martin Åberg */

#include <stdint.h>
#include <priv.h>

#if 1
#define info(...) kprintf(lib, __VA_ARGS__)
#else
#define info(...)
#endif

/*
 * This probing routine is destructive as it does not restore
 * overwritten data. Current stack memory and static data may
 * also be aliased in the probed window depending on memory
 * controller and configuration.
 */
static void *probetop(void *base, void *top, size_t chunk) {
  uintptr_t i;
  const uintptr_t CHUNKMASK = ~(chunk-1);
  const uintptr_t f = ((uintptr_t) base + chunk - 1) & CHUNKMASK;
  const uintptr_t l = ((uintptr_t) top - chunk) & CHUNKMASK;

  for (i = l; f <= i; i -= chunk) {
    *((volatile uintptr_t *) i) = i;
  }
  for (i = f; i <= l; i += chunk) {
    if (*((volatile uintptr_t *) i) != i) {
      break;
    }
  }
  if (i == f) {
    return NULL;
  }
  return (void *) i;
}

size_t probeaddmem(
  Lib *lib,
  char *bottom,
  char *top,
  size_t chunk,
  const char *name,
  int attr,
  int priority
) {
  size_t sz = 0;
  MemHeader *mh;

  top = probetop(bottom, top, chunk);
  sz = top - bottom;
  info("[%p..%p]  %lu KiB\n", bottom, top, (unsigned long) sz / 1024);
  if (sz == 0) {
    return 0;
  }

  mh = iInitMemHeader(lib, bottom, top - bottom);
  if (mh == NULL) {
    /* probably to little memory here */
    return 0;
  }

  iAddMemHeader(lib, mh, name, attr, priority);

  return sz;
}

