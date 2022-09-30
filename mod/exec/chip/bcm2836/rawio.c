/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

#include <priv.h>
#include <bcm2835.h>
#include "gpio.h"
#include "pl011.h"

static volatile struct pl011_regs *const uregs =
 (void *) 0x3f201000;

/* FIXME: This GPIO config is board (raspi3) specific */
static void set_io_for_pl011_on_raspi3(void) {
  gpio_setfunc(14, FSEL_ALT0);
  gpio_setfunc(15, FSEL_ALT0);
  gpio_setpull(14, PUD_OFF);
  gpio_setpull(15, PUD_OFF);
}

/*
 * Set baud rate and characteristics (115200 8N1) and map to
 * GPIO.
 */
void iRawIOInit(Lib *lib) {
  uregs->cr = 0;
  /* FIXME: set UART clock to 4 MHz */
  set_io_for_pl011_on_raspi3();
  uregs->icr = INT_ALL;
  /* 4000000.0 Hz / (16 * (0xb/64.0 + 2)) = 115107.91366906474 BAUD */
  uregs->ibrd = 0x2;
  uregs->fbrd = 0xb;
  uregs->lcrh = LCRH_WLEN_8 | LCRH_FEN; /* 8N1, FIFO enable */
  uregs->cr = CR_RXE | CR_TXE | CR_UARTEN;
}

void iRawPutChar(Lib *lib, int c) {
  if (c == '\n') {
    iRawPutChar(lib, '\r');
  }

  while (uregs->fr & FR_TXFF) {
    for (volatile int i = 0; i < 12; i++) {
      ;
    }
  }
  uregs->dr = c & 0xff;
}

int iRawMayGetChar(Lib *lib) {
  if (uregs->fr & FR_RXFE) {
    return -1;
  }
  return (unsigned char) uregs->dr;
}

