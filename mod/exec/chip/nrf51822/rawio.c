/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022 Martin Åberg */

#include <priv.h>
#include <rawio.h>

/* Interesting user interface */
#define TASK_STARTRX    (0x000/4)
#define TASK_STOPRX     (0x004/4)
#define TASK_STARTTX    (0x008/4)
#define TASK_STOPTX     (0x00C/4)
#define EVENT_RXDRDY    (0x108/4)
#define EVENT_TXDRDY    (0x11C/4)
#define REG_ENABLE      (0x500/4)
#define REG_PSEL_TXD    (0x50C/4) /* Pin select for TXD */
#define REG_PSEL_RXD    (0x514/4) /* Pin select for RXD */
#define REG_RXD         (0x518/4)
#define REG_TXD         (0x51C/4)
#define REG_BAUDRATE    (0x524/4)
#define REG_CONFIG      (0x56C/4)

#define UART_ENABLE_Enabled     4
#define UART_BAUDRATE_9600      0x00275000
#define UART_BAUDRATE_115200    0x01D7E000

void iRawIOInit(Lib *lib) {
  /* 115200/8-N-1, no flow control */
  regs[REG_ENABLE] = 0;
  regs[REG_BAUDRATE] = UART_BAUDRATE_115200;
  regs[REG_CONFIG] = 0;

  /*
   * PSELRXD, PSELRTS, PSELCTS and PSELTXD must only be
   * configured when the UART is disabled.
   */
  regs[REG_PSEL_TXD] = psel_txd;
  regs[REG_PSEL_RXD] = psel_rxd;

  regs[REG_ENABLE] = UART_ENABLE_Enabled;
  regs[EVENT_RXDRDY] = 0;
  regs[EVENT_TXDRDY] = 1;
  regs[TASK_STARTRX] = 1;
  regs[TASK_STARTTX] = 1;
}

void iRawPutChar(Lib *lib, int c) {
  if (c == '\n') {
    iRawPutChar(lib, '\r');
  }

  while (regs[EVENT_TXDRDY] == 0) {
    for (int i = 0; i < 12; i++) {
      ;
    }
  }
  regs[EVENT_TXDRDY] = 0;
  regs[REG_TXD] = (unsigned) c & 0xff;
}

int iRawMayGetChar(Lib *lib) {
  if (regs[EVENT_RXDRDY] == 0) {
    return -1;
  }
  regs[EVENT_RXDRDY] = 0;
  return (unsigned char) regs[REG_RXD] & 0xff;
}

