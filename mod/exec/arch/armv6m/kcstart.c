/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

#include <priv.h>
#include <port.h>
#include "arch_priv.h"
#include <arch_expects.h>
#include "scs.h"

#define PRINT_CPU       1
#define PRINT_REGS      1
#if 0
#define dbg(...) kprintf(lib, __VA_ARGS__)
#else
#define dbg(...)
#endif

Lib *AbsExecBase;
/*
 * Rules for ThisTask
 * - ThisTask is only read by tasks, scheduler and context
 *   switcher
 * - ThisTask is only written at context switch
 * - ThisTask is always on a task list (Ready, Removed, Wait).
 *
 * Task lists and task structs can be read/written at any time
 * (under lock).
 */
static struct {
  union {
    MemAlign align;
    LibOp op;
  } op;
  Lib lib;
  Task *ThisTask;
} priv;

static void nvic_set_enable(int irq) {
  int index = irq >> 5;
  unsigned bit = 1u << (irq & 0x1f);
  nvic->iser[index] = bit;
}

static void nvic_clear_enable(int irq) {
  int index = irq >> 5;
  unsigned bit = 1u << (irq & 0x1f);
  nvic->icer[index] = bit;
}

static void nvic_clear_pending(int irq) {
  int index = irq >> 5;
  unsigned bit = 1u << (irq & 0x1f);
  nvic->icpr[index] = bit;
}

static void nvic_set_priority(int irq, int priority) {
  int index = irq / 4;
  int bitnum = (irq % 4) * 8;
  unsigned reg = nvic->ipr[index];
  reg &= ~((unsigned) 0xff << bitnum);
  nvic->ipr[index] = reg | (unsigned) priority << bitnum;
}

/*
 * Set priority for system handlers
 * - exc: 4..15, priority: 0..255
 */
static void scb_set_shpr(
  enum armv7m_exception exc,
  int priority
) {
  volatile unsigned *const shpr = &scb->shpr1;
  int index = (exc - 4) / 4;
  int bitnum = (exc % 4) * 8;
  unsigned reg = shpr[index];
  reg &= ~((unsigned) 0xff << bitnum);
  shpr[index] = reg | (unsigned) priority << bitnum;
}

static int isarmv6m(void) {
  return ((scb->cpuid >> 16) & 0xf) == 0xc;
}

/*
 * We use the following encoding for "intnum"
 * intnum  exception  what
 * 0       15         systick
 * 1       16         external interrupt 0
 * i       15+i       external interrupt i-1
 * i+1     16+i       external interrupt i
 */
static int INTNUM_TO_EXTINT(int intnum) {
  return intnum - 1;
}

void port_enable_intnum(int intnum) {
  int extint;
  extint = INTNUM_TO_EXTINT(intnum);
  /* systick is permanently enabled in nvic */
  if (extint < 0) {
    return;
  }
  nvic_set_enable(extint);
}

void port_disable_intnum(int intnum) {
  int extint;
  extint = INTNUM_TO_EXTINT(intnum);
  if (extint < 0) {
    return;
  }
  nvic_clear_enable(intnum);
}

static int get_numinterrupts(void) {
  if (isarmv6m()) {
    /* ictr is reserved on ARMv6-M */
    return 32;
  }
  return 32 * ((*ictr & ICTR_INTLINESNUM) + 1);
}

#define EXC_RESERVED_PRIO 0x00
#define EXC_IRQ_PRIO      0x40
#define EXC_SYSTICK_PRIO  0x80
#define EXC_PENDSV_PRIO   0xFF

static void lsetexc(Lib *lib) {
  int numinterrupts;

  numinterrupts = get_numinterrupts();
  intserver_init(lib, numinterrupts);

  for (int i = 0; i < numinterrupts; i++) {
    nvic_clear_enable(i);
    nvic_clear_pending(i);
    nvic_set_priority(i, EXC_IRQ_PRIO);
  }

  scb->icsr = SCB_ICSR_PENDSVCLR | SCB_ICSR_PENDSTCLR;
  scb_set_shpr(ARMV7M_EXCEPTION_SysTick, EXC_SYSTICK_PRIO);
  scb_set_shpr(ARMV7M_EXCEPTION_PendSV, EXC_PENDSV_PRIO);
  scb_set_shpr(ARMV7M_EXCEPTION_SVCall, EXC_PENDSV_PRIO);
}

