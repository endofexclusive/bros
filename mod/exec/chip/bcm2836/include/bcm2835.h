/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

/* from "BCM2835 ARM Peripherals", 06 February 2012 */

/*
 * The controller uses level interrupt semantics: pending
 * register is read only and you clear requests at the
 * peripheral.
 */
struct bcm2835_interrupt_regs {
  unsigned pend_basic; /* read only */
  unsigned pend_gpu1;
  unsigned pend_gpu2;
  unsigned fiq_control;
  unsigned enable_gpu1; /* 1: enable, 0: unaffected */
  unsigned enable_gpu2;
  unsigned enable_basic;
  unsigned disable_gpu1; /* 1: disable, 0: unaffected */
  unsigned disable_gpu2;
  unsigned disable_basic;
};

#define BCM2835_INTNUM_UART   57 /* ARM PL011 */
#define BCM2835_INTNUM_SPI    54
#define BCM2835_INTNUM_I2C    53
#define BCM2835_INTNUM_AUX    29 /* incl. "mini UART" */


/* "Auxaliaries: UART1 & SPI1, SPI2" */
struct bcm2835_aux_regs {
  unsigned irq;
  unsigned enb;
};

#define AUX_ENB_SPI2          (1U <<  2)
#define AUX_ENB_SPI1          (1U <<  1)
#define AUX_ENB_UART          (1U <<  0)


struct bcm2835_miniuart_regs {
  unsigned io;
  unsigned ier;
  unsigned iir;
  unsigned lcr;
  unsigned mcr;
  unsigned lsr;
  unsigned msr;
  unsigned scratch;
  unsigned cntl;
  unsigned stat;
  unsigned baud;
};


/* General Purpose I/O (GPIO) */
struct bcm2835_gpio_regs {
  unsigned gpfsel0;       /* 00 */
  unsigned gpfsel1;
  unsigned gpfsel2;
  unsigned gpfsel3;

  unsigned gpfsel4;       /* 10 */
  unsigned gpfsel5;
  unsigned unused_18;
  unsigned gpset0;

  unsigned gpset1;        /* 20 */
  unsigned unused_24;
  unsigned gpclr0;
  unsigned gpclr1;

  unsigned unused_30;     /* 30 */
  unsigned gplev0;
  unsigned gplev1;
  unsigned unused_3c;

  unsigned gpeds0;        /* 40 */
  unsigned gpeds1;
  unsigned unused_48;
  unsigned gpren0;

  unsigned gpren1;        /* 50 */
  unsigned unused_54;
  unsigned gpfen0;
  unsigned gpfen1;

  unsigned unused_60;     /* 60 */
  unsigned gphen0;
  unsigned gphen1;
  unsigned unused_6c;

  unsigned gplen0;        /* 70 */
  unsigned gplen1;
  unsigned unused_78;
  unsigned gparen0;

  unsigned gparen1;       /* 80 */
  unsigned unused84;
  unsigned gpafen0;
  unsigned gpafen1;

  unsigned unused90;      /* 90 */
  unsigned gppud;
  unsigned gppudclk0;
  unsigned gppudclk1;
};

#define FSEL_GPIO_IN    0
#define FSEL_GPIO_OUT   1
#define FSEL_ALT0       4
#define FSEL_ALT1       5
#define FSEL_ALT2       6
#define FSEL_ALT3       7
#define FSEL_ALT4       3
#define FSEL_ALT5       2
#define FSEL_MASK       7

#define PUD_OFF         0
#define PUD_PULL_DOWN   1
#define PUD_PULL_UP     2
#define PUD_MASK        3

