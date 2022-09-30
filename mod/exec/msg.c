/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2019-2022 Martin Åberg */

#include <priv.h>

static void putit(Lib *lib, MsgPort *port, Message *msg, int type) {
  lObtainIntLock(lib, &port->inv.lock);
  msg->node.type = type;
  iAddTail(lib, &port->inv.msglist, &msg->node);
  lReleaseIntLock(lib, &port->inv.lock);

  /* FIXME: port and sigtask may have disappeared */
  if (port->sigtask) {
    lSignal(lib, port->sigtask, 1U << port->sigbit);
  }
}

void iPutMsg(Lib *lib, MsgPort *port, Message *msg) {
  putit(lib, port, msg, NT_MESSAGE);
}

Message *iGetMsg(Lib *lib, MsgPort *port) {
  Message *msg;

  lObtainIntLock(lib, &port->inv.lock);
  msg = (Message *) iRemHead(lib, &port->inv.msglist);
  lReleaseIntLock(lib, &port->inv.lock);

  return msg;
}

int iReplyMsg(Lib *lib, Message *msg) {
  MsgPort *port;

  port = msg->replyport;
  if (!port) {
    /* msg belongs to no-one from now */
    msg->node.type = NT_FREEMSG;
    return 0;
  }
  putit(lib, port, msg, NT_REPLYMSG);
  return 1;
}

Message *iWaitPort(Lib *lib, MsgPort *const port) {
  Message *msg;

  while (1) {
    lObtainIntLock(lib, &port->inv.lock);
    msg = (Message *) iGetHead(lib, &port->inv.msglist);
    lReleaseIntLock(lib, &port->inv.lock);
    if (msg) {
      break;
    }
    lWait(lib, 1U << port->sigbit);
  }
  return msg;
}

void iWaitMsg(Lib *lib, Message *msg) {
  MsgPort *port;
  unsigned int sigmask;

  port = msg->replyport;
  sigmask = 1U << port->sigbit;
  while (1) {
    /* lock so type-and-on-port invariant holds */
    lObtainIntLock(lib, &port->inv.lock);
    if (msg->node.type == NT_REPLYMSG) {
      /* msg is now in port */
      iRemove(lib, &msg->node);
      lReleaseIntLock(lib, &port->inv.lock);
      return;
    }
    lReleaseIntLock(lib, &port->inv.lock);
    lWait(lib, sigmask);
  }
}

