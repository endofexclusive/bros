/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2021-2022 Martin Åberg */

#include <priv.h>
#include <port.h>

void iInitMutex(Lib *lib, Mutex *ctx) {
  iNewList(lib, &ctx->waitqueue);
  ctx->nest = 0;
  lInitIntLock(lib, &ctx->lock);
  ctx->owner = NULL;
}

typedef struct {
  Node node;
  Task *task;
} Waiter;

/*
 * ObtainMutex() and ReleaseMutex() are called by init code
 * before ThisTask can be retrieved (via Cpu). Such calls will
 * hit the below empty functions. The proper functions are
 * installed with SetFunction() by the first task.
 */
void iObtainMutex(Lib *lib, Mutex *ctx) {
}
void iReleaseMutex(Lib *lib, Mutex *ctx) {
}

void iObtainMut(Lib *lib, Mutex *ctx) {
  Waiter waiter;
  Task *thistask;
  Task *previous_owner;

  thistask = port_ThisTask(lib);

  lObtainIntLock(lib, &ctx->lock);
  {
    previous_owner = ctx->owner;
    if (previous_owner == NULL) {
      ctx->owner = thistask;
    }
  }
  lReleaseIntLock(lib, &ctx->lock);

  if (previous_owner == NULL) {
    KASSERT(ctx->nest == 0);
    return;
  }

  waiter.task = thistask;
  lClearSignal(lib, SIGF_SINGLE);

  lObtainIntLock(lib, &ctx->lock);
  {
    if (ctx->owner == NULL) {
      ctx->owner = thistask;
      KASSERT(ctx->nest == 0);
      /* We got it after testing the first time. */
      lReleaseIntLock(lib, &ctx->lock);
      return;
    }
    if (ctx->owner == thistask) {
      lReleaseIntLock(lib, &ctx->lock);
      ctx->nest++;
      KASSERT(ctx->nest);
      return;
    }
    iAddTail(lib, &ctx->waitqueue, (Node *) &waiter);
  }
  lReleaseIntLock(lib, &ctx->lock);

  lWait(lib, SIGF_SINGLE);
  KASSERT(ctx->nest == 0);
  KASSERT(ctx->owner == thistask);
}

void iReleaseMut(Lib *lib, Mutex *ctx) {
  Task *sigtask;

  KASSERT(port_ThisTask(lib) == ctx->owner);
  if (ctx->nest) {
    ctx->nest--;
    return;
  }

  sigtask = NULL;

  lObtainIntLock(lib, &ctx->lock);
  {
    Waiter *waiter;

    waiter = (Waiter *) iRemHead(lib, &ctx->waitqueue);
    if (waiter) {
      sigtask = waiter->task;
    }
    ctx->owner = sigtask;
  }
  lReleaseIntLock(lib, &ctx->lock);

  if (sigtask) {
    lSignal(lib, sigtask, SIGF_SINGLE);
  }
}

