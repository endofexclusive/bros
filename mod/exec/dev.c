/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

#include <priv.h>

static const DeviceOp *GETOP(const Device *d) {
  DeviceOp * const op = (DeviceOp *) d;
  return op-1;
}

void iAddDevice(Lib *lib, Device *d) {
  enqnode(lib, &d->lib.node, &lib->devlist, &lib->devlock);
}

void iRemDevice(Lib *lib, Device *d) {
  remlib(lib, &d->lib, &lib->devlock);
}

int iOpenDevice(
  Lib *lib,
  const char *name,
  int unitnum,
  IORequest *ior
) {
  Device *d;
  int callexpunge;
  lObtainMutex(lib, &lib->devlock);
  d = (Device *) lFindName(lib, &lib->devlist, name);
  if (d) {
    d->lib.opencount++;
  }
  lReleaseMutex(lib, &lib->devlock);

  ior->device = d;
  if (d == NULL) {
    ior->error = IOERR_NOTFOUND;
    return IOERR_NOTFOUND;
  }

  ior->error = IOERR_OK;
  GETOP(d)->Open(d, ior, unitnum);

  callexpunge = 0;
  lObtainMutex(lib, &lib->devlock);
  if (ior->error == IOERR_OK) {
    d->lib.flags &= ~LIBF_DELEXP;
  } else {
    d->lib.opencount--;
    if (d->lib.opencount == 0 && d->lib.flags & LIBF_DELEXP) {
      iRemove(lib, &d->lib.node);
      callexpunge = 1;
    }
  }
  lReleaseMutex(lib, &lib->devlock);

  if (callexpunge) {
    expungelib(lib, &d->lib);
  }

  return ior->error;
}

void iCloseDevice(Lib *lib, IORequest *ior) {
  int callexpunge;
  Device *d;

  d = ior->device;
  GETOP(d)->Close(d, ior);

  callexpunge = 0;
  lObtainMutex(lib, &lib->devlock);
  d->lib.opencount--;
  if (d->lib.opencount == 0 && d->lib.flags & LIBF_DELEXP) {
    iRemove(lib, &d->lib.node);
    callexpunge = 1;
  }
  lReleaseMutex(lib, &lib->devlock);

  if (callexpunge) {
    expungelib(lib, &d->lib);
  }
}

int iDoIO(Lib *lib, IORequest *ior) {
  Device *d = ior->device;
  ior->flags |= IOF_QUICK;
  GETOP(d)->BeginIO(d, ior);
  return lWaitIO(lib, ior);
}

void iSendIO(Lib *lib, IORequest *ior) {
  Device *d = ior->device;
  ior->flags &= ~IOF_QUICK;
  GETOP(d)->BeginIO(d, ior);
}

int iCheckIO(Lib *lib, IORequest *ior) {
  if (ior->flags & IOF_QUICK) {
    return 1;
  }
  return ior->message.node.type == NT_REPLYMSG;
}

int iWaitIO(Lib *lib, IORequest *ior) {
  if ((ior->flags & IOF_QUICK) == 0) {
    lWaitMsg(lib, &ior->message);
  }
  return ior->error;
}

void iAbortIO(Lib *lib, IORequest *ior) {
  Device *d = ior->device;
  GETOP(d)->AbortIO(d, ior);
}

/* negative number indicating offset from library base to op */
#define LIBRARY_NEGOFFSET(optype, opfield) \
 (offsetof(optype, opfield) - (sizeof (optype)))

SASSERT(LIBRARY_NEGOFFSET(LibraryOp, Open)
 == LIBRARY_NEGOFFSET(DeviceOp, Open));
SASSERT(LIBRARY_NEGOFFSET(LibraryOp, Close)
 == LIBRARY_NEGOFFSET(DeviceOp, Close));
SASSERT(LIBRARY_NEGOFFSET(LibraryOp, Expunge)
 == LIBRARY_NEGOFFSET(DeviceOp, Expunge));

