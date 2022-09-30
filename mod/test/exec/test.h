/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

#include <exec/exec.h>
#include <exec/libcall.h>

void test_xyz(struct ExecBase *exec);
void test_list0(struct ExecBase *exec);
void test_msg0(struct ExecBase *exec);
void test_msg2(struct ExecBase *exec);

void kprintf(struct ExecBase *exec, const char *fmt, ...);

