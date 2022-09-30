/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022 Martin Åberg */

/*
 *  type      bits
 * -----------------
 *  char         8
 *  short       16
 *  int         32
 *  long        32
 *  long long   64
 *  pointer     32
 * -----------------
 *
 * g5, g6, g7 are "reserved for system" in the SPARC ABI and are
 * never used by compiler. Later ABI (V9) use g7 for TLS.
 *
 * g5: nwinmin1
 * g6: PortCPU
 */

/* Stacked on interrupt entry. */
struct intframe {
  unsigned int unused0;
  unsigned int g1;
  unsigned int g2;
  unsigned int g3;
  unsigned int g4;
  unsigned int y;
};

/* Additional window registers are on the stack. */
/* Stacked on unhandled trap entry. */
struct fullframe {
  unsigned int g[8];
  unsigned int o[8];
  unsigned int psr;
  unsigned int pc;
  unsigned int npc;
  unsigned int wim;
  unsigned int tbr;
  unsigned int y;
};

/* Saved and restored by port_switch_tasks(). */
struct TaskArch {
  unsigned int l0, l1, l2, l3, l4, l5, l6, l7;
  unsigned int i0, i1, i2, i3, i4, i5, i6, i7;
  unsigned int psr; /* only PS and EF used */
  unsigned int o7;  /* return pc */
};

void kcstart(void *ramtop, unsigned long id);
void kprint_cpu(Lib *lib);
void theexc(Lib *lib, struct fullframe *ff);

/* Only useful for IMPL and VER */
unsigned int get_psr(void);

#define LINKER_SYMBOL(sym) extern char sym [];

LINKER_SYMBOL(_rom_begin)
LINKER_SYMBOL(_rom_end)
LINKER_SYMBOL(_addmem_bottom)
LINKER_SYMBOL(_addmem_top)

