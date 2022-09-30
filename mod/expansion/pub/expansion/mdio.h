/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022 Martin Åberg */

#ifndef EXPANSION_MDIO_H
#define EXPANSION_MDIO_H

/* Definitions for working with Ethernet PHY */

#include <expansion/expansion.h>

struct MDIOCompat {
  unsigned long oui;
};

struct MDIODriver {
  struct ExpansionDriver base;
  struct MDIOCompat *compat;
};

struct MDIODev {
  struct ExpansionDev base;
  int phy;
  unsigned long oui;
};

enum {
  MDIO_BMCR   =  0, /* Basic mode control */
  MDIO_BMSR   =  1, /* Basic mode status */
  MDIO_PHYID1 =  2, /* PHY identifier 1 */
  MDIO_PHYID2 =  3, /* PHY identifier 2 */
  MDIO_ANAR   =  4, /* Auto-negotiation advertisement */
  MDIO_ANLPAR =  5, /* Auto-negotiation link partner ability */
};

#endif

