/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

/* ARM PL011 */
struct pl011_regs {
  unsigned dr;
  unsigned rsrecr;
  unsigned unused_08;
  unsigned unused_0c;
  unsigned unused_10;
  unsigned unused_14;
  unsigned fr;
  unsigned unused_1c;
  unsigned ilpr;
  unsigned ibrd;
  unsigned fbrd;
  unsigned lcrh;
  unsigned cr;
  unsigned ifls;
  unsigned imsc;
  unsigned ris;
  unsigned mis;
  unsigned icr;
  unsigned dmacr;
};

#define FR_TXFF         0x20
#define FR_RXFE         0x10

#define LCRH_WLEN       0x60
#define LCRH_WLEN_8     0x60
#define LCRH_WLEN_7     0x40
#define LCRH_FEN        0x10
#define LCRH_EPS        0x04
#define LCRH_PEN        0x02
#define LCRH_BRK        0x01

#define CR_RTSEN        0x4000
#define CR_RTS          0x0800
#define CR_RXE          0x0200
#define CR_TXE          0x0100
#define CR_UARTEN       0x0001

#define INT_OE          0x0400
#define INT_BE          0x0200
#define INT_PE          0x0100
#define INT_FE          0x0080
#define INT_RT          0x0040
#define INT_TX          0x0020
#define INT_RX          0x0010
#define INT_CTSM        0x0002

#define INT_ERROR       0x0780
#define INT_ALL         0x07f2

