/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

#ifndef DEVICES_SERIAL_H
#define DEVICES_SERIAL_H

#include <exec/io.h>

struct IOExtSer {
  struct IORequest ior;
  int              actual;
  int              length;
  void            *data;
  long             baud;
};

#define SDCMD_QUERY       (CMD_FOR_API +  0)
#define SDCMD_BREAK       (CMD_FOR_API +  1)
#define SDCMD_GETPARAMS   (CMD_FOR_API +  2)
#define SDCMD_SETPARAMS   (CMD_FOR_API +  3)

#endif

