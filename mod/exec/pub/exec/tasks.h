/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

#ifndef EXEC_TASKS_H
#define EXEC_TASKS_H

#include <exec/lists.h>

struct TaskArch;

struct StackCanaries {
  int *p;
  int num;
};

struct Task {
  struct Node      node;
  struct TaskArch *arch;
  volatile int     attached; /* to a CPU */
  char             state;
  unsigned int     sigalloc;
  unsigned int     sigwait;
  unsigned int     sigrecvd;
  void           (*init)(struct ExecBase *lib);
  void            *splower;
  void            *spupper;
  struct StackCanaries canaries;
  /*
   * Nodes on this list will be processed after RemTask(). The
   * processing takes place in a system task (not in the context
   * of the removed task).
   *
   * Processing depends on the node Type:
   * - NT_MEMLIST:        FreeEntry() will be called.
   * - NT_MESSAGE:        ReplyMsg() will be called.
   * - others:            No action
   */
  struct List      cleanlist;
  void            *trapdata;
  void           (*trapcode)(
    struct ExecBase *lib,
    void *trapdata,
    void *trapinfo
  );
  void            *user;
};

/* Task.state */
#define TS_INVALID   0
#define TS_READY     1 /* ExecBase.taskready */
#define TS_WAIT      2 /* ExecBase.taskwait */
#define TS_REMOVING  3 /* ExecBase.taskremoved */
#define TS_REMOVED   4 /* ExecBase.taskremoved */

/* Task.node.pri */
/* TASK_PRI_IDLE and all lower task priorities are reserved. */
#define TASK_PRI_IDLE      (-32767)

/* Reserved signal numbers and bits for Signal() and Wait() */
#define SIGB_SINGLE  0
#define SIGB_CLEANUP 1
#define SIGF_SINGLE  (1U << SIGB_SINGLE)
#define SIGF_CLEANUP (1U << SIGB_CLEANUP)

#endif

