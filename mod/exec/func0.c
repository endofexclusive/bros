/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

#include <priv.h>
#include <port.h>
#include <exec/offset.h>

static void freenode(Lib *lib, Node *node) {
  switch (node->type) {
    case NT_MEMLIST:
      lFreeEntry(lib, (MemList *) node);
      break;
    case NT_MESSAGE:
      lReplyMsg(lib, (Message *) node);
      break;
  }
}

/*
 * This function runs in a dedicated task. It has two
 * responsibilities:
 *   1. Remove tasks from the scheduler when finished
 *   2. Perform per-task cleanup requests, including
 *      deallocating memory (NT_MEMLIST) allocated by
 *      CreateTask(). The NT_MESSAGE operation may be requested
 *      by a "parent" task to get a notification when the child
 *      has ended and is no longer in the scheduler.
 */
static void cleanup(Lib *lib) {
  iAllocSignal(lib, SIGB_CLEANUP);
  KASSERT(port_ThisTask(lib)->sigalloc & SIGF_CLEANUP);

  while (1) {
    Node *node;
    Task *task;
    List rmtask;
    List tmplist;

    lWait(lib, SIGF_CLEANUP);
    iNewList(lib, &rmtask);
    iNewList(lib, &tmplist);
    lObtainIntLock(lib, &lib->tasklock);
    /*
     * Careful not to deallocate memory for tasks which may
     * still be executing.
     */
    while ((task = (Task *) iRemHead(lib, &lib->taskremoved))) {
      if (task->state == TS_REMOVING) {
        iAddTail(lib, &tmplist, &task->node);
      } else {
        iAddTail(lib, &rmtask, &task->node);
      }
    }
    /* put back the ones which may still execute */
    while ((task = (Task *) iRemHead(lib, &tmplist))) {
      iAddTail(lib, &lib->taskremoved, &task->node);
    }
    lReleaseIntLock(lib, &lib->tasklock);

    while ((task = (Task *) iRemHead(lib, &rmtask))) {
      /* for each removed task */
      while ((node = iRemHead(lib, &task->cleanlist))) {
        /* for each cleanup operation on that task */
        iAddTail(lib, &tmplist, node);
      }
    }

    /* finally do cleanup operations for all removed tasks */
    while ((node = iRemHead(lib, &tmplist))) {
      freenode(lib, node);
    }
  }
}

/* This is the first non-idle task ever started. */
void func0(Lib *lib) {
  /* Install the _real_ ObtainMutex() and ReleaseMutex() */
  lSetFunction(lib, &lib->lib, offset_ObtainMutex,
   (void (*)(void)) iObtainMut, NULL);
  lSetFunction(lib, &lib->lib, offset_ReleaseMutex,
   (void (*)(void)) iReleaseMut, NULL);

  lib->cleantask = iCreateTask(lib, "cleanup", 10, cleanup, 0,
   NULL, 0);
  KASSERT(lib->cleantask);

  /*
   * Initialize residents before and after bringing the other
   * CPU:s online.
   */
  lInitCode(lib, 2);
  create_and_start_other_processors(lib);
  lInitCode(lib, 3);

  /* It was expected that some resident did RemTask() on us. */
  kprintf(lib, "%s", "HALT\n");
  port_halt();
}

