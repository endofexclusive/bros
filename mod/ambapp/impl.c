/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022 Martin Åberg */

#include <exec/alerts.h>
#include <exec/resident.h>
#include <exec/memory.h>
#include <exec/libcall.h>
#include <expansion/expansion.h>
#include <expansion/ambapp.h>
#include <expansion/libcall.h>
#include <stdint.h>

typedef struct ExpansionBase    Expansion;
typedef struct ExpansionDriver  Driver;
typedef struct ExpansionBus     Bus;
typedef struct ExpansionDev     Dev;

enum bar_type {
  BAR_UNKNOWN,
  BAR_APB_IO,
  BAR_AHB_MEM,
  BAR_AHB_IO,
};

struct grlib_dev {
  enum {
    GRLIB_DEV_UNKNOWN,
    GRLIB_DEV_APB_SLV,
    GRLIB_DEV_AHB_SLV,
    GRLIB_DEV_AHB_MST,
  } type;
  short vendor;
  short device;
  short irq;
  short ver;
  uint32_t user[3];
  struct {
    uint32_t addr;
    uint32_t mask;
    enum bar_type type;
  } bar[4];
};

struct ppconfig {
  uint32_t id;
  uint32_t user[3];
  uint32_t bar[4];
};

struct ioa {
  struct ppconfig master[64];
  struct ppconfig slave[63];
  uint32_t b[4];
  uint16_t build;
  uint16_t soc;
  uint32_t c[3];
};

static int id_vendor(uint32_t id) {
  return id >> 24 & 0xff;
}
static int id_device(uint32_t id) {
  return id >> 12 & 0xfff;
}
static int id_ver(uint32_t id) {
  return id >>  5 & 0x1f;
}
static int id_irq(uint32_t id) {
  return (id >> 10 & 0x3) | (id & 0x1f);
}
static int bar_type(uint32_t bar) {
  return bar >>  0 & 0xf;
}

static void maybe_set_freq(Bus *bus, volatile unsigned int *base);

static void decode_ahb(
  struct grlib_dev *gdev,
  const struct ppconfig *raw,
  uint32_t base
) {
  const uint32_t id = raw->id;

  gdev->vendor  = id_vendor(id);
  gdev->device  = id_device(id);
  gdev->irq     = id_irq(id);
  gdev->ver     = id_ver(id);
  gdev->user[0] = raw->user[0];
  gdev->user[1] = raw->user[1];
  gdev->user[2] = raw->user[2];
  for (int i = 0; i < 4; i++) {
    const uint32_t bar = raw->bar[i];

    switch (bar_type(bar)) {
      case 2:
        gdev->bar[i].type = BAR_AHB_MEM;
        gdev->bar[i].addr = bar & 0xfff00000u;
        gdev->bar[i].mask = bar << 16 & 0xfff00000u;
        break;
      case 3:
        gdev->bar[i].type = BAR_AHB_IO;
        gdev->bar[i].addr = (bar >> 12 & 0x000fff00) | base;
        gdev->bar[i].mask = bar <<  8 & 0x000fff00;
        break;
      default:
        gdev->bar[i].type = BAR_UNKNOWN;
        gdev->bar[i].addr = 0;
        gdev->bar[i].mask = 0;
    }
  }
}

static void decode_apb(
  struct grlib_dev *gdev,
  uint32_t id,
  uint32_t bar,
  uint32_t base
) {
  gdev->vendor  = id_vendor(id);
  gdev->device  = id_device(id);
  gdev->irq     = id_irq(id);
  gdev->ver     = id_ver(id);
  switch (bar_type(bar)) {
    case 1:
      gdev->bar[0].type = BAR_APB_IO;
      gdev->bar[0].addr = (bar >> 12 & 0x000fff00) | base;
      gdev->bar[0].mask = (bar <<  4 & 0x000fff00) | 0xfff00000;
      break;
    default:
      gdev->bar[0].type = BAR_UNKNOWN;
      gdev->bar[0].addr = 0;
      gdev->bar[0].mask = 0;
  }
}

