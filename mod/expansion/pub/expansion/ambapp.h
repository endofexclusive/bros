/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022 Martin Åberg */

#ifndef EXPANSION_AMBAPP_H
#define EXPANSION_AMBAPP_H

/* Definitions for working with GRLIB peripherals */

#include <expansion/expansion.h>

struct AmbappCompat {
  short vendor;
  short device;
};

struct AmbappDriver {
  struct ExpansionDriver base;
  struct AmbappCompat *compat;
};

struct AmbappDev {
  struct ExpansionDev base;
  short vendor;
  short device;
  char version;
  char irq;
  struct {
    struct {
      unsigned int addr;
      unsigned int mask;
    } bar[4];
  } ahbs;
  struct {
    struct {
      unsigned int addr;
      unsigned int mask;
    } bar;
  } apbs;
};

/* Constants from "GRLIB IP Core User's Manual" */
#define AMBAPP_VENDOR_GAISLER     0x01
#define AMBAPP_VENDOR_ESA         0x04
#define AMBAPP_GAISLER_APBCTRL    0x006
#define AMBAPP_GAISLER_APBUART    0x00C
#define AMBAPP_GAISLER_IRQMP      0x00D
#define AMBAPP_GAISLER_GPTIMER    0x011
#define AMBAPP_GAISLER_GRGPIO     0x01A
#define AMBAPP_GAISLER_GRETH      0x01D
#define AMBAPP_GAISLER_AHB2AHB    0x020
#define AMBAPP_GAISLER_L2CL       0x0D0
#define AMBAPP_ESA_MCTRL          0x00F

#endif

