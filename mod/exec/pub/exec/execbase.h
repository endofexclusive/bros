/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

#ifndef EXEC_EXECBASE_H
#define EXEC_EXECBASE_H

#include <stddef.h>
#include <exec/mutex.h>
#include <exec/libraries.h>

struct ExecBase;
struct Task;
struct IntList;

struct Interrupt {
  struct Node      node;
  void            *data;
  void           (*code)(struct ExecBase *lib, void *data);
};

struct ExecCPU {
  struct Node      node;
  /* local access, prevents switch by Reschedule() etc in ISR */
  volatile long    switch_disable;
  /* local access. only accessed in isr */
  long             isr_nest;
  /* local access. set in inter-processor-interrupt */
  volatile long    switch_needed;
  /* read only. map ExecCPU to a CPU (hardware) id */
  unsigned long    id;
  /* local access. can be same on multiple CPU:s */
  struct Task     *thistask;
  /* local access. */
  struct Task     *removing;
  /*
   * written by scheduler which owns the CPU.
   * Can be same on multiple CPU:s
   */
  struct Task *volatile heir;
  /* read only. */
  struct Task     *idle;
};

struct ExecBase {
  struct Library   lib;

  struct List      memlist; /* MemHeader */
  struct Mutex     memlock;
  struct List      liblist; /* Library */
  struct Mutex     liblock; /* liblist nodes, opencount, flags */
  struct List      devlist; /* Device */
  struct Mutex     devlock; /* devlist nodes, opencount, flags */

  struct List      reslist; /* ResidentNode, read only */
  /* An array of lists containing interrupt server nodes. */
  struct IntList  *intserver;

  struct List      taskready;   /* Task */
  struct List      taskremoved; /* Task */
  struct List      taskwait;    /* Task */
  struct List      cpuonline;   /* ExecCPU */
  struct List      cpuoffline;  /* ExecCPU */
  /*
   * invariant for all task lists, all tasks on the lists,
   * cpuonline, cpuoffline.
   */
  struct IntLock   tasklock;

  struct Task     *cleantask;

  /* minimum stack buffer size for CreateTask() */
  size_t           minstack;

  /* for tasks which don't provide their own trapcode() */
  void (*trapcode)(
    struct ExecBase *lib,
    void *data,
    void *info
  );
};

#endif

