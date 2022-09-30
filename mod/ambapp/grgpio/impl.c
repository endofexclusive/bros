/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022 Martin Åberg */

#include <exec/alerts.h>
#include <exec/resident.h>
#include <exec/libcall.h>
#include <expansion/expansion.h>
#include <expansion/ambapp.h>
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
  unsigned data;      /* 00 I/O port data */
  unsigned output;    /* 04 I/O port output */
  unsigned dir;       /* 08 I/O port direction */
  unsigned imask;     /* 0c Interrupt mask */
  unsigned ipol;      /* 10 Interrupt polarity */
  unsigned iedge;     /* 14 Interrupt edge */
  unsigned bypass;    /* 18 Bypass */
  unsigned cap;       /* 1c Capability */
  unsigned irqmap[8]; /* 20..3c Interrupt map */
};

struct AmbappDevPriv {
  volatile struct regs *regs;
  char nlines;
  char irqgen;
};

/* ver = 2 (or later) and cap.ifl=1 required */
static int init1(struct ExpansionBase *exp, struct ExpansionDev *edev) {
  struct AmbappDev *const dev = (struct AmbappDev *) edev;
  struct AmbappDevPriv *priv = edev->priv;
  unsigned cap;

  if (dev->version < 2) {
    return EXPANSION_UNSUPPORTED_HW;
  }
  priv->regs = (void *) dev->apbs.bar.addr;
  cap = priv->regs->cap;
  if ((cap & 1<<16) == 0) {
    return EXPANSION_UNSUPPORTED_HW;
  }
  priv->nlines = ((cap >>  0) & 0x1f) + 1;
  priv->irqgen = (cap >>  8) & 0x1f;
  dev->base.node.name = "grgpio-dev";
  dbg("grgpio/init1: nlines=%d, irqgen=%d\n",
   priv->nlines, priv->irqgen);

  return EXPANSION_OK;
}

static const struct AmbappDriver template = {
  .base.node.name = "grgpio.driver",
  .base.bustype = EXPANSION_BUSTYPE_AMBAPP,
  .base.priv_size = sizeof (struct AmbappDevPriv),
  .base.op =
   (struct ExpansionDriverOp *) &(const struct ExpansionDriverOp) {
    .init[1]      = init1,
    .fini         = NULL,
  },
  .compat =
   (struct AmbappCompat *) (const struct AmbappCompat []) {
    {AMBAPP_VENDOR_GAISLER, AMBAPP_GAISLER_GRGPIO},
    {0}
  },
};

static int init(
  struct ExecBase *exec,
  void *data,
  struct Segment *seg
) {
  struct ExpansionBase *exp;
  struct AmbappDriver *driver;

  driver = data;
  *driver = template;

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
  .info.name            = "grgpio.driver",
  .info.idstring        = "grgpio.driver 0.1 2022-09-29",
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

