/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2019-2022 Martin Åberg */

int port_disable_interrupts(void);
void port_enable_interrupts(int level);
void port_halt(void);

/*
 * Number of interrupt sources (intnum) port understands,
 * ordered 0 to N-1. Used to initialize ExecBase.intserver.
 * port_init_interrupt() is called before to this function.
 */
int port_get_numinterrupts(void);
void port_enable_intnum(int intnum);
void port_disable_intnum(int intnum);

Task *port_ThisTask(Lib *lib);
/* Do port specific task initializations */
void port_prepstack(Lib *lib, Task *task);


/* Functions needed for SMP operation (mostly mp.c) */

/* called by the primary processor to start others available */
void port_start_other_processors(Lib *lib);
ExecCPU *port_get_cpu(void);
void port_set_cpu(ExecCPU *cpu);
/* Upper limit on CPU:s to use. */
int port_get_ncpu(Lib *lib);
/* ExecCPU structure is port specific, so let it allocate it. */
ExecCPU *port_alloc_cpu(Lib *lib);
/* NOTE: the idle task should never Wait() */
void port_idle(Lib *lib);

void port_switch_tasks(Task *thistask, Task *heir);
void *port_getstack(const Task *const task);
/* Prepare the interrupt hardware for our use. */
void port_init_interrupt(Lib *lib, unsigned long bootid);
/* Prepare CPU to receive inter-processor interrupts. */
void port_enable_ipi(unsigned int target_cpu);
/* Send inter-processor interrupt to a remote processor. */
void port_send_ipi(unsigned int target_cpu);
int port_interrupt_is_enabled(int level);
void port_really_enable_interrupts(void);
void default_trapcode(Lib *, void *data, void *info);

