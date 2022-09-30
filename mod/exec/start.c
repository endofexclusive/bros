/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

#include <priv.h>
#include <port.h>

#if 1
#define info(...) kprintf(lib, __VA_ARGS__)
#else
#define info(...)
#endif

static const char idstring[] = "exec 0.3 2022-09-16";
static const char name[]     = "exec.library";
#define LIB_VERSION 3

const Resident RES0 = {
  .matchword            = RTC_MATCHWORD,
  .matchtag             = (Resident *) &RES0,
  .endskip              = &((Resident *) (&RES0))[1],
  .info.name            = (char *) name,
  .info.idstring        = (char *) idstring,
  .info.type            = NT_LIBRARY,
  .info.version         = LIB_VERSION,
  .pri                  = 0,
  .flags                = 0,
};

void initlib(Lib *lib) {
  lib->lib.node.name = (char *) name;
  lib->lib.node.type = NT_LIBRARY;
  lib->lib.version = LIB_VERSION;
  lib->lib.opencount = 1;
  iMakeFunctions(NULL, &lib->lib, &optemplate, sizeof optemplate);
  iNewList(lib, &lib->memlist);
  iNewList(lib, &lib->liblist);
  iNewList(lib, &lib->devlist);
  iNewList(lib, &lib->reslist);
  iNewList(lib, &lib->cpuonline);
  iNewList(lib, &lib->cpuoffline);
  iNewList(lib, &lib->taskready);
  iNewList(lib, &lib->taskremoved);
  iNewList(lib, &lib->taskwait);
  iInitMutex(lib, &lib->memlock);
  iInitMutex(lib, &lib->liblock);
  iInitMutex(lib, &lib->devlock);
  iInitIntLock(lib, &lib->tasklock);
  iAddLibrary(lib, &lib->lib);
}

/* LibraryOp entries for exec.library */
Library *iOpenBase(Library *lib) {
  return lib;
}

void iCloseBase(Library *lib) {
}

Segment *iExpungeBase(Library *lib) {
  return NULL;
}

void kprintf(Lib *lib, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  iRawDoFmt(lib, (void (*)(void *, int)) iRawPutChar, lib, fmt, ap);
  va_end(ap);
}

void iAlert(Lib *lib, unsigned long why) {
  if (why & AT_DeadEnd) {
    port_disable_interrupts();
    kprintf(lib, "\nAlert: %08x\n", why);
    port_halt();
  } else {
    kprintf(lib, "\nAlert: %08x\n", why);
  }
}

void addmem0(Lib *lib, void *bottom, void *top) {
  size_t sz;
  MemHeader *mh;

  KASSERT(bottom);
  KASSERT(top);
  sz = (char *) top - (char *) bottom;
  info("[%p..%p]  %lu KiB\n", bottom, top, (unsigned long) sz / 1024);
  mh = lInitMemHeader(lib, bottom, sz);
  if (mh == NULL) {
    lAlert(lib, AT_DeadEnd | AN_ExecLib | AG_NoMemory | AO_ExecLib);
  }
  lAddMemHeader(lib, mh, "RAM", MEMF_DMA | MEMF_EXEC, 0);
}