static void scan_apb(Expansion *exp, Bus *bus, uint32_t base) {
  const uint32_t *const cfgarea = (void *) (base | 0x000ff000);

  /* GRIP: "The controller supports up to 16 slaves." */
  for (int i = 0; i < 16; i++) {
    struct AmbappDev *dev;
    struct grlib_dev gdev = { 0 };
    uint32_t id;
    uint32_t bar;

    id = cfgarea[2*i];
    bar = cfgarea[2*i+1];

    gdev.type = GRLIB_DEV_APB_SLV;
    decode_apb(&gdev, id, bar, base);
    if (gdev.vendor == 0) {
      continue;
    }
    if (
      gdev.vendor == AMBAPP_VENDOR_GAISLER &&
      gdev.device == AMBAPP_GAISLER_GPTIMER
    ) {
      maybe_set_freq(bus, (void *) gdev.bar[0].addr);
    }
    dev = lAllocMem(exp->exec, sizeof *dev, MEMF_ANY | MEMF_CLEAR);
    if (!dev) {
      lAlert(exp->exec, AN_Expansion | AG_NoMemory);
      continue;
    }
    dev->vendor = gdev.vendor;
    dev->device = gdev.device;
    dev->apbs.bar.addr = gdev.bar[0].addr;
    dev->apbs.bar.mask = gdev.bar[0].mask;
    dev->version = gdev.ver;
    dev->irq = gdev.irq;
    lAddExpansionDev(exp, &dev->base, bus);
  }
}

static void scan_ahb(Expansion *exp, Bus *bus, uint32_t ioarea) {
  const struct ioa *cfgarea;
  
  cfgarea = (void *) (ioarea | 0x000ff000);
  for (int i = 0; i < 63; i++) {
    struct AmbappDev *dev;
    struct grlib_dev gdev = { 0 };
    uint32_t next;

    gdev.type = GRLIB_DEV_AHB_SLV;
    decode_ahb(&gdev, &cfgarea->slave[i], ioarea);
    if (gdev.vendor == 0) {
      continue;
    }
    dev = lAllocMem(exp->exec, sizeof *dev, MEMF_ANY | MEMF_CLEAR);
    if (!dev) {
      lAlert(exp->exec, AN_Expansion | AG_NoMemory);
    } else {
      dev->vendor = gdev.vendor;
      dev->device = gdev.device;
      for (int b = 0; b < 4; b++) {
        dev->ahbs.bar[b].addr = gdev.bar[b].addr;
        dev->ahbs.bar[b].mask = gdev.bar[b].mask;
      }
      dev->version = gdev.ver;
      dev->irq = gdev.irq;
      lAddExpansionDev(exp, &dev->base, bus);
    }
    if (
      gdev.vendor == AMBAPP_VENDOR_GAISLER &&
      gdev.device == AMBAPP_GAISLER_AHB2AHB &&
      gdev.user[1]
    ) {
      scan_ahb(exp, bus, gdev.user[1]);
    } else if (
      gdev.vendor == AMBAPP_VENDOR_GAISLER &&
      gdev.device == AMBAPP_GAISLER_APBCTRL
    ) {
      next = gdev.bar[0].addr & gdev.bar[0].mask;
      scan_apb(exp, bus, next);
    }
  }
}

struct priv {
  Driver driver;
};

struct AmbappBusPriv {
  Bus                    bus;
  struct ExpansionBusOp  busop;
  long long              hz;
};

static int bus_ismatch(
  Expansion *exp,
  const Driver *edriver,
  const Dev *edev
) {
  const struct AmbappDriver *driver = (struct AmbappDriver *) edriver;
  const struct AmbappDev *dev = (struct AmbappDev *) edev;
  const struct AmbappCompat *compat = driver->compat;
  while (compat->vendor) {
    if (compat->vendor == dev->vendor &&
     compat->device == dev->device) {
      return 1;
    }
    compat++;
  }
  return 0;
}

