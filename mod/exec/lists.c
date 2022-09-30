/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2019-2022 Martin Åberg */

#include <priv.h>

void iNewList(Lib *lib, List *list) {
  list->head     = (Node *) &list->tail;
  list->tail     = NULL;
  list->tailpred = (Node *) &list->head;
}

Node *iGetHead(Lib *lib, const List *list) {
  Node *node = list->head;
  if (node == (Node *) &list->tail) {
    return NULL;
  }
  return node;
}

Node *iGetTail(Lib *lib, const List *list) {
  Node *node = list->tailpred;
  if (node == (Node *) &list->head) {
    return NULL;
  }
  return node;
}

void iInsert(Lib *lib, List *list, Node *node, Node *pred) {
  if (pred == NULL) {
    iAddHead(lib, list, node);
    return;
  }
  node->succ = pred->succ;
  node->pred = pred;
  pred->succ->pred = node;
  pred->succ = node;
}

void iEnqueue(Lib *lib, List *list, Node *node) {
  Node *nextnode;

  nextnode = list->head;
  while (NULL != nextnode->succ) {
    if (nextnode->pri < node->pri) {
      break;
    }
    nextnode = nextnode->succ;
  }
  node->succ = nextnode;
  node->pred = nextnode->pred;
  nextnode->pred->succ = node;
  nextnode->pred = node;
}

void iRemove(Lib *lib, Node *node) {
  Node *succ = node->succ;
  Node *pred = node->pred;
  pred->succ = succ;
  succ->pred = pred;
}

void iAddHead(Lib *lib, List *list, Node *node) {
  node->succ = list->head;
  node->pred = (Node *) &list->head;
  list->head->pred = node;
  list->head = node;
}

void iAddTail(Lib *lib, List *list, Node *node) {
  node->pred = list->tailpred;
  node->succ = (Node *) &list->tail;
  list->tailpred->succ = node;
  list->tailpred = node;
}

Node *iRemHead(Lib *lib, List *list) {
  Node *const oldhead = list->head;
  Node *const newhead = oldhead->succ;
  if (newhead == NULL) {
    return NULL;
  }
  list->head = newhead;
  newhead->pred = (Node *) &list->head;
  return oldhead;
}

Node *iRemTail(Lib *lib, List *list) {
  Node *const oldtail = list->tailpred;
  Node *const newtail = oldtail->pred;
  if (newtail == NULL) {
    return NULL;
  }
  list->tailpred = newtail;
  newtail->succ = (Node *) &list->tail;
  return oldtail;
}

static int c_strcmp(const char *s1, const char *s2) {
  while (*s1 == *s2++) {
    if (*s1++ == '\0') {
      return 0;
    }
  }
  return 1;
}

Node *iFindName(Lib *lib, const List *list, const char *name) {
  for (Node *node = list->head; node->succ; node = node->succ) {
    if (node->name == NULL) {
      continue;
    }
    if (0 == c_strcmp(name, node->name)) {
      return node;
    }
  }
  return NULL;
}

