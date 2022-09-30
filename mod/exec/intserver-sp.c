/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022 Martin Åberg */

/* Interrupt server selection, single-processor version */

#include <priv.h>
#include <port.h>

/* Contains all handlers for an interrupt source (intnum) */
typedef struct IntList {
  List list;
} IntList;

static void intlist_init(Lib *lib, IntList *islist) {
  iNewList(lib, &islist->list);
}

void iAddIntServer(Lib *lib, Interrupt *inode, int intnum) {
  IntList *islist;
  int level;

  inode->node.type = NT_INTERRUPT;
  islist = &lib->intserver[intnum];
  level = port_disable_interrupts();
  iEnqueue(lib, &islist->list, &inode->node);
  port_enable_intnum(intnum);
  port_enable_interrupts(level);
}

void iRemIntServer(Lib *lib, Interrupt *inode, int intnum) {
  IntList *islist;
  int level;

  islist = &lib->intserver[intnum];
  level = port_disable_interrupts();
  iRemove(lib, &inode->node);
  if (iGetHead(lib, &islist->list) == NULL) {
    port_disable_intnum(intnum);
  }
  port_enable_interrupts(level);
}

/* called from interrupt exception handler */
void runintservers(Lib *lib, int intnum) {
  IntList *islist;
  Node *node;

  islist = &lib->intserver[intnum];
  for (node = islist->list.head; node->succ; node = node->succ) {
    Interrupt *is = (Interrupt *) node;
    is->code(lib, is->data);
  }
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

