/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022 Martin Åberg */

/* Driver for GRLIB APBUART */

#include <devices/serial.h>
#include <exec/exec.h>
#include <exec/libcall.h>
#include <expansion/ambapp.h>
#include <expansion/expansion.h>
#include <expansion/libcall.h>

#define dbg(...)

#define NELEM(a) ((sizeof a) / (sizeof (a[0])))

static const struct Resident RES0;

struct ThisDriver {
  struct AmbappDriver ambappdriver;
  struct List units; /* AmbappDevPriv */
  /* Can add the exec Device node here. */
};

struct AmbappDevPriv {
  struct Node node; /* ThisDriver.units */
  struct AmbappDev *dev;
};

static int init2(struct ExpansionBase *exp,
 struct ExpansionDev *edev) {
  lInitResident(exp->exec, &RES0, (void *) edev->driver);
  return EXPANSION_OK;
}

static int init1(struct ExpansionBase *exp, struct ExpansionDev *edev) {
  struct ThisDriver *const driver = (struct ThisDriver *) edev->driver;
  struct AmbappDev *const dev = (struct AmbappDev *) edev;
  struct AmbappDevPriv *priv = edev->priv;

  priv->dev = dev;
  dev->base.node.name = "apbuart-dev";
  lAddTail(exp->exec, &driver->units, &priv->node);

  return EXPANSION_OK;
}

static const struct AmbappDriver template = {
  .base.node.name = "apbuart.driver",
  .base.bustype = EXPANSION_BUSTYPE_AMBAPP,
  .base.priv_size = sizeof (struct AmbappDevPriv),
  .base.op =
   (struct ExpansionDriverOp *) &(const struct ExpansionDriverOp) {
    .init[1]      = init1,
    .init[2]      = init2,
    .fini         = NULL,
  },
  .compat =
   (struct AmbappCompat *) (const struct AmbappCompat []) {
    {AMBAPP_VENDOR_GAISLER, AMBAPP_GAISLER_APBUART},
    {0}
  },
};

static int init(
  struct ExecBase *exec,
  void *data,
  struct Segment *seg
) {
  struct ExpansionBase *exp;
  struct ThisDriver *driver;

  driver = data;
  driver->ambappdriver = template;
  lNewList(exec, &driver->units);

  exp = (struct ExpansionBase *) lOpenLibrary(exec, "expansion.library", 0);
  if (exp == NULL) {
    lAlert(exec, AN_Driver | AG_OpenLib | AO_Expansion);
    return 1;
  }
  lAddExpansionDriver(exp, &driver->ambappdriver.base);

  return 0;
}

const struct Resident RES_EXP = {
  .matchword            = RTC_MATCHWORD,
  .matchtag             = &RES_EXP,
  .endskip              = &((struct Resident *) (&RES_EXP))[1],
  .flags                = 3,
  .info.name            = "apbuart.driver",
  .info.idstring        = "apbuart.driver 0.1 2022-09-29",
  .info.type            = NT_UNKNOWN,
  .info.version         = 1,
  .pri                  = 0,
  .init.direct.f        = init,
  .init.direct.possize  = sizeof (struct ThisDriver),
};

int _start(void);
int _start(void) {
        return -1;
}

struct regs {
  unsigned int data;
  unsigned int status;
  unsigned int ctrl;
  unsigned int scaler;
};

/* Control register */
#define APBUART_CTRL_FA         (1 << 31)
#define APBUART_CTRL_DB         (1 << 11)
#define APBUART_CTRL_RF         (1 << 10)
#define APBUART_CTRL_TF         (1 <<  9)
#define APBUART_CTRL_LB         (1 <<  7)
#define APBUART_CTRL_FL         (1 <<  6)
#define APBUART_CTRL_PE         (1 <<  5)
#define APBUART_CTRL_PS         (1 <<  4)
#define APBUART_CTRL_TI         (1 <<  3)
#define APBUART_CTRL_RI         (1 <<  2)
#define APBUART_CTRL_TE         (1 <<  1)
#define APBUART_CTRL_RE         (1 <<  0)

