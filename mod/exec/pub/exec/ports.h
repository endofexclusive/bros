/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

#ifndef EXEC_PORTS_H
#define EXEC_PORTS_H

#include <stddef.h>
#include <exec/lists.h>
#include <exec/mutex.h>

struct Task;

struct MsgPort {
  struct {
    struct IntLock   lock;
    struct List      msglist;
  } inv;
  /* NOTE: no arbitration on SigBit and SigTask */
  struct Task     *sigtask;
  signed char      sigbit;
};

struct Message {
  /*
   * Part of MsgPort->msglist invariant:
   * - succ, pred, type
   * Sender sets type to NT_MESSAGE, replier sets NT_REPLYMSG.
   * Everything else is owned by the message owner.
   */
  struct Node      node;
  struct MsgPort  *replyport;
  size_t           length;
};

#endif

