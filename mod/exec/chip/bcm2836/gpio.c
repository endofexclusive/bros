/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

#include <bcm2835.h>
#include "gpio.h"

static volatile struct bcm2835_gpio_regs *const gregs =
 (void *) 0x3f200000;

void gpio_setfunc(int pin, int func) {
  volatile unsigned *gpfsel = &gregs->gpfsel0;
  int shift;
  unsigned r;

  gpfsel += pin / 10;
  shift = pin % 10;
  shift *= 3;
  r = *gpfsel;
  r &= ~(FSEL_MASK << shift);
  r |= func << shift;
  *gpfsel = r;
}

void gpio_setpull(int pin, int pull) {
  volatile unsigned *gppudclk = &gregs->gppudclk0;
  int shift;

  gppudclk += pin / 32;
  shift = pin % 32;

  /* Set the control signal */
  gregs->gppud = pull;
  /* "Wait 150 cycles" */
  for (volatile int i = 0; i < 150; i++) {
    ;
  }

  /* Clock the control signal (GPPUD) into the GPIO pads. */
  *gppudclk = pull << shift;
  for (volatile int i = 0; i < 150; i++) {
    ;
  }
  /* Remove the control signal */
  gregs->gppud = 0;
  /* Remove the clock */
  *gppudclk = 0;
}

