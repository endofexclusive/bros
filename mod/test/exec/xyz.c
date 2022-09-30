/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

#include "test.h"
#define KASSERT(cond) if (!(cond)) lAlert(exec, AT_DeadEnd | __LINE__)

/*
 * "my.library"
 * Expunge stuff only useful for dynamic loaded libraries.
 */
struct MyLibrary {
  struct Library Lib;
  struct ExecBase *exec;
};
static struct Library *lib0open(struct Library *lib) {
  return lib;
}
static void lib0close(struct Library *lib) {
}
static struct Segment *lib0expunge(struct Library *lib) {
  return NULL;
}
static const struct LibraryOp theop = {
  .Expunge  = lib0expunge,
  .Close    = lib0close,
  .Open     = lib0open,
};
static struct Library *lib0init(
  struct ExecBase *exec,
  struct Library *l,
  struct Segment *segment
) {
  KASSERT(exec); KASSERT(l);
  struct MyLibrary *my = (struct MyLibrary *) l;
  KASSERT(l->node.type == NT_LIBRARY);
  KASSERT(l->version == 124);
  KASSERT(l->opencount == 0);
  my->exec = exec;
  return l;
}
/*
 * This resident structure one will not be found at resident
 * scanning since it has invalid matchword and matchtag.
 */
static const struct Resident lib0res = {
  .info.name              = "my.library",
  .info.idstring          = "my 1.0 2021-05-22",
  .info.type              = NT_LIBRARY,
  .info.version           = 124,
  .flags                  = RTF_AUTOINIT | 0,
  .init.iauto.f           = lib0init,
  .init.iauto.optable     = &theop,
  .init.iauto.opsize      = sizeof theop,
  .init.iauto.possize     = sizeof (struct MyLibrary),
};

void test_xyz(struct ExecBase *exec) {
  {
    struct Node *node;
    lObtainMutex(exec, &exec->devlock);
    node = lFindName(exec, &exec->devlist, "hej");
    lReleaseMutex(exec, &exec->devlock);
    KASSERT(node == NULL);
  }
  {
    void *p;
    size_t sz = 1024+1;
    size_t avail0;
    size_t avail1;

    avail0 = lAvailMem(exec, MEMF_ANY);
    p = lAllocMem(exec, sz, MEMF_ANY);
    KASSERT(p);

    avail1 = lAvailMem(exec, MEMF_ANY);
    KASSERT(avail1 < avail0);

    lFreeMem(exec, p, sz);
    avail1 = lAvailMem(exec, MEMF_ANY);
    KASSERT(avail1 == avail0);
  }
  {
    struct Library *ebase;

    ebase = lOpenLibrary(exec, "exec.library", exec->lib.version + 1);
    KASSERT(ebase == NULL);

    ebase = lOpenLibrary(exec, "exec.library", exec->lib.version);
    KASSERT(ebase == (struct Library *) exec);

    lCloseLibrary(exec, ebase);

    ebase = lOpenLibrary(exec, "fisk.library", 1);
    KASSERT(ebase == NULL);
  }
  {
    int ret;
    struct IORequest *ior;
    struct MsgPort *port;

    port = lCreateMsgPort(exec);
    KASSERT(port);

    ior = lCreateIORequest(exec, port, sizeof *ior);
    KASSERT(ior);

    ret = lOpenDevice(exec, "my.device", 0, ior);
    KASSERT(ret == IOERR_NOTFOUND);

    lDeleteIORequest(exec, ior);
    lDeleteMsgPort(exec, port);
  }
  {
    struct Library *mybase;

    lInitResident(exec, &lib0res, NULL);
    mybase = lOpenLibrary(exec, "my.library", 0);
    KASSERT(mybase);
    KASSERT(mybase->opencount == 1);

    lCloseLibrary(exec, mybase);
    KASSERT(mybase->opencount == 0);

    mybase = lOpenLibrary(exec, "my.library", 0);
    KASSERT(mybase);
    KASSERT(mybase->opencount == 1);

    lCloseLibrary(exec, mybase);
    KASSERT(mybase->opencount == 0);

    lRemLibrary(exec, mybase);
    mybase = lOpenLibrary(exec, "my.library", 0);
    KASSERT(mybase == NULL);
  }
}

