/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2021-2022 Martin Åberg */

#include <priv.h>

void *iMemMove(Lib *lib, void *dst, const void *src, size_t n) {
  char *d = dst;
  const char *s = src;

  if (n == 0) {
    return dst;
  }
  if (s < d && d < s + n) {
    /* Backward copy needed. */
    s += n;
    d += n;
    while (n--) {
      *--d = *--s;
    }
  } else {
    while (n--) {
      *d++ = *s++;
    }
  }

  return dst;
}

void *iMemSet(Lib *lib, void *dst, int c, size_t len) {
  unsigned char *p;

  p = dst;
  while (len--) {
    *p = c;
    p++;
  }

  return dst;
}

