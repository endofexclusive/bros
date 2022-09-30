/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022 Martin Åberg */

#include <exec/libcall.h>
#include <exec/alerts.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <expansion/expansion.h>
#include "internal.h"

typedef struct ExpansionBase    Lib;
typedef struct ExpansionDriver  Driver;
typedef struct ExpansionBus     Bus;
typedef struct ExpansionDev     Dev;

int iAddExpansionDriver(Lib *exp, Driver *driver) {
  driver->node.type = NT_EXPANSION_DRIVER;
  lObtainMutex(exp->exec, &exp->lock);
  lAddTail(exp->exec, &exp->drivers, &driver->node);
  lReleaseMutex(exp->exec, &exp->lock);
  return EXPANSION_OK;
}

int iSetExpansionRoot(Lib *exp, Driver *driver) {
  lObtainMutex(exp->exec, &exp->lock);
  exp->root.node.type = NT_EXPANSION_DEVICE;
  exp->root.parent = NULL;
  exp->root.driver = driver;
  lReleaseMutex(exp->exec, &exp->lock);
  return EXPANSION_OK;
}

int iAddExpansionDev(Lib *exp, Dev *dev, Bus *parent) {
  Driver *driver;
  int bustype;

  dev->node.type = NT_EXPANSION_DEVICE;
  dev->parent = parent;
  bustype = parent->bustype;

  lObtainMutex(exp->exec, &exp->lock);
  driver = (Driver *) exp->drivers.head;
  while (driver->node.succ) {
    int match = 0;

    if (driver->bustype == bustype) {
      if (parent->op->ismatch(exp, driver, dev)) {
        match = 1;
        /* let driver decide if it can handle this device */
        if (driver->op && driver->op->ismatch) {
          match = driver->op->ismatch(exp, driver, dev);
        }
      }
    }
    if (match) {
      dev->driver = driver;
      break;
    }
    driver = (Driver *) driver->node.succ;
  }
  lAddTail(exp->exec, &parent->children, &dev->node);
  lReleaseMutex(exp->exec, &exp->lock);

  return EXPANSION_OK;
}

static int upbus(Lib *exp, Bus *bus, int maxlevel) {
  Dev *child;

  if (bus->ret != EXPANSION_OK) {
    return EXPANSION_OK;
  }
  for (int level = bus->initlevel; level < maxlevel; level++) {
    if (bus->op && bus->op->init[level]) {
      bus->ret = bus->op->init[level](exp, bus);
      if (bus->ret != EXPANSION_OK) {
        lAlert(exp->exec, AN_Expansion);
        return EXPANSION_OK;
      }
    }
    bus->initlevel = level + 1;
  }

  child = (Dev *) bus->children.head;
  while (child->node.succ) {
    if (child->bus) {
      upbus(exp, child->bus, maxlevel);
    }
    child = (Dev *) child->node.succ;
  }

  return EXPANSION_OK;
}

static int updev(Lib *exp, Dev *dev, int maxlevel, int *addbus) {
  Dev *child;

  if (dev->ret != EXPANSION_OK) {
    return EXPANSION_OK;
  }

  for (int level = dev->initlevel; level < maxlevel; level++) {
    Driver *driver;

    driver = dev->driver;
    if (driver) {
      if (dev->priv == NULL && dev->driver->priv_size) {
        dev->priv = lAllocMem(
          exp->exec,
          driver->priv_size,
          MEMF_CLEAR | MEMF_ANY
        );
        if (dev->priv == NULL) {
          dev->ret = EXPANSION_NOT_ENOUGH_MEMORY;
          lAlert(exp->exec, AT_DeadEnd | AN_Expansion);
          return EXPANSION_OK;
        }
      }
      if (driver->op && driver->op->init[level]) {
        Bus *bus;

        bus = dev->bus;
        dev->ret = driver->op->init[level](exp, dev);
        if (dev->ret != EXPANSION_OK) {
          lAlert(exp->exec, AN_Expansion);
          return EXPANSION_OK;
        }
        if (bus == NULL && dev->bus) {
          (*addbus)++;
        }
      }
    }
    dev->initlevel = level + 1;
  }

  if (dev->bus == NULL) {
    return EXPANSION_OK;
  }

  child = (Dev *) dev->bus->children.head;
  while (child->node.succ) {
    updev(exp, child, maxlevel, addbus);
    child = (Dev *) child->node.succ;
  }

  return EXPANSION_OK;
}

int iUpdateExpansion(Lib *exp) {
  for (int i = 1; i <= EXPANSION_INIT_LEVELS; i++) {
    Dev *dev;
    Bus *bus;
    int addbus;

  again:
    addbus = 0;
    dev = &exp->root;
    bus = dev->bus;
    if (bus) {
      upbus(exp, dev->bus, i);
    }
    updev(exp, dev, i, &addbus);
    if (addbus) {
      goto again;
    }
  }
  return EXPANSION_OK;
}

static struct ExpansionBusOp *getop(Dev *dev) {
  Bus *bus;
  struct ExpansionBusOp *op;
  if ((bus = dev->parent)) {
    if ((op = bus->op)) {
      return op;
    }
  }
  return NULL;
}

int iGetDevFreq(Lib *exp, Dev *dev, long long *hz) {
  struct ExpansionBusOp *op;
  op = getop(dev);
  if (op && op->get_freq) {
    return op->get_freq(exp, dev, hz);
  }
  return EXPANSION_NOT_IMPLEMENTED;
}

int iAddDevInt(Lib *exp, Dev *dev, struct Interrupt *in, int num) {
  struct ExpansionBusOp *op;
  op = getop(dev);
  if (op && op->add_int) {
    return op->add_int(exp, dev, in, num);
  }
  return EXPANSION_NOT_IMPLEMENTED;
}

int iRemDevInt(Lib *exp, Dev *dev, struct Interrupt *in, int num) {
  struct ExpansionBusOp *op;
  op = getop(dev);
  if (op && op->rem_int) {
    return op->rem_int(exp, dev, in, num);
  }
  return EXPANSION_NOT_IMPLEMENTED;
}

struct Segment *iExpungeBase(struct Library *lib) {
  return NULL;
}

void iCloseBase(struct Library *lib) {
}

struct Library *iOpenBase(struct Library *lib) {
  return lib;
}

static struct Library *init(
  struct ExecBase *exec,
  struct Library *library,
  struct Segment *segment
) {
  Lib *lib;

  lib = (Lib *) library;
  lib->exec = exec;
  lib->lib.opencount++;
  lInitMutex(exec, &lib->lock);
  lNewList(exec, &lib->drivers);
  lib->root.node.name = "root-dev";

  return library;
}

const struct Resident RES0 = {
  .matchword          = RTC_MATCHWORD,
  .matchtag           = &RES0,
  .endskip            = &((struct Resident *) (&RES0))[1],
  .info.name          = "expansion.library",
  .info.idstring      = "expansion 0.1 2022-09-29",
  .info.type          = NT_LIBRARY,
  .info.version       = 1,
  .pri                = 4,
  .flags              = RTF_AUTOINIT | 2,
  .init.iauto.f       = init,
  .init.iauto.optable = &optemplate,
  .init.iauto.opsize  = sizeof (struct ExpansionBaseOp),
  .init.iauto.possize = sizeof (Lib),
};

int _start(void);
int _start(void) { return -1; }

