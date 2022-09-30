/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

#include "test.h"
#define vtest(cond) if (!(cond)) lAlert(exec, AT_DeadEnd | __LINE__)

static struct Task *inittask;
static struct Task *t0;
static struct Task *t1;
static struct MsgPort *p0;
static volatile int state = 0;

static const unsigned int ROUNDS = 1234;

/* init order: f0, f1 which is currently not enforced */
static void f0(struct ExecBase *exec) {
  p0 = lCreateMsgPort(exec);
  vtest(p0);

  state = 1;
  struct Task *sigtask = inittask;
  vtest(sigtask);
  lSignal(exec, sigtask, SIGF_SINGLE);

  for (unsigned int i = 0; i < ROUNDS; i++) {
    struct Message *msg;
    {
      struct Message *msgx;
      msgx = lWaitPort(exec, p0);
      vtest(msgx != NULL);
      msg = lGetMsg(exec, p0);
      vtest(msgx == msg);
    }
    msg->length++;
    int ret;
    ret = lReplyMsg(exec, msg);
    vtest(ret == 1);
  }

  lDeleteMsgPort(exec, p0);
  p0 = NULL;
}

static void f1(struct ExecBase *exec) {
  struct MsgPort *p1;
  p1 = lCreateMsgPort(exec);
  vtest(p1->sigtask != p0->sigtask);

  struct Message msg0;
  msg0.replyport = p1;
  msg0.length = 0;

  for (unsigned int i = 0; i < ROUNDS; i++) {
    msg0.length = 2 * i;
    lPutMsg(exec, p0, &msg0);
    struct Message *rmsg = NULL;
    while (1) {
      lWait(exec, 1U << p1->sigbit);
      rmsg = lGetMsg(exec, p1);
      if (rmsg) {
        break;
      }
    }
    vtest(rmsg == &msg0);
    vtest(rmsg->length == 2 * i + 1);
  }

  state = 2;
  struct Task *sigtask = inittask;
  vtest(sigtask);
  lSignal(exec, sigtask, SIGF_SINGLE);
  lDeleteMsgPort(exec, p1);
  p0 = NULL;
}

#define STACK_SIZE 1024
void test_msg0(struct ExecBase *exec) {
  inittask = lFindTask(exec);
  vtest(inittask);
  t0 = lCreateTask(exec, "t0", 1, f0, 0, NULL, STACK_SIZE);
  vtest(t0);
  while (state != 1) {
    lWait(exec, SIGF_SINGLE);
  }
  t1 = lCreateTask(exec, "t1", 2, f1, 0, NULL, STACK_SIZE);
  vtest(t1);
  while (state != 2) {
    lWait(exec, SIGF_SINGLE);
  }
}

