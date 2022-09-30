/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

#include <priv.h>
#include <port.h>

static void schedule(Lib *lib);

static ExecCPU *getcpu(Lib *lib, long dispdis) {
  ExecCPU *cpu;
  cpu = port_get_cpu();
  KASSERT(cpu);
  KASSERT(cpu->switch_disable == dispdis);
  KASSERT(cpu->isr_nest == 0);
  return cpu;
}

static void checkstate(Lib *lib, int mask) {
  int state;

  if (!CFG_DIAGNOSTIC) {
    return;
  }
  state = port_ThisTask(lib)->state;
  if (((1<<state) & mask) == 0) {
    kprintf(lib, "state=%d\n", state);
  }
  KASSERT(mask & (1<<state));
}

static void process_removing(Lib *lib, ExecCPU *cpu) {
  struct Task *rem;

  rem = cpu->removing;
  if (rem == NULL) {
    return;
  }
  lObtainIntLock(lib, &lib->tasklock);
  KASSERT(rem->state == TS_REMOVING);
  rem->state = TS_REMOVED;
  lReleaseIntLock(lib, &lib->tasklock);
  cpu->removing = NULL;
  lSignal(lib, lib->cleantask, SIGF_CLEANUP);
}

void task_entry(Lib *lib) {
  ExecCPU *cpu;
  Task *task;
  void (*init)(Lib *lib);
  int level;

  KASSERT(lib);
  /* switch_disable=1 so we can use cpu */
  cpu = getcpu(lib, 1);

  task = port_ThisTask(lib);
  if (task->node.pri == TASK_PRI_IDLE) {
    /*
     * The CPU specific IDLE task is special. It is the first
     * task to run and it does so here before the CPU has been
     * added to cpuonline. That means that the scheduler does
     * not yet know about this CPU and can not assign this CPU
     * another task.
     *
     * That changes now as we add ourselves to cpuonline.
     */
    lObtainIntLock(lib, &lib->tasklock);
    iRemove(lib, &cpu->node);
    iAddTail(lib, &lib->cpuonline, &cpu->node);
    lReleaseIntLock(lib, &lib->tasklock);
    lReschedule(lib);
  }
  process_removing(lib, cpu);
  /*
   * A local interrupt above could set switch_needed while
   * in dispatch_disable. The switch_tasks() handles that case.
   */
  level = port_disable_interrupts();
  KASSERT(port_interrupt_is_enabled(level));
  cpu = switch_tasks_if_needed(lib, cpu);
  cpu->switch_disable = 0;
  port_enable_interrupts(level);

  init = task->init;
  KASSERT(init);
  init(lib);
  lRemTask(lib);
}

static unsigned int get_cpuid(void) {
  return port_get_cpu()->id;
}

static void InitCPU(Lib *lib, ExecCPU *cpu, unsigned int id) {
  char *tname;
  Task *idletask;

  cpu->id = id;
  tname = lAllocMem(lib, 8, MEMF_ANY);
  KASSERT(tname);
  lMemMove(lib, tname, "idlex", 6);
  tname[4] = '0' + id;

  idletask = iCreateTask(lib, tname, TASK_PRI_IDLE, port_idle,
   0, NULL, 0);
  KASSERT(idletask);

  cpu->node.name  = &tname[4];
  cpu->node.pri   = 0;
  cpu->node.type  = NT_CPU;
  cpu->switch_disable = 1;
  cpu->switch_needed = 0;
  cpu->idle = idletask;
  cpu->heir = idletask;
  cpu->thistask = idletask;
  cpu->removing = NULL;
}

static void CreateCPU(Lib *lib, unsigned long id,
 unsigned long creator) {
  ExecCPU *cpu;

  cpu = port_alloc_cpu(lib);
  KASSERT(cpu);
  if (id == creator) {
    port_set_cpu(cpu);
  }
  InitCPU(lib, cpu, id);
  lObtainIntLock(lib, &lib->tasklock);
  iAddTail(lib, &lib->cpuoffline, &cpu->node);
  lReleaseIntLock(lib, &lib->tasklock);
}

void start_on_primary(Lib *lib, unsigned long id) {
  Task *inittask;

  CreateCPU(lib, id, id);
  /*
   * schedule() does not set switch_needed since there is
   * no cpu yet in cpuonline.
   */
  inittask = iCreateTask(lib, "init", 0, func0, 0, NULL, 4096);
  KASSERT(inittask); (void) inittask;

  start_on_secondary(lib, id);
}

ExecCPU *findcpu(Lib *lib, unsigned int id) {
  const List *list;
  ExecCPU *cpu;

  list = &lib->cpuoffline;
  for (Node *node = list->head; node->succ; node = node->succ) {
    cpu = (ExecCPU *) node;
    if (cpu->id == id) {
      return cpu;
    }
  }
  return NULL;
}

