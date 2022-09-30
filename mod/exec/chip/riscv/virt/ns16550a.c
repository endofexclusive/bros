/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022 Martin Åberg */

#include <priv.h>

struct regs {
  unsigned char io;  /* Transmitter holding, receiver data */
  unsigned char ier; /* Interrupt enable */
  unsigned char iir; /* Interrupt ID */
  unsigned char lcr; /* Line control */
  unsigned char mcr; /* Modem control */
  unsigned char lsr; /* Line status */
  unsigned char msr; /* Modem status */
};

#define LSR_THRE  0x20 /* transmit holding register empty */
#define LSR_RXRDY 0x01 /* receiver data available */

static volatile struct regs *const regs = (void *) 0x10000000;

void iRawIOInit(Lib *lib) {
  /* 8-bit mode */
  regs->lcr = 3;
  regs->mcr = 0;
  regs->ier = 0;
  /* clear interrupt status */
  regs->iir = 3 << 1;
}

void iRawPutChar(Lib *lib, int c) {
  if (c == '\n') {
    iRawPutChar(lib, '\r');
  }
  while (!(regs->lsr & LSR_THRE)) {
    volatile int i = 12;
    while (i--);
  }
  regs->io = (unsigned) c;
}

int iRawMayGetChar(Lib *lib) {
  if (!(regs->lsr & LSR_RXRDY)) {
    return -1;
  }
  return (unsigned char) regs->io;
}

