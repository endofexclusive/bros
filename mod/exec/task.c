/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2019-2022 Martin Åberg */

#include <stdint.h>
#include <priv.h>
#include <port.h>

#if 0
#define dbg(...) kprintf(lib, __VA_ARGS__)
#else
#define dbg(...)
#endif

static void *set_canaries(StackCanaries *can, void *dest) {
  can->p = (int *) (
    ((uintptr_t) dest + (sizeof (int) - 1)) &
    ~(sizeof (int) - 1)
  );
  for (int i = 0; i < can->num; i++) {
    can->p[i] = i;
  }
  return &can->p[can->num];
}

void check_canaries(Lib *lib, StackCanaries *can) {
  for (int i = 0; i < can->num; i++) {
    if (can->p[i] != i) {
      lAlert(lib, Alert_Canaries);
      return;
    }
  }
}

Task *iAddTask(Lib *lib, Task *task) {
  port_prepstack(lib, task);
  task->splower = set_canaries(&task->canaries, task->splower);

  dbg("AddTask() %-8s @ %p", task->node.name, (void *) task);
  dbg(" arch @ %p [%p..%p]\n", task->arch, task->splower,
   task->spupper);

  task->attached = 0;
  task->state    = TS_READY;
  task->sigalloc = SIGF_SINGLE;
  task->sigwait  = 0;
  task->sigrecvd = 0;
  if (task->trapcode == NULL) {
    task->trapcode = lib->trapcode;
  }

  lObtainIntLock(lib, &lib->tasklock);
  iEnqueue(lib, &lib->taskready, &task->node);
  lReleaseIntLock(lib, &lib->tasklock);
  lReschedule(lib);

  return task;
}

/*
 * Requirements for removing a task:
 * - Optionally deallocate memory, incl stack and Task - but
 *   only after it has been switched out for the last time.
 * - Shall be possible to announce removal to others with
 *   PutMsg().
 *
 * Approach:
 * - The task sets its state to TS_REMOVING.
 * - The removed task is moved to the ExecBase.removed list.
 * - The switch low-level code changes TS_REMOVING to TS_REMOVED
 *   after the final switch-out and keeps it on removed list.
 *   It also sends a signal to the cleanup task.
 * - The cleanup task scans ExecBase.removed for TS_REMOVED
 *   tasks and processes the entries in the task cleanlist.
 * - The task cleanlist can do these actions:
 *   - If NT_MESSAGE then ReplyMessage() on NT_MESSAGE
 *   - if NT_MEMLIST then FreeEntry()
 */
void iRemTask(Lib *lib) {
  Task *task;

  task = port_ThisTask(lib);
  KASSERT(task->state == TS_READY);

  lObtainIntLock(lib, &lib->tasklock);
  iRemove(lib, &task->node);
  task->state = TS_REMOVING;
  iAddTail(lib, &lib->taskremoved, &task->node);
  lReleaseIntLock(lib, &lib->tasklock);

  lSwitch(lib);
  lAlert(lib, AT_DeadEnd | AN_ExecLib | AG_RemTask);
}

Task *iFindTask(Lib *lib) {
  return port_ThisTask(lib);
}

enum {
  SIGNUM_MAX = sizeof (unsigned int) * 8 - 1,
};

int iAllocSignal(Lib *lib, const int reqnum) {
  int signum;
  unsigned int target;

  Task *task = port_ThisTask(lib);
  if (0 <= reqnum) {
    /* Try fulfill user request. */
    target = 1U << reqnum;
    if (target & task->sigalloc) {
      /* Requested signal is already allocated. */
      return -1;
    }
    signum = reqnum;
  } else {
    /* Search for a free signal. */
    signum = SIGNUM_MAX;
    target = 1U << SIGNUM_MAX;
    while (target) {
      if (0 == (target & task->sigalloc)) {
        /* This one is free. */
        break;
      }
      signum--;
      target >>= 1;
    }
    if (target == 0) {
      return -1;
    }
  }

  task->sigalloc |= target;
  lObtainIntLock(lib, &lib->tasklock);
  task->sigwait &= ~target;
  task->sigrecvd &= ~target;
  lReleaseIntLock(lib, &lib->tasklock);

  return signum;
}

void iFreeSignal(Lib *lib, int signum) {
  port_ThisTask(lib)->sigalloc &= ~(1U << signum);
}

unsigned int iClearSignal(Lib *lib, unsigned int sigmask) {
  unsigned int old;
  unsigned int keep;
  Task *task;

  keep = ~sigmask;
  task = port_ThisTask(lib);

  lObtainIntLock(lib, &lib->tasklock);
  old = task->sigrecvd;
  task->sigrecvd = old & keep;
  lReleaseIntLock(lib, &lib->tasklock);

  return old;
}

void iSignal(Lib *lib, Task *task, unsigned int sigmask) {
  lObtainIntLock(lib, &lib->tasklock);
  task->sigrecvd |= sigmask;
  if (task->state != TS_WAIT) {
    goto out;
  }
  if ((task->sigrecvd & task->sigwait) == 0) {
    goto out;
  }
  iRemove(lib, &task->node);
  task->state = TS_READY;
  iEnqueue(lib, &lib->taskready, &task->node);
  lReleaseIntLock(lib, &lib->tasklock);
  lReschedule(lib);
  return;

out:
  lReleaseIntLock(lib, &lib->tasklock);
}

unsigned int iWait(Lib *lib, unsigned int sigmask) {
  unsigned int match;
  Task *task;

  task = port_ThisTask(lib);
  KASSERT(task->state == TS_READY);

  while (1) {
    lObtainIntLock(lib, &lib->tasklock);
    match = task->sigrecvd & sigmask;
    if (match) {
      task->sigrecvd &= ~sigmask;
      lReleaseIntLock(lib, &lib->tasklock);
      break;
    }
    task->sigwait = sigmask;
    iRemove(lib, &task->node);
    task->state = TS_WAIT;
    iAddTail(lib, &lib->taskwait, &task->node);
    lReleaseIntLock(lib, &lib->tasklock);

    lSwitch(lib);
  }
  KASSERT(task->state == TS_READY);

  return match;
}

int iSetTaskPri(Lib *lib, Task *task, int priority) {
  int old;
  int doreschedule;

  doreschedule = 0;
  lObtainIntLock(lib, &lib->tasklock);
  old = task->node.pri;
  task->node.pri = priority;
  if (task->state == TS_READY) {
    iRemove(lib, &task->node);
    iEnqueue(lib, &lib->taskready, &task->node);
    doreschedule = 1;
  }
  lReleaseIntLock(lib, &lib->tasklock);
  if (doreschedule) {
    lReschedule(lib);
  }
  return old;
}

