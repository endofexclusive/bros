/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

/* 
 * This file is a template for porting exec.library to a new
 * architecture.
 */

#include <stdint.h>
#include <priv.h>
#include <port.h>

struct TaskArch {
  /* example task context */
  unsigned r0, r1, sp, fp, lr, pc, psr;
};

void _start(void);
void *kcstart(void);

/* Addresses provided by linker */
extern char _romtag_begin[]; /* Resident tags start here */
extern char _romtag_end[];

static Lib *AbsExecBase;

/* Allocate static memory for exec library base */
static struct {
  union {
    MemAlign align;
    LibOp op;
  } op;
  Lib lib;
  Task *ThisTask;
} priv;

/*
 * _start() is is typically implemented in assembly.
 * This C version is to catch the logic.
 */
void _start(void) {
  void *first_context;

  /* TODO: Clear .bss */
  /* TODO: Set initial stack */
  /* TODO: Low-level initialization of run-time */
  first_context = kcstart();
  /* TODO: switch to first_context */
  (void) first_context;
}

extern char _addmem_bottom;
extern char _addmem_top;

/* return: context (stack pointer) of first task. */
void *kcstart(void) {
  Lib *lib;

  lib = &priv.lib;
  AbsExecBase = lib;
  iRawIOInit(lib);
  initlib(lib);
  lib->minstack = 256;
  addmem0(lib, &_addmem_bottom, &_addmem_top);
  /* TODO: Initialize exceptions */
  collectres(lib, _romtag_begin, _romtag_end);

  /* Task switching should not be activated yet. */
  priv.ThisTask = iCreateTask(lib, "init", 0, NULL, 0, NULL, 512);
  KASSERT(priv.ThisTask);

  return &priv.ThisTask->arch[1];
}

/*
 * This is called by AddTask() to perform architecture specific
 * setup of the task context
 */
void port_prepstack(Lib *lib, Task *task) {
  TaskArch *arch;

  arch = (TaskArch *) ((uintptr_t) task->spupper & ~7U);
  arch--;
  arch->r0 = (unsigned) lib;
  arch->lr = ~0U;
  arch->pc = (unsigned) task_entry;
  arch->psr = 0x1234;
  task->arch = arch;
  task->canaries.num = 3;
}

Task *port_ThisTask(Lib *lib) {
  KASSERT(priv.ThisTask);
  return priv.ThisTask;
}

void port_halt(void) {
  port_disable_interrupts();
  while (1) {
    ;
  }
}

void task_entry(Lib *lib) {
  KASSERT(priv.ThisTask);
  KASSERT(priv.ThisTask->init);
  priv.ThisTask->init(lib);
  lRemTask(lib);
}

