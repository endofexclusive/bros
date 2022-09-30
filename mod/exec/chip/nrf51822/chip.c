/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

#include <arch_expects.h>
#include <rawio.h>

const char psel_txd = 24;
const char psel_rxd = 25;

volatile unsigned *const regs = (void *) 0x40002000;

#define TASKS_HFCLKSTART      (0x000/4)
#define EVENTS_HFCLKSTARTED   (0x100/4)

static volatile unsigned *const clock = (void *) 0x40000000;

/* Not emulated in QEMU (7.0.0). Required for radio operation. */
#define SET_HFCLK_TO_USE_EXTERNAL_CRYSTAL 0

void board_init0(void) {
  if (SET_HFCLK_TO_USE_EXTERNAL_CRYSTAL) {
    /* Start HFCLK Crystal oscillator. */
    clock[EVENTS_HFCLKSTARTED] = 0;
    clock[TASKS_HFCLKSTART] = 1;
    while (clock[EVENTS_HFCLKSTARTED] == 0) {
      for (int i = 0; i < 12; i++) {
        ;
      }
    }
  }
}

