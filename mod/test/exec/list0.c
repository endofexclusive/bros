/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

#include "test.h"
#define vtest(cond) if (!(cond)) lAlert(exec, AT_DeadEnd | __LINE__)

void test_list0(struct ExecBase *exec) {
  struct List l;
  struct Node n1;
  struct Node n2;

  lNewList(exec, &l);
  n1.pri = 3;
  n1.name = "one";
  n2.pri = 8;
  n2.name = "two";

  vtest(!lGetHead(exec, &l));

  lAddTail(exec, &l, &n1);
  vtest(&n1 == lGetHead(exec, &l));

  lAddTail(exec, &l, &n2);
  vtest(&n1 == lGetHead(exec, &l));

  vtest(&n1 == lFindName(exec, &l, "one"));
  vtest(&n2 == lFindName(exec, &l, "two"));

  vtest(lRemHead(exec, &l));
  vtest(&n2 == lGetHead(exec, &l));
  vtest(NULL == lFindName(exec, &l, "one"));
  vtest(&n2 == lFindName(exec, &l, "two"));
  vtest(lRemHead(exec, &l));
  vtest(NULL == lFindName(exec, &l, "two"));
  vtest(!lRemHead(exec, &l));
  vtest(!lGetHead(exec, &l));

  lEnqueue(exec, &l, &n1);
  vtest(&n1 == lGetHead(exec, &l));
  lEnqueue(exec, &l, &n2);
  vtest(&n2 == lGetHead(exec, &l));
  lRemove(exec, &n1);
  vtest(&n2 == lGetHead(exec, &l));

  lEnqueue(exec, &l, &n1);
  vtest(&n2 == lGetHead(exec, &l));

  vtest(&n2 == lRemHead(exec, &l));
  vtest(&n1 == lRemHead(exec, &l));
  vtest(NULL == lRemHead(exec, &l));
  vtest(NULL == lRemHead(exec, &l));

  lAddHead(exec, &l, &n1);
  vtest(&n1 == lGetHead(exec, &l));
  vtest(&n1 == lGetTail(exec, &l));

  lAddHead(exec, &l, &n2);
  vtest(&n2 == lGetHead(exec, &l));
  vtest(&n1 == lGetTail(exec, &l));
  vtest(&n1 == lRemTail(exec, &l));
  vtest(&n2 == lRemTail(exec, &l));
  vtest(NULL == lRemTail(exec, &l));

  vtest(3 == n1.pri);
  vtest(8 == n2.pri);
  vtest(NULL == lFindName(exec, &l, "one"));
  vtest(NULL == lFindName(exec, &l, "two"));
}

