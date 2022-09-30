/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022 Martin Åberg */

#ifndef EXPANSION_EXPANSION_H
#define EXPANSION_EXPANSION_H

#include <stddef.h>
#include <exec/libraries.h>
#include <exec/lists.h>
#include <exec/mutex.h>

struct IORequest;
struct Interrupt;
struct ExpansionBase;
struct ExpansionDriver;
struct ExpansionBus;
struct ExpansionDev;

enum {
  EXPANSION_OK,
  EXPANSION_NOT_ENOUGH_MEMORY,
  EXPANSION_UNSUPPORTED_HW,
  EXPANSION_IO_ERROR,
  EXPANSION_NOT_IMPLEMENTED,
};

enum {
  NT_EXPANSION_DRIVER = NT_CUSTOM,
  NT_EXPANSION_DEVICE,
  NT_EXPANSION_BUS,
};

enum {
  EXPANSION_BUSTYPE_UNKNOWN,
  EXPANSION_BUSTYPE_ROOT,
  EXPANSION_BUSTYPE_MDIO,
  EXPANSION_BUSTYPE_I2C,
  EXPANSION_BUSTYPE_PCI,
  EXPANSION_BUSTYPE_AMBAPP,
  EXPANSION_BUSTYPE_OTHER,
  EXPANSION_BUSTYPE_CUSTOM = 128,
};

#define EXPANSION_INIT_LEVELS 3

struct ExpansionDriverOp {
  /* for expansion */
  int (*init[EXPANSION_INIT_LEVELS])(
    struct ExpansionBase *exp,
    struct ExpansionDev *dev
  );
  int (*fini)(
    struct ExpansionBase *exp,
    struct ExpansionDev *dev
  );
  int (*ismatch)(
    struct ExpansionBase *exp,
    const struct ExpansionDriver *driver,
    const struct ExpansionDev *dev
  );
};

/* on expansion global driver list */
struct ExpansionDriver {
  /* name: name, type: NT_EXPANSION_DRIVER */
  struct Node  node;
  int          bustype;   /* EXPANSION_BUSTYPE_ */
  size_t       priv_size; /* Allocated for each dev->priv. */
  struct ExpansionDriverOp *op;
};

struct ExpansionBusOp {
  /* for expansion */
  int (*init[EXPANSION_INIT_LEVELS])(
    struct ExpansionBase *exp,
    struct ExpansionBus *bus
  );
  int (*fini)(
    struct ExpansionBase *exp,
    struct ExpansionBus *bus
  );
  int (*ismatch)(
    struct ExpansionBase *exp,
    const struct ExpansionDriver *driver,
    const struct ExpansionDev *dev
  );
  /* for driver */
  int (*add_int)(
    struct ExpansionBase *exp,
    struct ExpansionDev *dev,
    struct Interrupt *interrupt,
    int intnum
  );
  int (*rem_int)(
    struct ExpansionBase *exp,
    struct ExpansionDev *dev,
    struct Interrupt *interrupt,
    int intnum
  );
  int (*enable_int)(
    struct ExpansionBase *exp,
    struct ExpansionDev *dev,
    int intnum
  );
  int (*disable_int)(
    struct ExpansionBase *exp,
    struct ExpansionDev *dev,
    int intnum
  );
  int (*get_freq)(
    struct ExpansionBase *exp,
    struct ExpansionDev *dev,
    long long *hz
  );
};

struct ExpansionBus {
  int                    bustype;   /* EXPANSION_BUSTYPE_ */
  struct ExpansionDev   *dev;       /* dev->bus is us */
  struct List            children;
  struct ExpansionBusOp *op;
  int                    initlevel;
  int                    ret;
};

/* a specific device and driver pairing on bus children list */
struct ExpansionDev {
  /* name: name, type: NT_EXPANSION_DEVICE */
  struct Node              node;
  struct ExpansionDriver  *driver;
  struct ExpansionBus     *parent;
  /* instance state managed by driver: driver->priv_size bytes */
  void                    *priv;
  struct ExpansionBus     *bus; /* Iff this dev spawns a bus */
  int                      initlevel;
  int                      ret;
};

struct ExpansionBase {
  struct Library       lib;
  struct ExecBase     *exec;
  struct Mutex         lock;
  struct ExpansionDev  root;
  /* all registered struct ExpansionDriver */
  struct List          drivers;
  int                  initlevel;
};

#endif

