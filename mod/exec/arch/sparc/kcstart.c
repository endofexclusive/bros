/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022 Martin Åberg */

#include <sparc.h>
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
IntLock *tasklockp;

static struct {
  union {
    MemAlign align;
    LibOp op;
  } op;
  Lib lib;
} theexecbase;

struct PortCPU {
  ExecCPU cpu;
  char ipi_command[8];
  /* Set as sp in port_switch_tasks() when no task assigned to CPU. */
  struct {
    struct intframe intframe;
    unsigned long long il[16*4 / sizeof (unsigned long long)];
  } tmpstack;
  unsigned long long isrstack[4096 / sizeof (unsigned long long)];
};

ExecCPU *port_alloc_cpu(Lib *lib)
{
  return lAllocMem(lib, sizeof (struct PortCPU), MEMF_CLEAR | MEMF_ANY);
}

void kcstart(void *ramtop, unsigned long id) {
  Lib *const lib = &theexecbase.lib;
  AbsExecBase = lib;
  iRawIOInit(lib);
  initlib(lib);
  lib->minstack = 8 * 1024;
  lib->trapcode = default_trapcode;
  tasklockp = &lib->tasklock;

  /* we may now call exec functions but not allocate memory */
  if (LOG_MASK & LOG_MASK_INFO) {
    kprint_cpu(lib);
  }
  /* take ramtop as top of stack set by boot loader */
  addmem0(lib, _addmem_bottom, ramtop ? ramtop : _addmem_top);
  setexc(lib, id);
  collectres(lib, _rom_begin, _rom_end);
  start_on_primary(lib, id);
}

void *port_getstack(const Task *const task)
{
  return task->arch;
}

extern char trampo;

void port_prepstack(Lib *lib, Task *task) {
  TaskArch *arch;

  arch = (TaskArch *) ((unsigned) task->spupper & ~7U);
  arch--;
  task->spupper = arch;
  *arch = (TaskArch) { 0 };
  /* %fp=0 marks deepest stack frame in SPARC ABI, Chapter 3 */
  arch->o7 = (unsigned int) &trampo - 8;
  arch->psr = PSR_EF | PSR_PS;
  task->arch = arch;
  /* SPARC ABI allocates stack it may never use */
  task->canaries.num = 96/4 + 5;
}

int port_interrupt_is_enabled(int level) {
  return (level & 0xf00) == 0;
}

void theexc(Lib *lib, struct fullframe *ff) {
  if ((ff->psr & PSR_PS) == 0) {
    Task *task;

    task = port_ThisTask(lib);
    task->trapcode(lib, task->trapdata, ff);
  } else {
    default_trapcode(lib, NULL, ff);
  }

  /* Do not re-execute trapped instruction. */
  ff->pc  = ff->npc;
  ff->npc = ff->npc + 4;
}

void iSyncInstructions(Lib *lib, const void *begin, size_t size) {
  ExecCPU *cpu;

  lObtainIntLock(lib, &lib->tasklock);
  cpu = port_get_cpu();
  for (Node *n = lib->cpuonline.head; n->succ; n = n->succ) {
    struct PortCPU *remote;

    remote = (struct PortCPU *) n;
    if (&remote->cpu == cpu) {
      continue;
    }
    /* Post request to others to invalidate local icache. */
    remote->ipi_command[0] = 1;
    port_send_ipi(remote->cpu.id);
  }
  lReleaseIntLock(lib, &lib->tasklock);
  /* Invalidate icache on local CPU. */
  sparc_sync_instructions();
}