/* return: stack pointer of first task. */
void *kcstart(void) {
  Lib *lib;

  board_init0();
  /* These alignment constraints are always enabled on ARMv6-M. */
  scb->ccr = (scb->ccr | SCB_CCR_STKALIGN) | ~SCB_CCR_UNALIGN_TRP;
  /* We happen to know that MemSet() ignores lib. */
  iMemSet(NULL, _bss_begin, 0, _bss_end - _bss_begin);
  lib = &priv.lib;
  AbsExecBase = lib;
  iRawIOInit(lib);
  initlib(lib);
  lib->minstack = 256;
  KASSERT(&_data_begin[0] == &_data_end[0]);

  /* We can now call exec functions but not allocate memory. */
  if (PRINT_CPU) {
    kprint_cpu(lib);
  }
  addmem0(lib, _addmem_bottom, _addmem_top);
  lsetexc(lib);
  collectres(lib, _rom_begin, _rom_end);

  /* PendSV exception not yet activated so no task switch yet. */
  priv.ThisTask = iCreateTask(lib, "init", 0, NULL, 0, NULL, 512);
  KASSERT(priv.ThisTask);

  return &priv.ThisTask->arch[1];
}

/* NOTE: We visit this once when setting up the first task. */
void iReschedule(Lib *lib) {
  scb->icsr = SCB_ICSR_PENDSVSET;
  dsb_and_isb();
}

/*
 * Going for pendsv when voluntary leaving the cpu may waste
 * some interrupt disable time and stack because there is no
 * real need to store the the scratch registers.  However,
 * pendsv is convenient and fast on Cortex-M and makes
 * implementation less complex.
 */
/* This is called synchronously by Wait(), SetTaskPri(), etc. */
void iSwitch(Lib *lib) {
  KASSERT(get_primask() == 0);
  check_canaries(lib, &priv.ThisTask->canaries);
  svc0();
}

static void checkstack(Lib *lib, const Task *const task) {
  char *spreg;
  char *spwashere;

  spreg = (char *) task->arch;
  spwashere = spreg + sizeof (TaskArch);
  if (
    (spreg < (char *) task->splower) ||
    ((char *) task->spupper < spwashere)
  ) {
    lAlert(lib, Alert_StackCheck);
  }
}

/* This is the only place where ThisTask is written. */
static TaskArch *theswitch(Lib *lib, TaskArch *arch) {
  Task *task;
  int removed;

  task = priv.ThisTask;
  dbg("from %s (sp %p)\n", task->node.name, arch);
  KASSERT(get_primask() == 0);
  task->arch = arch;
  checkstack(lib, task);
  check_canaries(lib, &task->canaries);

  removed = 0;
  lObtainIntLock(lib, &lib->tasklock);
  if (task->state == TS_REMOVING) {
    task->state = TS_REMOVED;
    removed = 1;
  }
  lReleaseIntLock(lib, &lib->tasklock);
  if (removed) {
    lSignal(lib, lib->cleantask, SIGF_CLEANUP);
  }

  task = NULL;
  lObtainIntLock(lib, &lib->tasklock);
  do {
    task = (Task *) iGetHead(lib, &lib->taskready);
    if (task == NULL) {
      lReleaseIntLock(lib, &lib->tasklock);
      wfe();
      lObtainIntLock(lib, &lib->tasklock);
    }
    scb->icsr = SCB_ICSR_PENDSVCLR;
  } while (task == NULL);
  lReleaseIntLock(lib, &lib->tasklock);

  priv.ThisTask = task;
  dbg("to %s (sp %p)\n", task->node.name, task->arch);

  return task->arch;
}

TaskArch *theexc(Lib *lib, TaskArch *arch) {
  unsigned exc;

  exc = get_ipsr();
  if (exc == ARMV7M_EXCEPTION_SVCall ||
   exc == ARMV7M_EXCEPTION_PendSV) {
    return theswitch(lib, arch);
  }
  if (PRINT_REGS) {
    kprint_regs(lib, arch, exc);
  }
  check_canaries(lib, &priv.ThisTask->canaries);
  lAlert(lib, Alert_Exception | exc);

  return NULL;
}

void port_prepstack(Lib *lib, Task *task) {
  TaskArch *arch;

  arch = (TaskArch *) ((unsigned) task->spupper & ~7U);
  arch--;
  arch->a1 = (unsigned) lib;
  arch->lr = ~0U;
  arch->pc = (unsigned) task_entry;
  arch->xpsr = 1U<<24;     /* T: Thumb state */
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
    wfe();
  }
}

void task_entry(Lib *lib) {
  KASSERT(priv.ThisTask);
  KASSERT(priv.ThisTask->init);
  priv.ThisTask->init(lib);
  lRemTask(lib);
}

void create_and_start_other_processors(Lib *lib) {
}

/* FIXME: Is this the right thing? */
void iSyncInstructions(Lib *lib, const void *begin, size_t size) {
  dsb_and_isb();
}
