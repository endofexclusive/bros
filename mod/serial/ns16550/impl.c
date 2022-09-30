/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022 Martin Åberg */

/* Driver for NS16550 */

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

#define KASSERT(e)
#define NELEM(a) ((sizeof a) / (sizeof (a[0])))

/* riscv-virt */
#define DEV0_REGADDR    0x10000000UL
#define DEV0_INTERRUPT  10

/* DLAB is Divisor Latch Access Bit (DLAB) */
/* DLAB = 0 */
#define REG_R 8
#define REG_W 16
#define REG_RW (REG_R | REG_W)
#define REG_RBR (0+REG_R)  /* (r ) Receiver buffer */
#define REG_THR (0+REG_W)  /* ( w) Transmitter holding */
#define REG_IER (1+REG_RW) /* (rw) Interrupt enable */
#define REG_IIR (2+REG_R)  /* (r ) Interrupt identification */
#define REG_FCR (2+REG_W)  /* ( w) FIFO control */
#define REG_LCR (3+REG_RW) /* (rw) Line control */
#define REG_MCR (4+REG_RW) /* (rw) Modem control */
#define REG_LSR (5+REG_R)  /* (r ) Line status */
#define REG_MSR (6+REG_R)  /* (r ) Modem status */
#define REG_RCR (7+REG_RW) /* (rw) Scratch */
/* DLAB = 1 */
#define REG_DLL (0+REG_RW) /* (rw) Divisor latch LSB */
#define REG_DLM (1+REG_RW) /* (rw) Divisor latch MSB */

#define IER_TBE   0x02 /* Transmitter holding register empty */
#define IER_RDA   0x01 /* Received data available */
#define IIR_NIP   0x01 /* No interrupt pending */
#define IIR_WHY   0x0e /* Interrupt ID */
#define IIR_CT    0x0c /* Character timeout */
#define IIR_LSC   0x06 /* Line status change */
#define IIR_RDA   0x04 /* Received data available */
#define IIR_THRE  0x02 /* Transmitter holding register empty */
#define IIR_MSC   0x00 /* Modem status change */
#define FCR_XFR   0x04 /* Clear transmit FIFO */
#define FCR_RFR   0x02 /* Clear receive FIFO */
#define FCR_FIFO  0x01 /* Enable FIFO */
#define LCR_WORD8 0x03 /* 8 bit word */
#define MCR_RTS   0x02 /* RTS output */
#define MCR_DTR   0x01 /* DTR output */
#define LSR_TEMT  0x40 /* THR is empty and line is idle */
#define LSR_THRE  0x20 /* THR is empty */
#define LSR_RDA   0x01 /* receiver data available */

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
  volatile unsigned char *regs;
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

static void write_reg(struct DevUnit *u, int reg, int val) {
  KASSERT(reg & REG_W);
  u->regs[reg & 0x7] = val;
}

static int read_reg(struct DevUnit *u, int reg) {
  KASSERT(reg & REG_R);
  return u->regs[reg & 0x7];
}

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
  write_reg(u, REG_LCR, LCR_WORD8); /* 8N1 */
  write_reg(u, REG_MCR, MCR_RTS | MCR_DTR);
  write_reg(u, REG_IER, 0);
  /* clear interrupt status */
  write_reg(u, REG_FCR, FCR_FIFO | FCR_RFR | FCR_XFR);
  write_reg(u, REG_IER, IER_RDA);
}

static void finihw(struct DevUnit *u) {
  /* Wait for transmitter shift register empty condition. */
  while ((read_reg(u, REG_LSR) & LSR_TEMT) == 0) {
    for (volatile int i = 0; i < 1234; i++);
  }
  write_reg(u, REG_MCR, 0);
  write_reg(u, REG_IER, 0);
}

static void isr_tx(
  struct ExecBase *exec,
  struct DevUnit *u,
  void *regs
) {
  lObtainIntLock(exec, &u->tx.lock);
  while (1) {
    struct IOExtSer *req = (struct IOExtSer *) lGetHead(exec, &u->tx.list);
    if (req == NULL) {
      write_reg(u, REG_IER, IER_RDA);
      break;
    }
    unsigned char *reqbuf = req->data;
    /* Write up to 16 words if empty. */
    if (read_reg(u, REG_LSR) & LSR_THRE) {
      int n = 16;
      while (req->actual < req->length && n) {
        write_reg(u, REG_THR, (unsigned int) reqbuf[req->actual++]);
        n--;
      }
    }
    if (req->actual == req->length) {
      lRemove(exec, &req->ior.message.node);
      req->ior.message.node.type = NT_FREEMSG;
      lReleaseIntLock(exec, &u->tx.lock);
      lReplyMsg(exec, &req->ior.message);
      dbg("%s: replied\n", __func__);
      lObtainIntLock(exec, &u->tx.lock);
    }
    if ((read_reg(u, REG_LSR) & LSR_THRE) == 0) {
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
  void *regs
) {
  unsigned char rxdata[16];
  int nrx = 0;

  while ((nrx < (int) NELEM(rxdata)) &&
   read_reg(u, REG_LSR) & LSR_RDA) {
    rxdata[nrx] = read_reg(u, REG_RBR);
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
  while (1) {
    int iir = read_reg(u, REG_IIR);
    if (iir & IIR_NIP) {
      break;
    }
    switch (iir & IIR_WHY) {
      case IIR_CT:
      case IIR_RDA:  isr_rx(exec, u, NULL); break;
      case IIR_THRE: isr_tx(exec, u, NULL); break;
    }
  }
}

static int cmd_write(struct DevBase *d, struct DevUnit *u,
 struct IOExtSer *req) {
  req->actual = 0;
  if (req->length == 0) {
    return 0;
  }

  struct ExecBase *exec = d->exec;
  lObtainIntLock(exec, &u->tx.lock);
  if (!lGetHead(exec, &u->tx.list)) {
    unsigned char *reqbuf = req->data;
    /* Write up to 16 words if empty. */
    if (read_reg(u, REG_LSR) & LSR_THRE) {
      int n = 16;
      while (req->actual < req->length && n) {
        write_reg(u, REG_THR, (unsigned int) reqbuf[req->actual++]);
        n--;
      }
    }
    if (req->actual == req->length) {
      lReleaseIntLock(exec, &u->tx.lock);
      return 0;
    }
  }
  req->ior.flags &= ~IOF_QUICK;
  lAddTail(exec, &u->tx.list, &req->ior.message.node);
  write_reg(u, REG_IER, IER_TBE | IER_RDA);
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
  .info.idstring        = "serial.device 0.1 2022-09-29",
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

