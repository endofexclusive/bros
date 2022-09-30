/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

/* Driver for ARM PrimeCell (PL011) UART */

#include <stdint.h>
#include <devices/serial.h>
#include <exec/exec.h>
#include <exec/libcall.h>

#if 0
#define dbg(...) kprintf(exec, __VA_ARGS__)
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

#define NELEM(a) ((sizeof a) / (sizeof (a[0])))

/* FIXME: let expansion add devices to driver */
#define DEV0_REGADDR    0x3F201000UL
#define DEV0_INTERRUPT  2

struct pl011_regs {
  uint32_t dr;
  uint32_t rsrecr;
  uint32_t unused_08;
  uint32_t unused_0c;
  uint32_t unused_10;
  uint32_t unused_14;
  uint32_t fr;
  uint32_t unused_1c;
  uint32_t ilpr;
  uint32_t ibrd;
  uint32_t fbrd;
  uint32_t lcrh;
  uint32_t cr;
  uint32_t ifls;
  uint32_t imsc;
  uint32_t ris;
  uint32_t mis;
  uint32_t icr;
  uint32_t dmacr;
};

#define FR_TXFE         0x80
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

struct DevBase {
  struct Device    Device;
  struct ExecBase *exec;
  struct Segment  *segment;
  struct List      unitlist;
  struct Mutex     unitlock;
  int              nunits;
};

struct DevUnit {
  struct Node        node;
  volatile struct pl011_regs *regs;
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
};

extern const struct Resident RES0;

static struct Library *init(
  struct ExecBase *exec,
  struct Library *lib,
  struct Segment *segment
) {
  struct DevBase *dev = (struct DevBase *) lib;

  dev->exec = exec;
  dev->segment = segment;
  lNewList(exec, &dev->unitlist);
  lInitMutex(exec, &dev->unitlock);
  /* FIXME: attach units somehow */
  dev->nunits = 1;

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

/* NOTE/FIXME: assumes I/O mux and pull configured */
static void inithw(struct DevUnit *u) {
  volatile struct pl011_regs *const regs = u->regs;
  regs->cr = 0;
  /* FIXME: set UART clock to 4 MHz */
  regs->icr = INT_ALL;
  /* 4000000.0 Hz / (16 * (0xb/64.0 + 2)) = 115107.91366906474 BAUD */
  regs->ibrd = 0x2;
  regs->fbrd = 0xb;
  regs->lcrh = LCRH_WLEN_8 | LCRH_FEN; /* 8N1, FIFO enable */
  regs->ifls = 0;
  regs->imsc = INT_ERROR;
  regs->cr = CR_RXE | CR_TXE | CR_UARTEN;
  regs->imsc |= INT_RX;
}

static void finihw(struct DevUnit *u) {
  volatile struct pl011_regs *const regs = u->regs;

  regs->imsc = 0;
  while (!(regs->fr & FR_TXFE)) {
    for (volatile int i = 0; i < 1234; i++);
  }
  regs->cr = 0;
}

static void isr_tx(
  struct ExecBase *exec,
  struct DevUnit *u,
  volatile struct pl011_regs *const regs
) {
  lObtainIntLock(exec, &u->tx.lock);
  while (1) {
    struct IOExtSer *req = (struct IOExtSer *) lGetHead(exec, &u->tx.list);
    if (req == NULL) {
      regs->imsc &= ~INT_TX;
      break;
    }
    unsigned char *reqbuf = req->data;
    while (req->actual < req->length &&
     (regs->fr & FR_TXFF) == 0) {
      regs->dr = (uint32_t) reqbuf[req->actual++] & 0xff;
    }
    if (req->actual == req->length) {
      lRemove(exec, &req->ior.message.node);
      req->ior.message.node.type = NT_FREEMSG;
      lReleaseIntLock(exec, &u->tx.lock);
      lReplyMsg(exec, &req->ior.message);
      dbg("%s: replied\n", __func__);
      lObtainIntLock(exec, &u->tx.lock);
    }
    if (regs->fr & FR_TXFF) {
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
  volatile struct pl011_regs *const regs
) {
  unsigned char rxdata[32];
  int nrx = 0;

  while ((nrx < (int) NELEM(rxdata)) &&
   !(regs->fr & FR_RXFE)) {
    rxdata[nrx] = regs->dr;
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
  volatile struct pl011_regs *const regs = u->regs;
  isr_rx(exec, u, regs);
  isr_tx(exec, u, regs);
}

static int cmd_write(struct DevBase *d, struct DevUnit *u,
 struct IOExtSer *req) {
  req->actual = 0;
  if (req->length == 0) {
    return 0;
  }

  /* at least one character must be in FIFO to get things going */
  struct ExecBase *exec = d->exec;
  volatile struct pl011_regs *const regs = u->regs;
  lObtainIntLock(exec, &u->tx.lock);
  if (!lGetHead(exec, &u->tx.list)) {
    unsigned char *reqbuf = req->data;
    while (req->actual < req->length &&
     (regs->fr & FR_TXFF) == 0) {
      regs->dr = (uint32_t) reqbuf[req->actual++] & 0xff;
    }
    if (req->actual == req->length) {
      lReleaseIntLock(exec, &u->tx.lock);
      return 0;
    }
  }
  req->ior.flags &= ~IOF_QUICK;
  lAddTail(exec, &u->tx.list, &req->ior.message.node);
  regs->imsc |= INT_TX;
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
  lRemIntServer(exec, &u->interrupt, DEV0_INTERRUPT);
  finihw(u);
  lRemove(exec, &u->node);
  ior->unit = NULL;
  lFreeMem(exec, u, sizeof *u);
  lReleaseMutex(exec, &d->unitlock);
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
  struct DevUnit *u;

  dbg("%s: entry\n", __func__);
  if (d->nunits <= unitnum) {
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

  lNewList(exec, &u->rx.list);
  lInitIntLock(exec, &u->rx.lock);
  lNewList(exec, &u->tx.list);
  lInitIntLock(exec, &u->tx.lock);

  u->regs = (void *) DEV0_REGADDR;
  u->num = unitnum;
  u->interrupt.node.name = RES0.info.name;
  u->interrupt.data = u;
  u->interrupt.code = isr;
  lAddTail(exec, &d->unitlist, &u->node);
  lReleaseMutex(exec, &d->unitlock);
  inithw(u);
  lAddIntServer(exec, &u->interrupt, DEV0_INTERRUPT);
}

static struct Segment *iFini(struct Device *dev) {
  struct DevBase *d = (struct DevBase *) dev;
  return d->segment;
}

static const struct DeviceOp optemplate = {
  .AbortIO  = iAbortIO,
  .BeginIO  = iBeginIO,
  .Expunge  = iFini,
  .Close    = iClose,
  .Open     = iOpen,
};

const struct Resident RES0 = {
  .matchword            = RTC_MATCHWORD,
  .matchtag             = &RES0,
  .endskip              = &((struct Resident *) (&RES0))[1],
  .flags                = RTF_AUTOINIT | 3,
  .info.name            = "serial.device",
  .info.idstring        = "serial.device 0.1 2022-05-18",
  .info.type            = NT_DEVICE,
  .info.version         = 1,
  .pri                  = 0,
  .init.iauto.f         = init,
  .init.iauto.optable   = &optemplate,
  .init.iauto.opsize    = sizeof optemplate,
  .init.iauto.possize   = sizeof (struct DevBase),
};

int _start(void);
int _start(void) {
  return -1;
}

