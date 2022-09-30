/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022 Martin Åberg */

#include <priv.h>

struct apbuart_regs {
  unsigned data;
  unsigned status;
  unsigned ctrl;
  unsigned scaler;
};

#define APBUART_CTRL_TE         (1 <<  1)
#define APBUART_STATUS_TE       (1 <<  2)
#define APBUART_STATUS_DR       (1 <<  0)
#define APBUART_STATUS_HOLD_REGISTER_EMPTY (1 << 2)

/* FIXME: the apbuart address assumption */
static volatile struct apbuart_regs *const regs =
 (void *) 0x80000100;

void iRawIOInit(Lib *lib) {
  regs->ctrl |= APBUART_CTRL_TE;
}

void iRawPutChar(Lib *lib, int c) {
  if (c == '\n') {
    iRawPutChar(lib, '\r');
  }
  while (!(regs->status & APBUART_STATUS_HOLD_REGISTER_EMPTY)) {
    for (volatile int i = 0; i < 12; i++) {
      ;
    }
  }
  regs->data = c & 0xff;
}

int iRawMayGetChar(Lib *lib) {
  if ((regs->status & APBUART_STATUS_DR) == 0) {
    return -1;
  }
  return (unsigned char) regs->data & 0xff;
}