/* Status register */
#define APBUART_STATUS_RF       (1 << 10)
#define APBUART_STATUS_TF       (1 <<  9)
#define APBUART_STATUS_RH       (1 <<  8)
#define APBUART_STATUS_TH       (1 <<  7)
#define APBUART_STATUS_FE       (1 <<  6)
#define APBUART_STATUS_PE       (1 <<  5)
#define APBUART_STATUS_OV       (1 <<  4)
#define APBUART_STATUS_BR       (1 <<  3)
#define APBUART_STATUS_TE       (1 <<  2)
#define APBUART_STATUS_TS       (1 <<  1)
#define APBUART_STATUS_DR       (1 <<  0)

struct DevBase {
  struct Device    Device;
  struct ExecBase *exec;
  struct ExpansionBase *exp;
  struct ThisDriver *edrv;
  struct List      unitlist;
  struct Mutex     unitlock;
};

struct DevUnit {
  struct Node        node;
  struct AmbappDev  *adev;
  volatile struct regs *regs;
  struct Interrupt   interrupt;
  struct {
    struct List        list;
    struct IntLock     lock;
    int                r;
    int                w;
    unsigned char      buf[512];
  } rx;
  struct {
    struct List        list;
    struct IntLock     lock;
  } tx;
  int                num;
  int                fifo_available;
};

static struct Library *serinit(
  struct ExecBase *exec,
  struct Library *lib,
  struct Segment *segment
) {
  struct DevBase *dev = (struct DevBase *) lib;
  struct ExpansionBase *exp;

  exp = (struct ExpansionBase *)
   lOpenLibrary(exec, "expansion.library", 0);
  if (exp == NULL) {
    lAlert(exec, AN_Driver | AG_OpenLib | AO_Expansion);
    return NULL;
  }
  dev->exp = exp;

  lib->opencount++;
  dev->exec = exec;
  dev->edrv = (struct ThisDriver *) segment;
  lNewList(exec, &dev->unitlist);
  lInitMutex(exec, &dev->unitlock);

  return &dev->Device.lib;
}

static void iAbortIO(struct Device *dev, struct IORequest *ior) {
  struct DevBase *d = (struct DevBase *) dev;
  struct ExecBase *exec = d->exec;
  struct DevUnit *u = (struct DevUnit *) ior->unit;
  struct IOExtSer *req = (struct IOExtSer *) ior;
  int abort = 0;

  switch (req->ior.command) {
    case CMD_WRITE:
      lObtainIntLock(exec, &u->tx.lock);
      if (req->ior.message.node.type == NT_MESSAGE) {
        lRemove(exec, &req->ior.message.node);
        abort = 1;
      }
      lReleaseIntLock(exec, &u->tx.lock);
      break;
    case CMD_READ:
      lObtainIntLock(exec, &u->rx.lock);
      if (req->ior.message.node.type == NT_MESSAGE) {
        lRemove(exec, &req->ior.message.node);
        abort = 1;
      } else {
        /* already replied or in the process of being replied in isr */
      }
      lReleaseIntLock(exec, &u->rx.lock);
      break;
    default:
      break;
  }
  if (abort) {
    req->ior.error = IOERR_ABORTED;
    if (!(req->ior.flags & IOF_QUICK)) {
      lReplyMsg(exec, &req->ior.message);
    }
  }
}

static void inithw(struct DevUnit *u) {
  volatile struct regs *const regs = u->regs;
  unsigned int ctrl;

  ctrl = regs->ctrl;
  u->fifo_available = !!(ctrl & APBUART_CTRL_FA);
  /* CTRL_FL reset value is 0. If it is 1 we keep debug config. */
  if (ctrl & APBUART_CTRL_FL) {
    ctrl &= (APBUART_CTRL_FL | APBUART_CTRL_LB | APBUART_CTRL_DB);
  } else {
    ctrl = 0;
  }
  ctrl |= APBUART_CTRL_RI | APBUART_CTRL_TE | APBUART_CTRL_RE;
  regs->status = 0;
  regs->ctrl = ctrl;
}

