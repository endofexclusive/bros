/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

#include <arch_expects.h>
#include <rawio.h>

#define DEVPIN(port_, pin_) (((port_)<<5) + (pin_))
const char psel_txd = DEVPIN(0, 6);
const char psel_rxd = DEVPIN(1, 8);

volatile unsigned *const regs = (void *) 0x40002000;

#define TASKS_HFCLKSTART      (0x000/4)
#define EVENTS_HFCLKSTARTED   (0x100/4)

static volatile unsigned *const clock = (void *) 0x40000000;

#define REG_ICACHECNF           (0x540/4)
#define REG_ICACHECNF_CACHEEN   0x001u
static volatile unsigned *const nvmc = (void *) 0x4001e000;

void board_init0(void) {
  nvmc[REG_ICACHECNF] = REG_ICACHECNF_CACHEEN;

  clock[EVENTS_HFCLKSTARTED] = 0;
  /* Start HFXO Crystal oscillator. */
  clock[TASKS_HFCLKSTART] = 1;
  while (clock[EVENTS_HFCLKSTARTED] == 0) {
    for (int i = 0; i < 12; i++) {
      ;
    }
  }
}

