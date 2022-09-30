/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

#include <priv.h>

Task *iCreateTask(
  Lib *lib,
  char *name,
  int priority,
  void (init)(Lib *lib),
  void *user,
  List *cleanup,
  size_t stacksize
) {
  MemRequest req[2];
  MemList *ml;
  Node *node;

  if (stacksize < lib->minstack) {
    stacksize = lib->minstack;
  }
  req[0].attr = MEMF_CLEAR | MEMF_ANY;
  req[0].size = sizeof (Task);
  req[1].attr = MEMF_ANY;
  req[1].size = stacksize;
  ml = lAllocEntry(lib, req, NELEM(req));
  if (ml == NULL) {
    return NULL;
  }

  Task *task = ml->entry[0].ptr;
  task->user = user;
  task->splower = ml->entry[1].ptr;
  task->spupper = (char *) ml->entry[1].ptr + stacksize;
  task->node.pri = priority;
  task->node.type = NT_TASK;
  task->node.name = name;
  task->init = init;
  iNewList(lib, &task->cleanlist);
  iAddTail(lib, &task->cleanlist, &ml->node);

  if (cleanup) {
    while ((node = iRemHead(lib, cleanup))) {
      iAddTail(lib, &task->cleanlist, node);
    }
  }
  task = lAddTask(lib, task);

  return task;
}

MsgPort *iCreateMsgPort(Lib *lib) {
  MsgPort *port;
  int sigbit;

  port = lAllocMem(lib, sizeof *port, MEMF_CLEAR | MEMF_ANY);
  if (port == NULL) {
    return NULL;
  }

  sigbit = lAllocSignal(lib, -1);
  if (sigbit < 0) {
    lFreeMem(lib, port, sizeof *port);
    return NULL;
  }

  port->sigbit = sigbit;
  port->sigtask = lFindTask(lib);
  iNewList(lib, &port->inv.msglist);
  lInitIntLock(lib, &port->inv.lock);

  return port;
}

void iDeleteMsgPort(Lib *lib, MsgPort *port) {
  if (port == NULL) {
    return;
  }
  lFreeSignal(lib, port->sigbit);
  lFreeMem(lib, port, sizeof *port);
}

IORequest *iCreateIORequest(
  Lib *lib,
  MsgPort *port,
  size_t size
) {
  IORequest *ior;

  /* for convenience with CreateMsgPort() */
  if (port == NULL) {
    return NULL;
  }
  ior = lAllocMem(lib, size, MEMF_CLEAR | MEMF_ANY);
  if (ior == NULL) {
    return NULL;
  }
  ior->message.node.type = NT_REPLYMSG;
  ior->message.replyport = port;
  ior->message.length = size;
  return ior;
}

void iDeleteIORequest(Lib *lib, IORequest *ior) {
  if (ior == NULL) {
    return;
  }
  lFreeMem(lib, ior, ior->message.length);
}