static void finihw(struct DevUnit *u) {
  volatile struct regs *const regs = u->regs;
  unsigned int ctrl;

  /* Wait for transmitter shift register empty condition. */
  while ((regs->status & APBUART_STATUS_TS) == 0) {
    for (volatile int i = 0; i < 1234; i++);
  }
  ctrl = regs->ctrl;
  if (ctrl & APBUART_CTRL_DB) {
    ctrl &= ~(APBUART_CTRL_RI | APBUART_CTRL_TI);
  } else {
    /* NOTE: May break lRawPutChar() */
    ctrl = 0;
  }
  regs->ctrl = ctrl;
}

/* HW user interface depends on synthesis option... */
static int txfull(struct DevUnit *u, unsigned int status) {
  if (u->fifo_available) {
    return status & APBUART_STATUS_TF;
  } else {
    return !(status & APBUART_STATUS_TE);
  }
}

static void isr_tx(
  struct ExecBase *exec,
  struct DevUnit *u,
  volatile struct regs *const regs
) {
  lObtainIntLock(exec, &u->tx.lock);
  while (1) {
    struct IOExtSer *req = (struct IOExtSer *) lGetHead(exec, &u->tx.list);
    if (req == NULL) {
      regs->ctrl &= ~APBUART_CTRL_TI;
      break;
    }
    unsigned char *reqbuf = req->data;
    while (req->actual < req->length &&
     (!txfull(u, regs->status))) {
      regs->data = (unsigned int) reqbuf[req->actual++];
    }
    if (req->actual == req->length) {
      lRemove(exec, &req->ior.message.node);
      req->ior.message.node.type = NT_FREEMSG;
      lReleaseIntLock(exec, &u->tx.lock);
      lReplyMsg(exec, &req->ior.message);
      dbg("%s: replied\n", __func__);
      lObtainIntLock(exec, &u->tx.lock);
    }
    if (txfull(u, regs->status)) {
        break;
    }
  }
  lReleaseIntLock(exec, &u->tx.lock);
}

/* INVARIANT: rx.buf  has data   => rx.list is empty */
/* INVARIANT: rx.list has req(s) => rx.buf  is empty */
static void isr_rx(
  struct ExecBase *exec,
  struct DevUnit *u,
  volatile struct regs *const regs
) {
  unsigned char rxdata[32];
  int nrx = 0;

  while ((nrx < (int) NELEM(rxdata)) &&
   (regs->status & APBUART_STATUS_DR)) {
    rxdata[nrx] = regs->data;
    nrx++;
  }
  if (nrx == 0) {
    return;
  }

  int i = 0;
  lObtainIntLock(exec, &u->rx.lock);
  while (1) {
    struct IOExtSer *req = (struct IOExtSer *) lGetHead(exec, &u->rx.list);
    if (req == NULL) {
      break;
    }
    /* rx.buf is empty */
    unsigned char *reqbuf = req->data;
    while (req->actual < req->length && i < nrx) {
      reqbuf[req->actual++] = rxdata[i++];
    }
    if (req->actual == req->length) {
      lRemove(exec, &req->ior.message.node);
      req->ior.message.node.type = NT_FREEMSG;

      lReleaseIntLock(exec, &u->rx.lock);
      /*
       * NOTE: AbortIO(req) may be called here.  If AbortIO() sees NT_FREEMSG
       * (after taking the lock), then it will ignore the abort and WaitIO()
       * will soon see NT_REPLYMSG.
       *
       * NOTE: cmd_read() may be called here to add new requests on rx.list.
       */
      lReplyMsg(exec, &req->ior.message);
      dbg("%s: replied\n", __func__);
      lObtainIntLock(exec, &u->rx.lock);
    } else {
      break;
    }
  }
  int full = 0;
  while (i < nrx) {
    /* rx.list is empty */
    int diff = u->rx.w - u->rx.r;
    if (diff < 0) {
      diff += NELEM(u->rx.buf);
    }
    if ((int) (NELEM(u->rx.buf)-1) <= diff) {
      full = 1;
      break;
    }
    u->rx.buf[u->rx.w++] = rxdata[i++];
    if (u->rx.w == NELEM(u->rx.buf)) {
      u->rx.w = 0;
    }
  }
  lReleaseIntLock(exec, &u->rx.lock);
  if (full) {
    dbg("%s: full\n", __func__);
  }
}

