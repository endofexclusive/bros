/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022 Martin Åberg */

/*
 *  type      LP64   ILP32
 * ------------------------
 *  char         8       8
 *  short       16      16
 *  int         32      32
 *  long        64      32
 *  long long   64      64
 *  pointer     64      32
 * ------------------------
 *
 * a0-a7:       arguments
 * t0-t6:       temporary
 * s0-s11:      saved
 * tp:          thread pointer (not used)
 * gp:          not for exec. saved/restored with task
 * sp:          stack pointer
 * ra:          return address
 * zero:        zero
 */

/* Stacked on exception (interrupt) entry. */
struct excframe {
  unsigned long s[2];
  unsigned long a[8];
  unsigned long t[7];
  unsigned long ra;
  unsigned long sstatus;
  unsigned long sepc;
};

struct fullframe {
  unsigned long s2[10]; /* s2-s11 */
  unsigned long gp;
  unsigned long tp;
  unsigned long sp;
  unsigned long stval;
  unsigned long scause;
  unsigned long pad0;
  struct excframe excframe;
};

/* Saved and restored by port_switch_tasks(). */
struct TaskArch {
  unsigned long s[12];
  unsigned long gp;
  unsigned long ra;
};

extern char entry_for_secondary;
void kcstart(unsigned long id);
void kprint_cpu(Lib *lib);
void theexc(Lib *lib, struct fullframe *ff);
void riscv_fence_rw(void);
void riscv_fence_i(void);

#if __riscv_xlen == 32
 #define PR_REG "%08x"
 #define NO_REG "        "
#elif __riscv_xlen == 64
 #define PR_REG "%016lx"
 #define NO_REG "                "
#endif

#define LINKER_SYMBOL(sym) extern char sym [];

LINKER_SYMBOL(_rom_begin)
LINKER_SYMBOL(_rom_end)
LINKER_SYMBOL(_addmem_bottom)
LINKER_SYMBOL(_addmem_top)

