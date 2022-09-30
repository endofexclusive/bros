/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022 Martin Åberg */

/* Interrupt server selection, SMP version */

#include <priv.h>
#include <port.h>

/*
 * Requirement is that
 * - Any task shall be able to call {Add,Rem}IntServer() at any
 *   time
 * - Interrupts must be able to nest
 * - All servers shall be called with the list lock held
 * - Same or different interrupt on different CPU simultaneously
 *   - could limit that requirement, for example all ISR on
 *     CPUn.
 * Consequence: ISR can not do {Add,Rem}Intserver()
 */

/*
 * rwlock based on IntLock with opportunity to optimize
 *
 * Note that the implementation is not general. It assumes that
 * the reader executes in a context which can never Wait(). It
 * is OK for the reader to be interrupted (nesting).
 */
typedef struct {
  IntLock r;
  IntLock g;
  int b;
} RWLock;

static void rwlock_init(Lib *lib, RWLock *rwlock) {
  lInitIntLock(lib, &rwlock->r);
  lInitIntLock(lib, &rwlock->g);
  rwlock->b = 0;
}

/*
 * In rlock/runlock, the Obtain(g) and Release(g) can be on
 * different CPUs. That means that the
 * port_{enable/disable}_interrupts() could get out of sync if
 * standard Obtain() and Release() was used. In addition, that
 * would mean running the intservers in port_disable state so no
 * nesting is possible.
 *
 * The solution is to use dedicated ObtainDisabled() and
 * ReleaseDisabled().
 */
static void rwlock_rlock(Lib *lib, RWLock *rwlock) {
  lObtainIntLock(lib, &rwlock->r);
  if (rwlock->b == 0) {
    /* The first reader entering will lock out writers */
    iObtainIntLockDisabled(lib, &rwlock->g);
  }
  rwlock->b++;
  lReleaseIntLock(lib, &rwlock->r);
}

static void rwlock_runlock(Lib *lib, RWLock *rwlock) {
  lObtainIntLock(lib, &rwlock->r);
  rwlock->b--;
  if (rwlock->b == 0) {
    /* The final reader leaving will re-allow writers */
    iReleaseIntLockDisabled(lib, &rwlock->g);
  }
  lReleaseIntLock(lib, &rwlock->r);
}

static void rwlock_wlock(Lib *lib, RWLock *rwlock) {
  lObtainIntLock(lib, &rwlock->g);
}

static void rwlock_wunlock(Lib *lib, RWLock *rwlock) {
  lReleaseIntLock(lib, &rwlock->g);
}

/* Contains all handlers for an interrupt source (intnum) */
typedef struct IntList {
  List list;
  RWLock rwlock;
} IntList;

static void intlist_init(Lib *lib, IntList *islist) {
  iNewList(lib, &islist->list);
  rwlock_init(lib, &islist->rwlock);
}
static void intlist_wlock(Lib *lib, IntList *islist) {
  rwlock_wlock(lib, &islist->rwlock);
}
static void intlist_wunlock(Lib *lib, IntList *islist) {
  rwlock_wunlock(lib, &islist->rwlock);
}
static void intlist_rlock(Lib *lib, IntList *islist) {
  rwlock_rlock(lib, &islist->rwlock);
}
static void intlist_runlock(Lib *lib, IntList *islist) {
  rwlock_runlock(lib, &islist->rwlock);
}

void iAddIntServer(Lib *lib, Interrupt *inode, int intnum) {
  IntList *islist;

  inode->node.type = NT_INTERRUPT;
  islist = &lib->intserver[intnum];
  intlist_wlock(lib, islist);
  iEnqueue(lib, &islist->list, &inode->node);
  port_enable_intnum(intnum);
  intlist_wunlock(lib, islist);
}

void iRemIntServer(Lib *lib, Interrupt *inode, int intnum) {
  IntList *islist;

  islist = &lib->intserver[intnum];
  intlist_wlock(lib, islist);
  iRemove(lib, &inode->node);
  if (iGetHead(lib, &islist->list) == NULL) {
    port_disable_intnum(intnum);
  }
  intlist_wunlock(lib, islist);
}

/* called from interrupt exception handler */
void runintservers(Lib *lib, int intnum) {
  IntList *islist;
  Node *node;

  islist = &lib->intserver[intnum];
  intlist_rlock(lib, islist);
  for (node = islist->list.head; node->succ; node = node->succ) {
    Interrupt *is;

    is = (Interrupt *) node;
    is->code(lib, is->data);
  }
  intlist_runlock(lib, islist);
}

/* called once at init */
void intserver_init(Lib *lib, int num) {
  IntList *islist;

  islist = iAllocMem(lib, num * sizeof *islist, MEMF_ANY);
  KASSERT(islist);
  for (int i = 0; i < num; i++) {
    intlist_init(lib, &islist[i]);
  }
  lib->intserver = islist;
}