static void isr(struct ExecBase *exec, void *data) {
  struct DevUnit *u = (struct DevUnit *) data;
  volatile struct regs *const regs = u->regs;
  isr_rx(exec, u, regs);
  isr_tx(exec, u, regs);
}

static int cmd_write(struct DevBase *d, struct DevUnit *u,
 struct IOExtSer *req) {
  req->actual = 0;
  if (req->length == 0) {
    return 0;
  }

  struct ExecBase *exec = d->exec;
  volatile struct regs *const regs = u->regs;
  lObtainIntLock(exec, &u->tx.lock);
  if (!lGetHead(exec, &u->tx.list)) {
    unsigned char *reqbuf = req->data;
    while (req->actual < req->length &&
     (!txfull(u, regs->status))) {
      regs->data = (unsigned int) reqbuf[req->actual++];
    }
    if (req->actual == req->length) {
      lReleaseIntLock(exec, &u->tx.lock);
      return 0;
    }
  }
  req->ior.flags &= ~IOF_QUICK;
  lAddTail(exec, &u->tx.list, &req->ior.message.node);
  regs->ctrl |= APBUART_CTRL_TI;
  lReleaseIntLock(exec, &u->tx.lock);

  return 1;
}

static int cmd_read(struct DevBase *d, struct DevUnit *u,
 struct IOExtSer *req) {
  req->actual = 0;
  if (req->length == 0) {
    return 0;
  }

  struct ExecBase *exec = d->exec;
  unsigned char *reqbuf = req->data;
  int diff;

  lObtainIntLock(exec, &u->rx.lock);
  diff = u->rx.w - u->rx.r;
  if (diff < 0) {
    diff += NELEM(u->rx.buf);
  }

  while (req->actual < req->length && diff) {
    reqbuf[req->actual++] = u->rx.buf[u->rx.r++];
    if (u->rx.r == NELEM(u->rx.buf)) {
      u->rx.r = 0;
    }
    diff--;
  }
  if (req->actual == req->length) {
    lReleaseIntLock(exec, &u->rx.lock);
    return 0;
  }
  /*
   * NOTE: Must AddTail _before_ releasing lock otherwise new data can arrive
   * at rx.buf.
   */
  req->ior.flags &= ~IOF_QUICK;
  lAddTail(exec, &u->rx.list, &req->ior.message.node);
  lReleaseIntLock(exec, &u->rx.lock);
  if (diff == 0) {
    dbg("%s: empty\n", __func__);
  }

  return 1;
}

static long getbaud(struct DevBase *dev, struct DevUnit *u) {
  long long hz = 0;
  if (lGetDevFreq(dev->exp, &u->adev->base, &hz)) {
    return -1;
  }
  return hz / (8 * (u->regs->scaler + 1));
}

static void iBeginIO(struct Device *dev, struct IORequest *ior) {
  struct DevBase *d = (struct DevBase *) dev;
  struct DevUnit *u = (struct DevUnit *) ior->unit;
  struct IOExtSer *req = (struct IOExtSer *) ior;
  int req_is_queued = 0;

  /* Can be NT_REPLYMSG at return from BeginIO() only if replied. */
  req->ior.message.node.type = NT_MESSAGE;
  req->ior.error = IOERR_OK;
  switch (req->ior.command) {
    case CMD_WRITE:
      req_is_queued = cmd_write(d, u, req);
      break;
    case CMD_READ:
      req_is_queued = cmd_read(d, u, req);
      break;
    case SDCMD_QUERY:
      lObtainIntLock(d->exec, &u->rx.lock);
      {
        int diff = u->rx.w - u->rx.r;
        if (diff < 0) {
          diff += NELEM(u->rx.buf);
        }
        req->actual = diff;
      }
      lReleaseIntLock(d->exec, &u->rx.lock);
      break;
    case SDCMD_GETPARAMS:
      req->baud = getbaud(d, u);
      break;
    default:
      req->ior.error = IOERR_NOCMD;
      break;
  }
  /*
   * Do not access req if it was queued: it may be accessed by ISR right now
   * or may already be replied.
   */

  if (req_is_queued) {
    ;
  } else if (!(req->ior.flags & IOF_QUICK)) {
    lReplyMsg(d->exec, &req->ior.message);
  }
}

