/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

#include <priv.h>
#include <bcm2835.h>
#include "gpio.h"

static volatile struct bcm2835_aux_regs      *const aregs =
 (void *) 0x3f215000;
static volatile struct bcm2835_miniuart_regs *const uregs =
 (void *) 0x3f215040;

/* FIXME: This GPIO config is board (raspi3) specific */
static void set_io_for_mini_uart(void) {
  gpio_setfunc(14, FSEL_ALT5);
  gpio_setfunc(15, FSEL_ALT5);
  gpio_setpull(14, PUD_OFF);
  gpio_setpull(15, PUD_OFF);
}

/*
 * Set baud rate and characteristics (115200 8N1) and map to
 * GPIO.
 */
void iRawIOInit(Lib *lib) {
  aregs->enb |= AUX_ENB_UART;
  uregs->cntl = 0;
  /* 8-bit mode */
  uregs->lcr = 3;
  uregs->mcr = 0;
  uregs->ier = 0;
  /* clear interrupt status */
  uregs->iir = 3 << 1;
  /* magic for 115200 BAUD */
  uregs->baud = 270;
  set_io_for_mini_uart();
  /* TXEN, RXEN */
  uregs->cntl = 3;
}

void iRawPutChar(Lib *lib, int c) {
  if (c == '\n') {
    iRawPutChar(lib, '\r');
  }

  while (!(uregs->lsr & 0x20)) {
    for (volatile i = 0; i < 12; i++) {
      ;
    }
  }
  uregs->io = (uint32_t) c;
}

int iRawMayGetChar(Lib *lib) {
  if (!(uregs->lsr & 0x01)) {
    return -1;
  }
  return (unsigned char) uregs->io;
}