static int bus_init0(Expansion *exp, Bus *bus) {
  static const uint32_t IOBASE = 0xfff00000;
  scan_ahb(exp, bus, IOBASE);
  return EXPANSION_OK;
}

static int bus_add_int(
  Expansion *exp,
  Dev *dev,
  struct Interrupt *interrupt,
  int intnum
) {
  /* FIXME: should remap using irqmp */
  struct AmbappDev *adev = (struct AmbappDev *) dev;
  lAddIntServer(exp->exec, interrupt, adev->irq + intnum);
  return EXPANSION_NOT_IMPLEMENTED;
}

static int bus_rem_int(
  Expansion *exp,
  Dev *dev,
  struct Interrupt *interrupt,
  int intnum
) {
  struct AmbappDev *adev = (struct AmbappDev *) dev;
  lRemIntServer(exp->exec, interrupt, adev->irq + intnum);
  return EXPANSION_NOT_IMPLEMENTED;
}

static int bus_enable_int(Expansion *exp, Dev *dev, int intnum) {
  return EXPANSION_NOT_IMPLEMENTED;
}

/* Expect boot loader to set gptimer0 to underflow every us. */
static void maybe_set_freq(Bus *bus, volatile unsigned *gptimer) {
  struct AmbappBusPriv *priv;
  unsigned int reload;

  priv = bus->dev->priv;
  if (priv->hz == 0) {
    reload = gptimer[1];
    if (reload) {
      priv->hz = (reload + 1) * 1000 * 1000;
    }
  }
}

static int bus_get_freq(Expansion *exp, Dev *dev, long long *hz) {
  struct AmbappBusPriv *priv = dev->parent->dev->priv;
  *hz = priv->hz;
  return EXPANSION_OK;
}

static int ambapp_init0(Expansion *exp, Dev *dev) {
  struct AmbappBusPriv *priv = dev->priv;
  Bus *bus;
  struct ExpansionBusOp *op;

  bus = &priv->bus;
  bus->bustype = EXPANSION_BUSTYPE_AMBAPP;
  bus->dev = dev;
  lNewList(exp->exec, &bus->children);
  op = &priv->busop;
  op->init[0]      = bus_init0,
  op->ismatch      = bus_ismatch,
  /* Set by interrupt controller driver. */
  op->add_int      = bus_add_int,
  op->rem_int      = bus_rem_int,
  op->enable_int   = bus_enable_int,
  op->disable_int  = bus_enable_int,
  op->get_freq     = bus_get_freq,
  bus->op = op;
  dev->bus = bus;

  return EXPANSION_OK;
}

static const struct ExpansionDriverOp ambapp_driverop = {
  .init[0]  = ambapp_init0,
  .fini     = NULL,
};

static int init(struct ExecBase *exec, void *data,
 struct Segment *seg) {
  Expansion *exp;
  struct priv *priv;

  exp = (Expansion *) lOpenLibrary(exec, "expansion.library", 0);
  if (exp == NULL) {
    lAlert(exec, AN_Driver | AG_OpenLib | AO_Expansion);
    return 1;
  }

  priv = data;
  priv->driver.node.name = "ambapp-root";
  priv->driver.bustype = EXPANSION_BUSTYPE_ROOT;
  priv->driver.priv_size = sizeof (struct AmbappBusPriv);
  priv->driver.op = (struct ExpansionDriverOp *) &ambapp_driverop;
  lAddExpansionDriver(exp, &priv->driver);
  lSetExpansionRoot(exp, &priv->driver);

  return 0;
}

const struct Resident RES0 = {
  .matchword            = RTC_MATCHWORD,
  .matchtag             = &RES0,
  .endskip              = &((struct Resident *) (&RES0))[1],
  .flags                = 3,
  .info.name            = "ambapp.driver",
  .info.idstring        = "ambapp.driver 0.1 2022-09-29",
  .info.type            = NT_UNKNOWN,
  .info.version         = 1,
  .pri                  = 0,
  .init.direct.f        = init,
  .init.direct.possize  = sizeof (struct priv),
};

int _start(void);
int _start(void) {
        return -1;
}