/* Must be called by the local CPU */
void start_on_secondary(Lib *lib, unsigned long id) {
  Task none;
  ExecCPU *cpu;

  lObtainIntLock(lib, &lib->tasklock);
  cpu = findcpu(lib, id);
  lReleaseIntLock(lib, &lib->tasklock);
  KASSERT(cpu);
  if (cpu == NULL) {
    lAlert(lib, AT_DeadEnd | AN_ExecLib | AG_NoCPU);
    while (1);
  }

  port_set_cpu(cpu);
  port_enable_ipi(cpu->id);
  port_really_enable_interrupts();
  /*
   * FIXME: isrstack is same as current stack. A potential fix
   * is to enable interrupts in the idle task.
   */
  port_switch_tasks(&none, cpu->idle);
  /* we will continue in task_entry for the IDLE task */
}

/* stack bound checker */
static void checkstack(Lib *lib, const Task *const task) {
  char *spreg;

  spreg = port_getstack(task);
  if (
    (spreg < (char *) task->splower) ||
    ((char *) task->spupper < spreg)
  ) {
    lAlert(lib, Alert_StackCheck);
  }
}

/*
 * Assumptions at entry:
 * - all interrupts are disabled
 * - switch_disable = 1
 * - isr_nest = 0
 *
 * Any ExecCPU pointers to the local cpu held by the caller are
 * invalidated by this function. You can use the return value of
 * this function to get an updated pointer.
 */
ExecCPU *switch_tasks(Lib *lib, ExecCPU *cpu) {
  Task *thistask;

  KASSERT(cpu);
  KASSERT(cpu->switch_disable == 1);
  KASSERT(cpu->isr_nest == 0);

  thistask = cpu->thistask;
  KASSERT(thistask);

  do {
    Task *heir;
    int level;
 
    check_canaries(lib, &thistask->canaries);
    checkstack(lib, thistask);
    cpu->switch_needed = 0;
    heir = cpu->heir;
    KASSERT(heir->state != TS_REMOVED);
    if (heir == thistask) {
      break;
    }
    cpu->thistask = heir;
    /* FIXME: take tasklock when looking at Task.state? */
    if (thistask->state == TS_REMOVING) {
      KASSERT(cpu->removing == NULL);
      cpu->removing = thistask;
    }
    port_really_enable_interrupts();
    cpu = NULL;
    /* run as "thistask" */
    port_switch_tasks(thistask, heir);
    /* run as "heir" = "thistask" = "cpu->thistask" */
    cpu = getcpu(lib, 1);
    KASSERT(cpu->isr_nest == 0);
    process_removing(lib, cpu);
    level = port_disable_interrupts();
    KASSERT(port_interrupt_is_enabled(level));
    (void) level;
  } while (cpu->switch_needed);

  KASSERT(thistask->state != TS_REMOVED);
  KASSERT(cpu->switch_disable == 1);
  KASSERT(cpu->isr_nest == 0);
  if (CFG_DIAGNOSTIC) {
    const int type = port_ThisTask(lib)->node.type;
    KASSERT(type == NT_TASK || type == NT_PROCESS); (void) type;
  }

  return cpu;
}

/* Assumptions at entry: same as switch_tasks() */
ExecCPU *switch_tasks_if_needed(Lib *lib, ExecCPU *cpu) {
  if (cpu->switch_needed) {
    cpu = switch_tasks(lib, cpu);
  }
  return cpu;
}

Task *port_ThisTask(Lib *lib) {
  Task *task;
  ExecCPU *cpu;
  int level;

  level = port_disable_interrupts();
  cpu = port_get_cpu();
  KASSERT(cpu);
  task = cpu->thistask;
  KASSERT(task);
  port_enable_interrupts(level);
  return task;
}

void iReschedule(Lib *lib) {
  int level;
  ExecCPU *cpu;

  /*
   * FIXME: we do not want to call reschedule() to often. For
   * example, do not schedule() multiple times in an ISR or when
   * nesting interrupts. maybe:
   * IF cpu->switch_disable
   *   cpu->schedule_necessary = 1
   * ELSE
   *   cpu->schedule_necessary = 0
   *   schedule()
   * ENDIF
   */
  schedule(lib);

  level = port_disable_interrupts();
  cpu = port_get_cpu();
  if (cpu->switch_disable == 0) {
    if (cpu->switch_needed) {
      KASSERT(port_interrupt_is_enabled(level));
      cpu->switch_disable = 1;
      cpu = switch_tasks(lib, cpu);
      cpu->switch_disable = 0;
    }
  }
  port_enable_interrupts(level);
}

void iSwitch(Lib *lib) {
  ExecCPU *cpu;
  int level;

  schedule(lib);

  level = port_disable_interrupts();
  KASSERT(port_interrupt_is_enabled(level));
  cpu = getcpu(lib, 0);

  if (cpu->switch_needed) {
    checkstate(lib, 1<<TS_READY | 1<<TS_WAIT | 1<<TS_REMOVING);
    cpu->switch_disable = 1;
    cpu = switch_tasks(lib, cpu);
    cpu->switch_disable = 0;
  }
  port_enable_interrupts(level);
  checkstate(lib, 1<<TS_READY | 1<<TS_WAIT);
}

