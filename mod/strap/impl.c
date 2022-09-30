/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

#include <devices/serial.h>
#include <exec/exec.h>
#include <exec/libcall.h>
#include <expansion/expansion.h>
#include <expansion/ambapp.h>
#include <expansion/mdio.h>
#include <expansion/libcall.h>

#define KASSERT(lib_, e) \
  if (!(e)) { \
    kprintf(lib_->exec, "assertion failed %s:%s:%d '" # e "'\n", \
     __FILE__, __func__, __LINE__); \
    lAlert(lib_->exec, AT_DeadEnd | AN_Strap | AG_Assert); \
  }

#if 1
 #define dbg(...) kprintf(lib->exec, __VA_ARGS__)
#else
 #define dbg(...)
#endif
static void kputchar(void *arg, int c) {
  lRawPutChar((struct ExecBase *) arg, c);
}
static void kprintf(struct ExecBase *exec, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  lRawDoFmt(exec, kputchar, exec, fmt, ap);
  va_end(ap);
}

static void tuart(struct ExecBase *exec) {
  int ret;
  char bufz[6] = { 0 };
  struct MsgPort *port;
  struct IOExtSer *ior;

  port = lCreateMsgPort(exec);
  ior = (struct IOExtSer *) lCreateIORequest(exec, port, sizeof *ior);
  if (ior == NULL) {
    return;
  }
  ret = lOpenDevice(exec, "serial.device", 0, &ior->ior);
  if (ret != IOERR_OK) {
    kprintf(exec, "ERROR: open serial.device unit 0\n");
    return;
  }

  ior->ior.command = SDCMD_GETPARAMS;
  lDoIO(exec, &ior->ior);
  if (ior->ior.error == IOERR_OK) {
    kprintf(exec, "%s: BAUD %ld\n", __func__, (long) ior->baud);
  }

  ior->ior.command = CMD_WRITE;
  ior->actual = 0;
  ior->length = 14;
  ior->data = "hello, world\r\n";
  lDoIO(exec, &ior->ior);

  ior->ior.command = CMD_WRITE;
  ior->actual = 0;
  ior->length = 2;
  ior->data = "> ";
  lDoIO(exec, &ior->ior);

  /* Echo some characters... */
  while (1) {
    ior->ior.command = CMD_READ;
    ior->actual = 0;
    ior->length = 1;
    ior->data = bufz;
    lDoIO(exec, &ior->ior);

    ior->ior.command = CMD_WRITE;
    ior->actual = 0;
    ior->length = 1;
    ior->data = bufz;
    lDoIO(exec, &ior->ior);
    if (bufz[0] == '\r') {
      break;
    }
  }
  kprintf(exec, "\n%s: OK!\n", __func__);

  lCloseDevice(exec, &ior->ior);
  lRawIOInit(exec);
  lDeleteIORequest(exec, &ior->ior);
  lDeleteMsgPort(exec, port);
}

#define PRETTY_SIZE(t) kprintf(exec, fmt, #t , (int) sizeof (struct t));
static void typesize(struct ExecBase *exec) {
  const char *fmt = " %-12s %3d\n";
  const char *sep = "------------------\n";
  kprintf(exec, "\nSize of some system types follow:\n");
  kprintf(exec, " Type        Size\n");
  kprintf(exec, "%s", sep);
  PRETTY_SIZE(IntLock);
  PRETTY_SIZE(Node);
  PRETTY_SIZE(Mutex);
  PRETTY_SIZE(Task);
  PRETTY_SIZE(MsgPort);
  PRETTY_SIZE(Library);
  PRETTY_SIZE(IORequest);
  PRETTY_SIZE(Resident);
  kprintf(exec, "%s", sep);
}

static int foreach_node(
  const struct List *const list,
  int (*f)(struct Node *node, void *arg),
  void *arg
) {
  int ret;
  struct Node *node;

  node = list->head;
  while (node->succ) {
    ret = f(node, arg);
    if (ret) {
      return ret;
    }
    node = node->succ;
  }
  return 0;
}

static int showdriver(struct Node *node, void *arg) {
  struct ExpansionBase *const exp = arg;
  const struct ExpansionDriver *const driver =
   (struct ExpansionDriver *) node;
  kprintf(exp->exec, " %-16s  %8d\n", driver->node.name, driver->bustype);
  return 0;
}

