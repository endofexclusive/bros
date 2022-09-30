/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022 Martin Åberg */

#include <exec/alerts.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <exec/libcall.h>
#include <expansion/expansion.h>
#include <expansion/ambapp.h>
#include <expansion/mdio.h>
#include <expansion/libcall.h>

#if 1
#define dbg(...) kprintf(exp->exec, __VA_ARGS__)
static void kputchar(void *arg, int c) {
  lRawPutChar((struct ExecBase *) arg, c);
}
static void kprintf(struct ExecBase *exec, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  lRawDoFmt(exec, kputchar, exec, fmt, ap);
  va_end(ap);
}

#else
#define dbg(...)
#endif

struct regs {
  unsigned ctrl;
  unsigned stat;
  unsigned macmsb;
  unsigned maclsb;
  unsigned mdio;
  unsigned txbase;
  unsigned rxbase;
};

#define CTRL_ED            (1U << 14)
#define CTRL_DD            (1U << 12)
#define CTRL_RS            (1U <<  6)
#define CTRL_PM            (1U <<  5)
#define CTRL_FD            (1U <<  4)
#define CTRL_RI            (1U <<  3)
#define CTRL_TI            (1U <<  2)
#define CTRL_RE            (1U <<  1)
#define CTRL_TE            (1U <<  0)

#define STAT_TI            (1U <<  3)  /* Transmitter interrupt */
#define STAT_RI            (1U <<  2)  /* Receiver interrupt */
#define STAT_TE            (1U <<  1)  /* Transmitter error */
#define STAT_RE            (1U <<  0)  /* Receiver error */

#define MDIO_DATA     (0xffffU << 16) /* Data */
#define MDIO_PHYADDR  (  0x1fU << 11) /* PHY address */
#define MDIO_REGADDR  (  0x1fU <<  6) /* Register address */
#define MDIO_BU       (     1U <<  3) /* Busy */
#define MDIO_LF       (     1U <<  2) /* Linkfail */
#define MDIO_RD       (     1U <<  1) /* Read */
#define MDIO_WR       (     1U <<  0) /* Write */

struct AmbappDevPriv {
  struct ExpansionBus    bus;
  volatile struct regs  *regs;
};

/* phy: 0..31, reg: 0..31, val: read value */
static int read(
  volatile struct regs *regs,
  int phy,
  int reg,
  unsigned short *val
) {
  unsigned mdio = 0;
  /* "worst case" 100 MDIO cycles at CPU/MDC ratio 512 */
  const int MDIOWAIT = 100 * 512;

  for (volatile int i = 0; i < MDIOWAIT; i++) {
    mdio = regs->mdio;
    if ((mdio & MDIO_BU) == 0) {
      break;
    }
  }
  if (mdio & MDIO_BU) {
    return EXPANSION_IO_ERROR;
  }
  regs->mdio = phy << 11 | reg << 6 | MDIO_RD;
  for (volatile int i = 0; i < MDIOWAIT; i++) {
    mdio = regs->mdio;
    if ((mdio & MDIO_BU) == 0) {
      break;
    }
  }
  if ((mdio & MDIO_LF) || (mdio & MDIO_BU)) {
    return EXPANSION_IO_ERROR;
  }
  *val = mdio >> 16;
  return EXPANSION_OK;
}

static int bus_init1(struct ExpansionBase *exp,
 struct ExpansionBus *bus) {
  struct AmbappDevPriv *priv = bus->dev->priv;

  for (int i = 0; i < 32; i++) {
    struct MDIODev *dev;
    unsigned short id;
    unsigned long oui;
    int ret;

    ret = read(priv->regs, i, MDIO_PHYID1, &id);
    if (ret) {
      continue;
    }
    oui = id << 6 & 0xfffc0;
    ret = read(priv->regs, i, MDIO_PHYID2, &id);
    if (ret) {
      continue;
    }
    oui |= id >> 10 & 0x3f;
    if (oui == 0 || oui == 0xfffff) {
      continue;
    }

    dev = lAllocMem(exp->exec, sizeof *dev, MEMF_ANY | MEMF_CLEAR);
    if (!dev) {
      lAlert(exp->exec, AN_Expansion | AG_NoMemory);
      continue;
    }
    dev->oui = oui;
    dbg("%s: add PHY oui=%05lx\n", __func__, (unsigned long) oui);
    lAddExpansionDev(exp, &dev->base, bus);
  }

  return EXPANSION_OK;
}

/* MDIO bus driver */
static const struct ExpansionBusOp busop = {
  .init[1]      = bus_init1,
  .fini         = NULL,
};
static const struct ExpansionBus bustemplate = {
  .bustype  = EXPANSION_BUSTYPE_MDIO,
  .op       = (struct ExpansionBusOp *) &busop,
};

static int init1(struct ExpansionBase *exp, struct ExpansionDev *edev) {
  struct AmbappDev *const dev = (struct AmbappDev *) edev;
  struct AmbappDevPriv *priv = edev->priv;
  struct ExpansionBus *bus;

  priv->regs = (void *) dev->apbs.bar.addr;
  dev->base.node.name = "greth-dev";
  dbg("greth/init1:\n");

  bus = &priv->bus;
  *bus = bustemplate;
  bus->dev = &dev->base;
  lNewList(exp->exec, &bus->children);
  dev->base.bus = bus;

  return EXPANSION_OK;
}

static const struct ExpansionDriverOp driverop = {
  .init[1]      = init1,
  .fini         = NULL,
};
static const struct AmbappCompat compat[] = {
  {AMBAPP_VENDOR_GAISLER, AMBAPP_GAISLER_GRETH},
};
static const struct AmbappDriver drivertemplate = {
  .base.node.name = "greth.driver",
  .base.bustype   = EXPANSION_BUSTYPE_AMBAPP,
  .base.priv_size = sizeof (struct AmbappDevPriv),
  .base.op        = (struct ExpansionDriverOp *) &driverop,
  .compat         = (struct AmbappCompat *) compat,
};

static int init(
  struct ExecBase *exec,
  void *data,
  struct Segment *seg
) {
  struct ExpansionBase *exp;
  struct AmbappDriver *driver;

  driver = data;
  *driver = drivertemplate;

  exp = (struct ExpansionBase *)
   lOpenLibrary(exec, "expansion.library", 0);
  if (exp == NULL) {
    lAlert(exec, AN_Driver | AG_OpenLib | AO_Expansion);
    return 1;
  }
  lAddExpansionDriver(exp, &driver->base);

  return 0;
}

const struct Resident RES0 = {
  .matchword            = RTC_MATCHWORD,
  .matchtag             = &RES0,
  .endskip              = &((struct Resident *) (&RES0))[1],
  .flags                = 3,
  .info.name            = "greth.driver",
  .info.idstring        = "greth.driver 0.1 2022-09-29",
  .info.type            = NT_UNKNOWN,
  .info.version         = 1,
  .pri                  = 0,
  .init.direct.f        = init,
  .init.direct.possize  = sizeof (struct AmbappDriver),
};

int _start(void);
int _start(void) {
        return -1;
}

