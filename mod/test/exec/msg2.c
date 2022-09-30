/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

#include "test.h"
#define vtest(cond) if (!(cond)) lAlert(exec, AT_DeadEnd | __LINE__)
#define dbg(...)

static volatile long fxcount = 0;
static struct Task *fxsigtask;
static int fxsigbit;
static struct IntLock fxlock;

static void fx(struct ExecBase *exec) {
  struct Library *l = lOpenLibrary(exec, "hej123", 0);
  vtest(l == NULL);
  long mycount;
  lObtainIntLock(exec, &fxlock);
  fxcount--;
  mycount = fxcount;
  lReleaseIntLock(exec, &fxlock);
  (void) mycount;
  dbg("b %ld\n", mycount);
  if (fxcount == 0) {
    lSignal(exec, fxsigtask, 1U<<fxsigbit);
  }
}

#define STACK_SIZE 512
void test_msg2(struct ExecBase *exec) {
  dbg("enter\n");
  const long NTASK = 128*2*4 + 15;
  lInitIntLock(exec, &fxlock);
  fxsigtask = lFindTask(exec);
  vtest(fxsigtask);
  fxsigbit = lAllocSignal(exec, -1);
  vtest(0 < fxsigbit);
  fxcount = NTASK;
  for (long i = 0; i < NTASK; i++) {
    struct Task *t;
    t = lCreateTask(exec, "blupp", 3, fx, 0, NULL, STACK_SIZE);
    dbg("t=%p\n", t);
    if (t == NULL) {
      kprintf(exec, "ERROR: CreateTask()\n");
    }
    vtest(t);
    dbg("x %ld\n", i);
  }
  while (1) {
    int done = 0;
    lObtainIntLock(exec, &fxlock);
    done = !fxcount;
    lReleaseIntLock(exec, &fxlock);
    if (done) {
      break;
    }
    lWait(exec, 1U<<fxsigbit);
  }
  lFreeSignal(exec, fxsigbit);
  dbg("leave\n");
}

