/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

#ifndef EXEC_IO_H
#define EXEC_IO_H

#include <exec/ports.h>

struct IORequest {
  struct Message   message;
  struct Device   *device;
  struct Unit     *unit;
  unsigned char    flags;
  unsigned char    command;
  char             error;
};

#define IOF_QUICK          (1<<0)

#define CMD_INVALID        0
#define CMD_RESET          1
#define CMD_READ           2
#define CMD_WRITE          3
#define CMD_UPDATE         4
#define CMD_CLEAR          5
#define CMD_STOP           6
#define CMD_START          7
#define CMD_FLUSH          8
#define CMD_FOR_API       64 /* standard API */
#define CMD_FOR_IMPL     128 /* device/driver specific */

#define IOERR_OK           0
#define IOERR_OPENFAIL     1 /* device/unit failed to open */
#define IOERR_ABORTED      2 /* AbortIO() */
#define IOERR_NOCMD        3 /* command not supported by device */
#define IOERR_UNITBUSY     4
#define IOERR_NOTFOUND     5 /* device driver not found */
#define IOERR_INVALIDUNIT  6 /* there is no such unit */

#endif

