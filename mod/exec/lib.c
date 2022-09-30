/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

#include <priv.h>

static const LibraryOp *GETOP(const Library *l) {
  LibraryOp * const op = (LibraryOp *) l;
  return op-1;
}

Library *iMakeLibrary(
  Lib *lib,
  const ResidentInfo *rinfo,
  const ResidentAuto *rauto,
  Segment *segment
)
{
  void *base;
  Library *l;
  int aligned_negsize;
  int allocated_size;

  /* Make sure Library base aligned on MemAlign */
  aligned_negsize = rauto->opsize + ALIGNOF(MemAlign) - 1;
  aligned_negsize /= ALIGNOF(MemAlign);
  aligned_negsize *= ALIGNOF(MemAlign);

  allocated_size = aligned_negsize + rauto->possize;
  base = lAllocMem(lib, allocated_size, MEMF_CLEAR | MEMF_ANY);
  if (base == NULL) {
    return NULL;
  }

  l = (Library *) ((char *) base + aligned_negsize);
  KASSERT(((char *) l) == ((char *) base + aligned_negsize));
  l->allocatedmemory = base;
  l->allocatedsize = allocated_size;

  iMakeFunctions(lib, l, rauto->optable, rauto->opsize);

  if (rinfo) {
    l->node.name    = rinfo->name;
    l->idstring     = rinfo->idstring;
    l->node.type    = rinfo->type;
    l->version      = rinfo->version;
  }

  l = rauto->f(lib, l, segment);
  if (l == NULL) {
    lFreeMem(lib, base, allocated_size);
  }

  return l;
}

int iSetFunction(
  Lib *lib,
  Library *library,
  int negoffset,
  void (*newfunc)(void),
  void (**oldfunc)(void)
) {
  struct {
    void (*func)(void);
  } *p;

  p = (void *) library;
  p += negoffset;
  /*
   * There are at least two issues with the below assignments
   * 1) Race if someone else is patching it at the same time.
   *    This can be mitigated by some higher level protocol
   *    among patchers.
   * 2) Pointer write may be divisible on some machines (AVR).
   *    It means that someone calling, for example
   *    lObtainIntLock(), can read a half-written broken
   *    pointer. On single-processor systems it should be enough
   *    to arbitrate by disabling interrupts. On SMP,
   *    compare-and-swap may be used.
   */
  if (oldfunc) {
    *oldfunc = p->func;
  }
  p->func = newfunc;
  return 0;
}

void *iMakeFunctions(
  Lib *lib,
  Library *library,
  const void *optable,
  int opsize
) {
  char *dst;

  dst = (char *) library - opsize;
  return iMemMove(lib, dst, optable, opsize);
}

Segment *expungelib(Lib *lib, Library *l) {
  Segment *seg;
  void *mem;
  int size;

  mem = l->allocatedmemory;
  size = l->allocatedsize;
  seg = GETOP(l)->Expunge(l);
  l = NULL; /* Library may manage its own memory */
  if (seg) {
    lFreeMem(lib, mem, size);
  }
  return seg;
}

void remlib(Lib *lib, Library *l, Mutex *lock) {
  int callexpunge;

  callexpunge = 0;

  lObtainMutex(lib, lock);
  if (l->opencount) {
    l->flags |= LIBF_DELEXP;
  } else {
    iRemove(lib, &l->node);
    callexpunge = 1;
  }
  lReleaseMutex(lib, lock);

  if (callexpunge) {
    expungelib(lib, l);
  }
}

void enqnode(Lib *lib, Node *node, List *list, Mutex *lock) {
  lObtainMutex(lib, lock);
  iEnqueue(lib, list, node);
  lReleaseMutex(lib, lock);
}

void iAddLibrary(Lib *lib, Library *l) {
  enqnode(lib, &l->node, &lib->liblist, &lib->liblock);
}

void iRemLibrary(Lib *lib, Library *l) {
  remlib(lib, l, &lib->liblock);
}

/*
 * The library Open() entry may search liblist, open other
 * libraries with OpenLibrary(), load from disk, etc.
 */
Library *iOpenLibrary(Lib *lib, const char *name, int version) {
  Library *l;
  Library *openedl;
  List *list;
  int callexpunge;

  list = &lib->liblist;
  lObtainMutex(lib, &lib->liblock);
  do {
    l = (Library *) lFindName(lib, list, name);
    list = (List *) l;
  } while (l && l->version < version);
  if (l) {
    /* Prevent library from going away while calling Open(). */
    l->opencount++;
  }
  lReleaseMutex(lib, &lib->liblock);

  if (l == NULL) {
    return NULL;
  }

  /* Do the Open() without lock. */
  openedl = GETOP(l)->Open(l);

  callexpunge = 0;
  lObtainMutex(lib, &lib->liblock);
  /*
   * NOTE: Since we last changed the invariants under liblock
   * before the Open(l) call, many things could have happened,
   * including OpenLibrary(name), CloseLibrary(name) and
   * RemLibrary. In particualr, a delayed expunge could have
   * been requested, so we must check the expunge condition.
   */
  if (openedl) {
    /* Open OK so reset delayed expunge hint */
    l->flags &= ~LIBF_DELEXP;
  } else {
    /* Undo the open. Delayed expunge may still be there. */
    l->opencount--;
    if (l->opencount == 0 && l->flags & LIBF_DELEXP) {
      iRemove(lib, &l->node);
      callexpunge = 1;
    }
  }
  lReleaseMutex(lib, &lib->liblock);

  if (callexpunge) {
    expungelib(lib, l);
  }

  return openedl;
}

void iCloseLibrary(Lib *lib, Library *l) {
  int callexpunge;

  GETOP(l)->Close(l);

  callexpunge = 0;
  lObtainMutex(lib, &lib->liblock);
  l->opencount--;
  if (l->opencount == 0 && l->flags & LIBF_DELEXP) {
    iRemove(lib, &l->node);
    callexpunge = 1;
  }
  lReleaseMutex(lib, &lib->liblock);

  if (callexpunge) {
    expungelib(lib, l);
  }
}

