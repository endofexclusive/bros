/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

#include "test.h"

#if 0
#define info(...) kprintf(exec, __VA_ARGS__)
#else
#define info(...)
#endif

static void kputchar(void *arg, int c) {
  lRawPutChar((struct ExecBase *) arg, c);
}
void kprintf(struct ExecBase *exec, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  lRawDoFmt(exec, kputchar, exec, fmt, ap);
  va_end(ap);
}

static int init2(struct ExecBase *exec, void *data,
 struct Segment *segment) {
  info("%s: test_list0\n", __func__);
  test_list0(exec);

  info("%s: test_xyz\n", __func__);
  test_xyz(exec);

  info("%s: test_msg0\n", __func__);
  test_msg0(exec);

  info("%s: test_msg2\n", __func__);
  test_msg2(exec);

  info("%s: done\n", __func__);

  return 0;
}

const struct Resident RES_FOR_LEVEL_2 = {
  .matchword      = RTC_MATCHWORD,
  .matchtag       = &RES_FOR_LEVEL_2,
  .endskip        = &((struct Resident *) (&RES_FOR_LEVEL_2))[1],
  .flags          = 3,
  .info.name      = "test-for-level-3",
  .info.idstring  = "test-for-level-3 0.1 2021-07-12",
  .info.type      = NT_UNKNOWN,
  .info.version   = 1,
  .pri            = -20,
  .init.direct.f  = init2,
};

int _start(void);
int _start(void) {
        return -1;
}

