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

struct subtimer {
  unsigned count;
  unsigned reload;
  unsigned ctrl;
  unsigned latch;
};

#define GPTIMER_CTRL_DH   (1U <<  6)
#define GPTIMER_CTRL_CH   (1U <<  5)
#define GPTIMER_CTRL_IP   (1U <<  4)
#define GPTIMER_CTRL_IE   (1U <<  3)
#define GPTIMER_CTRL_LD   (1U <<  2)
#define GPTIMER_CTRL_RS   (1U <<  1)
#define GPTIMER_CTRL_EN   (1U <<  0)

struct regs {
  unsigned scaler;
  unsigned reload;
  unsigned cfg;
  unsigned latchcfg;
  struct subtimer sub[7];
};

struct AmbappDevPriv {
  volatile struct regs *regs;
  char si;
  char timers;
};

static int init1(struct ExpansionBase *exp, struct ExpansionDev *edev) {
  struct AmbappDev *const dev = (struct AmbappDev *) edev;
  struct AmbappDevPriv *priv = edev->priv;
  unsigned cfg;

  priv->regs = (void *) dev->apbs.bar.addr;
  cfg = priv->regs->cfg;
  priv->timers = cfg & 7;
  priv->si = !!(cfg & 0x100);
  dev->base.node.name = "gptimer-dev";
  dbg("gptimer/init1: timers=%d, si=%d, reload=%d\n",
   priv->timers, priv->si, priv->regs->reload);

  return EXPANSION_OK;
}

static const struct ExpansionDriverOp driverop = {
  .init[1]      = init1,
  .fini         = NULL,
};
static const struct AmbappCompat compat[] = {
  {AMBAPP_VENDOR_GAISLER, AMBAPP_GAISLER_GPTIMER},
  {0}
};

static const struct AmbappDriver template = {
  .base.node.name = "gptimer.driver",
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
  .info.name            = "gptimer.driver",
  .info.idstring        = "gptimer.driver 0.1 2022-09-29",
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

