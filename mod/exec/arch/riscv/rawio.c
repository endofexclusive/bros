/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022 Martin Åberg */

/* This implementation uses an SBI deprecated interface. */
#include <priv.h>
#include <sbi/sbi.h>

void iRawIOInit(Lib *lib) {
}

void iRawPutChar(Lib *lib, int c) {
  if (c == '\n') {
    iRawPutChar(lib, '\r');
  }
  sbi_console_putchar(c);
}

int iRawMayGetChar(Lib *lib) {
  return sbi_console_getchar();
}

