/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022 Martin Åberg */

#include <riscv.h>
#include <priv.h>
#include <port.h>
#include <arch_priv.h>
#include <arch_expects.h>
#include <sbi/sbi.h>

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
  struct excframe tmpstack;
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

  arch = (TaskArch *) ((unsigned long) task->spupper & ~7U);
  arch--;
  task->spupper = arch;
  arch->ra = (unsigned long) trampo;
  task->arch = arch;
  task->canaries.num = 5;
}

int port_interrupt_is_enabled(int level) {
  return level & RISCV_SSTATUS_SIE;
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
  {
    const unsigned short *inst;
    int instlen;

    instlen = 4;
    inst = (const unsigned short *) ff->excframe.sepc;
    if ((*inst & 3) != 3) {
      /* compressed extension */
      instlen = 2;
    }
    ff->excframe.sepc += instlen;
  }
}

/*
 * There is no guarantee that this CPU did the instruction
 * update: do fence.i on local CPU and put request on remote.
 */
void iSyncInstructions(Lib *lib, const void *begin, size_t size) {
  int level;
  ExecCPU *cpu;

  level = port_disable_interrupts();
  KASSERT(port_interrupt_is_enabled(level));
  cpu = port_get_cpu();
  cpu->switch_disable = 1;
  port_enable_interrupts(level);

  riscv_fence_rw();
  riscv_fence_i();
  sbi_remote_fence_i(0, -1);

  level = port_disable_interrupts();
  cpu = switch_tasks_if_needed(lib, cpu);
  cpu->switch_disable = 0;
  port_enable_interrupts(level);
}

void port_start_other_processors(Lib *lib) {
  int ncpu;
  unsigned int nstarted;

  if (sbi_probe_extension(SBI_EID_HSM).value == 0) {
    return;
  }

  ncpu = port_get_ncpu(lib);
  nstarted = 0;
  for (int i = 0; i < ncpu; i++) {
    struct sbiret ret;
    ExecCPU *cpu;

    ret = sbi_hart_get_status(i);
    if (ret.error != SBI_SUCCESS) {
      continue;
    }
    if (ret.value != SBI_HSM_HART_STATUS_STOPPED) {
      continue;
    }
    lObtainIntLock(lib, &lib->tasklock);
    cpu = findcpu(lib, i);
    lReleaseIntLock(lib, &lib->tasklock);
    KASSERT(cpu);
    sbi_hart_start(i, (unsigned long) &entry_for_secondary,
     (unsigned long) cpu);
    nstarted++;
  }
}

int port_get_ncpu(Lib *lib) {
  int n;

  if (sbi_probe_extension(SBI_EID_HSM).value == 0) {
    return 1;
  }

  n = 0;
  for (int i = 0; i < 32; i++) {
    struct sbiret ret;

    ret = sbi_hart_get_status(i);
    if (ret.error != SBI_SUCCESS) {
      continue;
    }
    n++;
  }

  return n;
}

void port_send_ipi(unsigned int target_cpu) {
  sbi_send_ipi(1UL << target_cpu, 0);
}

void port_halt(void) {
  sbi_system_reset(SBI_SRST_TYPE_SHUTDOWN,
   SBI_SRST_REASON_NO_REASON);
  while (1);
}

