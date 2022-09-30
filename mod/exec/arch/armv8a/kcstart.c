/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

#include <armv8a.h>
#include <priv.h>
#include <port.h>
#include <arch_priv.h>
#include <arch_expects.h>

#define LOG_MASK_INFO 2
#if 1
#define LOG_MASK LOG_MASK_INFO
#else
#define LOG_MASK 0
#endif

Lib *AbsExecBase;

static struct {
  union {
    MemAlign align;
    LibOp op;
  } op;
  Lib lib;
} theexecbase;

struct PortCPU {
  ExecCPU cpu;
  /* Set as sp in port_switch_tasks() when no task assigned to CPU. */
  struct intframe tmpstack;
  /* FIXME: get rid of linker stack because we have this */
  unsigned long long isrstack[4096 / sizeof (unsigned long long)];
};

ExecCPU *port_alloc_cpu(Lib *lib)
{
  return lAllocMem(lib, sizeof (struct PortCPU), MEMF_CLEAR | MEMF_ANY);
}

void kcstart(unsigned long id) {
  Lib *const lib = &theexecbase.lib;
  AbsExecBase = lib;
  iRawIOInit(lib);
  initlib(lib);
  lib->minstack = 8 * 1024;
  lib->trapcode = default_trapcode;

  /* we may now call exec functions but not allocate memory */
  if (LOG_MASK & LOG_MASK_INFO) {
    kprint_cpu(lib);
  }
  addmem0(lib, _addmem_bottom, _addmem_top);
  setexc(lib, id);
  collectres(lib, _rom_begin, _rom_end);
  start_on_primary(lib, id);
}

void *port_getstack(const Task *const task)
{
  return task->arch;
}

static void trampo(void) {
  task_entry(AbsExecBase);
}

void port_prepstack(Lib *lib, Task *task) {
  TaskArch *arch;

  arch = (TaskArch *) ((unsigned long) task->spupper & ~15U);
  arch--;
  arch->x30 = (unsigned long) trampo;
  task->arch = arch;
  task->canaries.num = 5;
}

int port_interrupt_is_enabled(int level) {
  return (level & SPSR_DAIF_I) == 0;
}

void theexc(Lib *lib, struct fullframe *ff) {
  ExecCPU *cpu;

  cpu = port_get_cpu();
  if (cpu->isr_nest == 0 && cpu->switch_disable == 0) {
    Task *task;

    task = port_ThisTask(lib);
    task->trapcode(lib, task->trapdata, ff);
  } else {
    default_trapcode(lib, NULL, ff);
  }

  /* Do not re-execute trapped instruction. */
  ff->elr += 4;
}

void port_halt(void) {
  port_disable_interrupts();
  while (1);
}

