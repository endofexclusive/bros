/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

#ifndef EXEC_LISTS_H
#define EXEC_LISTS_H

struct Node {
  struct Node *succ;
  struct Node *pred;
  char        *name;
  short        type;
  short        pri;
};

/* Node.type */
#define NT_UNKNOWN     0
#define NT_TASK        1
#define NT_INTERRUPT   2
#define NT_DEVICE      3
#define NT_UNIT        4
#define NT_MESSAGE     5  /* Message is at port or receiver */
#define NT_FREEMSG     6
#define NT_REPLYMSG    7  /* Message has been replied */
#define NT_LIBRARY     8
#define NT_MEMHEADER   9
#define NT_MEMLIST    10
#define NT_CPU        11
#define NT_PROCESS    12
#define NT_CUSTOM     64

struct List {
  struct Node *head;
  struct Node *tail;
  struct Node *tailpred;
};

#endif

