/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

struct TaskArch {
  /* software managed */
  unsigned v1, v2, v3, v4, v5, v6, v7, v8; /* r4 - r11 */
  /* hardware stacking */
  unsigned a1, a2, a3, a4;    /* r0 - r3 */
  unsigned ip, lr, pc, xpsr;  /* r12, r14, r15, xpsr */
  /* HW adds one 32-bit word if SP was not 8-byte aligned at stacking */
};

void *kcstart(void);
void kprint_cpu(Lib *lib);
void kprint_regs(Lib *lib, TaskArch *arch, unsigned ipsr);
TaskArch *theexc(Lib *lib, TaskArch *arch);

void dsb_and_isb(void);
void svc0(void);
void wfe(void);
unsigned get_ipsr(void);
unsigned get_primask(void);

#define LINKER_SYMBOL(sym) extern char sym [];

LINKER_SYMBOL(_bss_begin)
LINKER_SYMBOL(_bss_end)
LINKER_SYMBOL(_data_begin)
LINKER_SYMBOL(_data_end)
LINKER_SYMBOL(_rom_begin)
LINKER_SYMBOL(_rom_end)
LINKER_SYMBOL(_addmem_bottom)
LINKER_SYMBOL(_addmem_top)

