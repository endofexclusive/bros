/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

#ifndef EXEC_DEVICES_H
#define EXEC_DEVICES_H

#include <exec/libraries.h>
#include <exec/ports.h>

struct IORequest;

struct Device {
  struct Library   lib;
  /* capabilities such as command set, number of units, etc. */
  char *cap;
};

struct Unit {
  struct Node      node;
  struct MsgPort   port;    /* Queue for unprocessed messages */
  short            opencount;
};

/* starts out (ends) as LibraryOp but different parameters */
struct DeviceOp {
  void (*AbortIO)(
    struct Device *dev,
    struct IORequest *ior
  );
  void (*BeginIO)(
    struct Device *dev,
    struct IORequest *ior
  );
  struct Segment *(*Expunge)(
    struct Device *dev
  );
  void (*Close)(
    struct Device *dev,
    struct IORequest *ior
  );
  void (*Open)(
    struct Device *dev,
    struct IORequest *ior,
    int unitnum
  );
};

#endif