static void iClose(struct Device *dev, struct IORequest *ior) {
  struct DevBase *d = (struct DevBase *) dev;
  struct ExecBase *exec = d->exec;
  struct DevUnit *u = (struct DevUnit *) ior->unit;

  dbg("%s: entry\n", __func__);
  lObtainMutex(exec, &d->unitlock);
  lRemDevInt(d->exp, &u->adev->base, &u->interrupt, 0);
  finihw(u);
  lRemove(exec, &u->node);
  ior->unit = NULL;
  lFreeMem(exec, u, sizeof *u);
  lReleaseMutex(exec, &d->unitlock);
}

static struct AmbappDev *findadev(struct DevBase *d, int unitnum) {
  struct Node *node;
  int i = 0;

  for (node = d->edrv->units.head; node->succ; node = node->succ) {
    if (i == unitnum) {
      return ((struct AmbappDevPriv *) node)->dev;
    }
    i++;
  }
  return NULL;
}

static struct DevUnit *findunit(struct DevBase *d, int unitnum) {
  for (struct Node *node = d->unitlist.head; node->succ; node = node->succ) {
    struct DevUnit *const u = (struct DevUnit *) node;
    if (u->num == unitnum) {
      return u;
    }
  }
  return NULL;
}

static void iOpen(
  struct Device *dev,
  struct IORequest *ior,
  int unitnum
)
{
  struct DevBase *d = (struct DevBase *) dev;
  struct ExecBase *exec = d->exec;
  struct AmbappDev *adev = NULL;
  struct DevUnit *u;

  dbg("%s: entry\n", __func__);
  adev = findadev(d, unitnum);
  if (adev == NULL) {
    ior->error = IOERR_INVALIDUNIT;
    return;
  }

  lObtainMutex(exec, &d->unitlock);
  u = findunit(d, unitnum);
  if (u) {
    lReleaseMutex(exec, &d->unitlock);
    ior->error = IOERR_UNITBUSY;
    return;
  }

  u = lAllocMem(exec, sizeof *u, MEMF_ANY | MEMF_CLEAR);
  if (u == NULL) {
    lReleaseMutex(exec, &d->unitlock);
    ior->error = IOERR_OPENFAIL;
    return;
  }
  ior->unit = (struct Unit *) u;
  u->adev = adev;

  lNewList(exec, &u->rx.list);
  lInitIntLock(exec, &u->rx.lock);
  lNewList(exec, &u->tx.list);
  lInitIntLock(exec, &u->tx.lock);

  u->regs = (void *) adev->apbs.bar.addr;
  u->num = unitnum;
  u->interrupt.node.name = RES0.info.name;
  u->interrupt.data = u;
  u->interrupt.code = isr;
  lAddTail(exec, &d->unitlist, &u->node);
  lReleaseMutex(exec, &d->unitlock);
  inithw(u);
  lAddDevInt(d->exp, &adev->base, &u->interrupt, 0);
}

static struct Segment *iFini(struct Device *dev) {
  return NULL;
}

static const struct DeviceOp optemplate = {
  .AbortIO  = iAbortIO,
  .BeginIO  = iBeginIO,
  .Expunge  = iFini,
  .Close    = iClose,
  .Open     = iOpen,
};

static const struct Resident RES0 = {
  .flags                = RTF_AUTOINIT | 0,
  .info.name            = "serial.device",
  .info.idstring        = "serial.device 0.1 2022-09-29",
  .info.type            = NT_DEVICE,
  .info.version         = 1,
  .pri                  = 0,
  .init.iauto.f         = serinit,
  .init.iauto.optable   = &optemplate,
  .init.iauto.opsize    = sizeof optemplate,
  .init.iauto.possize   = sizeof (struct DevBase),
};