/*
 * There is one idle task per CPU. All idle tasks have
 * TASK_PRI_IDLE in task.node.pri.
 * All idle tasks are in the taskready list.
 * The thistask tasks are in the taskready list.
 *
 * 1.
 * Generate set T of n ready non-idle tasks. n will be
 * [0..num_online].
 * for each cpu
 *   if cpu.heir is in T then
 *     remove heir from T
 *     mark cpu as done
 *   end if
 * end for
 *
 * Each remaining cpu now has either an idle task assigned or a
 * task which shall disappear.
 *
 * 2.
 * for each remaining cpu
 *   select a task from T
 *   set cpu.heir := task
 *   set cpu.switch_needed
 *   mark that cpu as done
 *   remove task from T
 * end for
 *
 * 3.
 * for each remaining cpu
 *   if cpu.heir == cpu.idle
 *     do nothing
 *   else
 *     set cpu.heir := cpu.idle
 *     set cpu.switch_needed
 *   end if
 * end for
 *
 * It is important that a task is heir on at most one CPU.  That
 * property is independent of any locks.
 */

static int isinlist(List *list, Node *target) {
  for (Node *node = list->head; node->succ; node = node->succ) {
    if (node == target) {
      return 1;
    }
  }
  return 0;
}

static void setheir(Lib *lib, ExecCPU *cpu, Task *heir) {
  cpu->heir = heir;
  /* TODO: barrier for heir? */
  if (cpu == port_get_cpu()) {
    cpu->switch_needed = 1;
  } else {
    port_send_ipi(cpu->id);
  }
}

static ExecCPU *firstcpu(Lib *lib) {
  return (ExecCPU *) lib->cpuonline.head;
}

static ExecCPU *nextcpu(ExecCPU *cpu) {
  return (ExecCPU *) cpu->node.succ;
}

static void schedule(Lib *lib) {
  List tlist;
  unsigned int allmask;
  unsigned int assmask;
  ExecCPU *cpu;

  allmask = 0;
  assmask = 0; /* bit set means "done" */
  iNewList(lib, &tlist);
  lObtainIntLock(lib, &lib->tasklock);

  /* move [0..num_online] non-idle tasks to tlist */
  for (cpu = firstcpu(lib); nextcpu(cpu); cpu = nextcpu(cpu)) {
    Task *nh;

    allmask |= 1U << cpu->id;
    nh = (Task *) iGetHead(lib, &lib->taskready);
    /* at least the idle tasks should be there */
    KASSERT(nh);
    if (nh->node.pri == TASK_PRI_IDLE) {
      continue;
    }
    iRemove(lib, &nh->node);
    iAddTail(lib, &tlist, &nh->node);
  }

  /* tasks which are already assigned a CPU will remain so */
  /* Complexity: O(n*n) where n is number of processors */
  for (cpu = firstcpu(lib); nextcpu(cpu); cpu = nextcpu(cpu)) {
    Task *heir;

    heir = cpu->heir;
    if (isinlist(&tlist, &heir->node)) {
      /* nothing changed */
      iRemove(lib, &heir->node);
      iEnqueue(lib, &lib->taskready, &heir->node);
      assmask |= 1U << cpu->id;
      continue;
    }
  }

  /* assign a CPU to each remaining task in tlist */
  /* Complexity: O(n*n) where n is number of processors */
  for (cpu = firstcpu(lib); nextcpu(cpu); cpu = nextcpu(cpu)) {
    Task *heir;

    if (assmask & (1U << cpu->id)) {
      continue;
    }
    heir = (Task *) iRemHead(lib, &tlist);
    if (heir == NULL) {
      break;
    }
    setheir(lib, cpu, heir);
    iEnqueue(lib, &lib->taskready, &heir->node);
    assmask |= 1U << cpu->id;
  }
  KASSERT(lGetHead(lib, &tlist) == NULL);

  /* remaining processors get their dedicated idle task */
  for (cpu = firstcpu(lib); nextcpu(cpu); cpu = nextcpu(cpu)) {
    if (assmask & (1U << cpu->id)) {
      continue;
    }
    if (cpu->heir == cpu->idle) {
      /* nothing changed */
      assmask |= 1U << cpu->id;
      continue;
    }
    setheir(lib, cpu, cpu->idle);
    assmask |= 1U << cpu->id;
  }
  KASSERT(assmask == allmask);

  lReleaseIntLock(lib, &lib->tasklock);
}

void announce_ipi(void) {
  ExecCPU *cpu = port_get_cpu();
  cpu->switch_needed = 1;
}

void create_and_start_other_processors(Lib *lib) {
  unsigned int myid;
  int ncpu;

  myid = get_cpuid();
  ncpu = port_get_ncpu(lib);
  /* FIXME: support ranges other than 0..ncpu-1 */
  for (unsigned int id = 0; id < (unsigned int) ncpu; id++) {
    if (id == myid) {
      continue;
    }
    CreateCPU(lib, id, myid);
  }
  if (1 < ncpu) {
    port_start_other_processors(lib);
  }
}

void setexc(Lib *lib, unsigned long bootid) {
  int numinterrupts;

  port_init_interrupt(lib, bootid);
  numinterrupts = port_get_numinterrupts();
  intserver_init(lib, numinterrupts);
}

