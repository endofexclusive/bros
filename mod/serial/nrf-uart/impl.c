/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022 Martin Åberg */

/* Driver for nRF51 Series UART */

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

/* Peripheral ID 2 */
#define DEV0_REGADDR    0x40002000UL
#define DEV0_INTERRUPT  2

#define TASK_STARTRX    (0x000/4)
#define TASK_STOPRX     (0x004/4)
#define TASK_STARTTX    (0x008/4)
#define TASK_STOPTX     (0x00C/4)
#define EVENT_RXDRDY    (0x108/4)
#define EVENT_TXDRDY    (0x11C/4)
#define EVENT_ERROR     (0x124/4)
#define EVENT_RXTO      (0x144/4)
#define REG_INTEN       (0x300/4)
#define REG_INTENSET    (0x304/4)
#define REG_INTENCLR    (0x308/4)
#define REG_ENABLE      (0x500/4)
#define REG_PSEL_TXD    (0x50C/4) /* Pin select for TXD */
#define REG_PSEL_RXD    (0x514/4) /* Pin select for RXD */
#define REG_RXD         (0x518/4)
#define REG_TXD         (0x51C/4)
#define REG_BAUDRATE    (0x524/4)
#define REG_CONFIG      (0x56C/4)

#define INT_RXTO    (1<<17)
#define INT_ERROR   (1<< 9)
#define INT_TXDRDY  (1<< 7)
#define INT_RXDRDY  (1<< 2)
#define UART_ENABLE_Enabled     4
#define UART_BAUDRATE_9600      0x00275000
#define UART_BAUDRATE_115200    0x01D7E000

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
  volatile unsigned *regs;
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

static void inithw(struct DevUnit *u) {
  volatile unsigned *const regs = u->regs;
  /* 115200/8-N-1, no flow control */
  regs[REG_ENABLE] = 0;
  regs[REG_BAUDRATE] = UART_BAUDRATE_115200;
  regs[REG_CONFIG] = 0;
  /* TODO: Set PSEL{RXD,RTS,CTS,TXD when UART is disabled. */
  regs[REG_ENABLE] = UART_ENABLE_Enabled;
  regs[EVENT_TXDRDY] = 1;
  regs[EVENT_RXDRDY] = 0;
  regs[EVENT_ERROR] = 0;
  regs[EVENT_RXTO] = 0;
  regs[TASK_STARTRX] = 1;
  regs[TASK_STARTTX] = 1;
  regs[REG_INTEN] = INT_RXTO | INT_ERROR | INT_RXDRDY;
}

static void finihw(struct DevUnit *u) {
  volatile unsigned *const regs = u->regs;

  regs[REG_INTEN] = 0;
  regs[TASK_STOPRX] = 1;
  while (regs[EVENT_TXDRDY] == 0) {
    for (volatile int i = 0; i < 1234; i++);
  }
  regs[TASK_STOPTX] = 1;
  regs[REG_ENABLE] = 0;
}

static void isr_tx(
  struct ExecBase *exec,
  struct DevUnit *u,
  volatile unsigned *const regs
) {
  lObtainIntLock(exec, &u->tx.lock);
  while (1) {
    struct IOExtSer *req = (struct IOExtSer *) lGetHead(exec, &u->tx.list);
    if (req == NULL) {
      regs[REG_INTENCLR] = INT_TXDRDY;
      break;
    }
    if (regs[EVENT_TXDRDY] == 0) {
      break;
    }
    unsigned char *reqbuf = req->data;
    regs[EVENT_TXDRDY] = 0;
    regs[REG_TXD] = (unsigned) reqbuf[req->actual++];
    if (req->actual == req->length) {
      lRemove(exec, &req->ior.message.node);
      req->ior.message.node.type = NT_FREEMSG;
      lReleaseIntLock(exec, &u->tx.lock);
      lReplyMsg(exec, &req->ior.message);
      dbg("%s: replied\n", __func__);
      lObtainIntLock(exec, &u->tx.lock);
    }
  }
  lReleaseIntLock(exec, &u->tx.lock);
}

/* INVARIANT: rx.buf  has data   => rx.list is empty */
/* INVARIANT: rx.list has req(s) => rx.buf  is empty */
static void isr_rx(
  struct ExecBase *exec,
  struct DevUnit *u,
  volatile unsigned *const regs
) {
  unsigned char rxdata[8];
  int nrx = 0;

  while ((nrx < (int) NELEM(rxdata)) &&
   regs[EVENT_RXDRDY]) {
    regs[EVENT_RXDRDY] = 0;
    rxdata[nrx] = regs[REG_RXD] & 0xff;
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
  volatile unsigned *const regs = u->regs;
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
  volatile unsigned *const regs = u->regs;
  lObtainIntLock(exec, &u->tx.lock);
  if (!lGetHead(exec, &u->tx.list)) {
    unsigned char *reqbuf = req->data;
    if (regs[EVENT_TXDRDY]) {
      regs[EVENT_TXDRDY] = 0;
      regs[REG_TXD] = (unsigned) reqbuf[req->actual++];
    }
    if (req->actual == req->length) {
      lReleaseIntLock(exec, &u->tx.lock);
      return 0;
    }
  }
  req->ior.flags &= ~IOF_QUICK;
  lAddTail(exec, &u->tx.list, &req->ior.message.node);
  regs[REG_INTENSET] = INT_TXDRDY;
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
  lRemIntServer(exec, &u->interrupt, DEV0_INTERRUPT + 1);
  finihw(u);
  /* NOTE: RawIOInit() may be needed before RawPutChar(). */
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
  lAddIntServer(exec, &u->interrupt, DEV0_INTERRUPT + 1);
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

