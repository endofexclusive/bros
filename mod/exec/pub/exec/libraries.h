/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

#ifndef EXEC_LIBRARIES_H
#define EXEC_LIBRARIES_H

#include <exec/lists.h>

struct LibraryOp;
struct Segment;

/*
 * Covered by ExecBase LibLock invariant:
 * - node, opencount, flags
 *
 * A user which has done OpenLibrary() has read-only access to:
 * - Anything not covered by the liblock invariant
 * - node.name, node.type
 */
struct Library {
  struct Node  node;
  char        *idstring;
  void        *allocatedmemory;
  short        allocatedsize;
  short        version;
  short        opencount;
  short        flags;
};

#define LIBF_DELEXP        (1<<0)

struct LibraryOp {
  /* return: free indication */
  struct Segment *(*Expunge)(
    struct Library *lib
  );
  void (*Close)(
    struct Library *lib
  );
  struct Library *(*Open)(
    struct Library *lib
  );
};

#endif

