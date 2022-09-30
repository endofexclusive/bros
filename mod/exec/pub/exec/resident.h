/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

#ifndef EXEC_RESIDENT_H
#define EXEC_RESIDENT_H

#include <exec/lists.h>

struct ExecBase;
struct Library;
struct Segment;

struct ResidentInfo {
  char  *name;
  char  *idstring;
  short  type;
  short  version;
};

/* Used to bring together ROM modules to the system. */
struct Resident {
  unsigned long            matchword;
  const struct Resident   *matchtag;
  void                    *endskip;
  struct ResidentInfo      info;
  short                    pri;
  unsigned char            flags;

  union {
    void *data;

    struct {
      /* return 0 iff success */
      int (*f)(
        struct ExecBase *exec,
        void *posdata,
        struct Segment *segment
      );
      int possize;
    } direct;

    struct ResidentAuto {
      struct Library *(*f)(
        struct ExecBase *exec,
        struct Library *l,
        struct Segment *segment
      );
      const void *optable;
      short opsize;
      short possize;
    } iauto;
  } init;
};

#define RTC_MATCHWORD   (0xDEAD)
#define RTF_AUTOINIT    0x80
/*
 * Init levels:
 *  0: never run with InitCode()
 *  1: reserved
 *  2: one processor
 *  3: multi processor
 */
#define RTF_LEVEL       0x7F

/* Used to put (constant) Resident records on reslist */
struct ResidentNode {
  struct Node      node;
  struct Resident *res;
};

#endif