static void lShowDrivers(struct ExpansionBase *exp) {
  const char *sep = "----------------------------";
  kprintf(exp->exec, "\nRegistered Device Drivers\n");
  kprintf(exp->exec,   "=========================\n\n");
  kprintf(exp->exec, " %-16s  %8s\n", "Driver", "Bus type");
  kprintf(exp->exec, "%s\n", sep);
  lObtainMutex(exp->exec, &exp->lock);
  foreach_node(&exp->drivers, showdriver, exp);
  lReleaseMutex(exp->exec, &exp->lock);
  kprintf(exp->exec, "%s\n", sep);
}

struct blabla {
  struct ExpansionBase *exp;
  const char *sp;
};

static int showdev(struct Node *node, void *arg) {
  const struct blabla *const argi = arg;
  struct ExpansionBase *const exp = argi->exp;
  const char *sp = argi->sp;
  struct ExpansionDev *const dev = (struct ExpansionDev *) node;
  struct blabla argo;
  const char *drivername = "?";
  const char *devname = "?";
  unsigned vendor = 0, device = 0;
  long long hz;
  int ret;

  KASSERT(exp, dev);
  if (dev->driver && dev->driver->node.name) {
    drivername = dev->driver->node.name;
  }
  if (dev->node.name) {
    devname = dev->node.name;
  }
  if (dev->parent && dev->parent->bustype == EXPANSION_BUSTYPE_AMBAPP) {
    const struct AmbappDev *const adev = (struct AmbappDev *) dev;
    vendor = adev->vendor;
    device = adev->device;
  } else if (dev->parent && dev->parent->bustype == EXPANSION_BUSTYPE_MDIO) {
    const struct MDIODev *const mdev = (struct MDIODev *) dev;
    vendor = mdev->oui >> 12 & 0xff;
    device = mdev->oui & 0x0fff;
  }
  hz = 0;
  ret = lGetDevFreq(exp, dev, &hz);
  if (ret) {
    hz = 0;
  }
  kprintf(
    exp->exec,
    "%sdev:%-12s %02x:%03x driver:%p (%s) level:%d, freq:%ld",
    sp, devname,
    vendor, device,
    (void *) dev->driver, drivername, dev->initlevel, (long) hz
  );
  if (dev->bus == NULL) {
    kprintf(exp->exec, "\n");
    return 0;
  }
  kprintf(exp->exec, " spawn bustype:%d level:%d\n", dev->bus->bustype, dev->bus->initlevel);
  sp--;
  if (*sp == 'x') {
    /* too deep */
    return 1;
  }
  argo = *argi;
  argo.sp = sp;
  return foreach_node(&dev->bus->children, showdev, &argo);
}

static void lShowTopology(struct ExpansionBase *exp) {
  const char sp[] = "x          ";
  struct blabla thearg = {
    .exp = exp,
    .sp = &sp[sizeof(sp)-1],
  };
  kprintf(exp->exec, "\nBus topology\n");
  kprintf(exp->exec,   "============\n\n");
  lObtainMutex(exp->exec, &exp->lock);
  showdev(&exp->root.node, &thearg);
  lReleaseMutex(exp->exec, &exp->lock);
  kprintf(exp->exec, "\n");
}

static void doexpansion(struct ExecBase *exec) {
  struct ExpansionBase *exp;

  exp = (struct ExpansionBase *) lOpenLibrary(exec, "expansion.library", 0);
  if (exp == NULL) {
    return;
  }

  if (lGetHead(exec, &exp->drivers) == NULL) {
    return;
  }
  lShowDrivers(exp);
  kprintf(exec, "\n");
  lUpdateExpansion(exp);
  lShowTopology(exp);

  lCloseLibrary(exec, &exp->lib);

  return;
}

static int init(struct ExecBase *exec, void *data, struct Segment *seg) {
  typesize(exec);
  doexpansion(exec);
  tuart(exec);

  kprintf(exec, "%s\n", "strap: RemTask()");
  lRemTask(exec);
  return 0;
}

const struct Resident RES0 = {
  .matchword           = RTC_MATCHWORD,
  .matchtag            = &RES0,
  .endskip             = &((struct Resident *) (&RES0))[1],
  .flags               = 3,
  .info.name           = "strap",
  .info.idstring       = "strap 0.1 2021-08-12",
  .info.type           = NT_UNKNOWN,
  .info.version        = 1,
  .pri                 = -127,
  .init.direct.f       = init,
};

int _start(void);
int _start(void) {
  return -1;
}

