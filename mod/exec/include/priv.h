/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2019-2022 Martin Åberg */

#include <exec/exec.h>
#include <exec/libcall.h>
#include <exec/op.h>
#include "../internal.h"

/* 0: disable asserts, 1: enable asserts */
#ifndef CFG_DIAGNOSTIC
# define CFG_DIAGNOSTIC 1
#endif

#if CFG_DIAGNOSTIC == 0
# define KASSERT(e) ((void)0)
#else
# define KASSERT(e) \
  if (!(e)) { \
    kassert_hit(lib, #e, __FILE__, __LINE__, __func__); \
  }
#endif

#define Alert_Exception   (AT_DeadEnd|AN_ExecLib|AG_Exception)
#define Alert_StackCheck  (AT_DeadEnd|AN_ExecLib|AG_StackCheck)
#define Alert_Canaries    (AT_DeadEnd|AN_ExecLib|AG_Canaries)

#define NELEM(v) ((sizeof (v)) / (sizeof (v[0])))

typedef struct Device           Device;
typedef struct DeviceOp         DeviceOp;
typedef struct ExecBase         Lib;
typedef struct ExecBaseOp       LibOp;
typedef struct IORequest        IORequest;
typedef struct IntLock          IntLock;
typedef struct Interrupt        Interrupt;
typedef struct Library          Library;
typedef struct LibraryOp        LibraryOp;
typedef struct List             List;
typedef struct MemChunk         MemChunk;
typedef struct MemEntry         MemEntry;
typedef struct MemHeader        MemHeader;
typedef struct MemList          MemList;
typedef struct MemRequest       MemRequest;
typedef struct Message          Message;
typedef struct MsgPort          MsgPort;
typedef struct Mutex            Mutex;
typedef struct Node             Node;
typedef struct ExecCPU          ExecCPU;
typedef struct Resident         Resident;
typedef struct ResidentInfo     ResidentInfo;
typedef struct ResidentAuto     ResidentAuto;
typedef struct ResidentNode     ResidentNode;
typedef struct Segment          Segment;
typedef struct StackCanaries    StackCanaries;
typedef struct Task             Task;
typedef struct TaskArch         TaskArch;

/* Enqueue(list, node) under Mutex lock */
void enqnode(Lib *lib, Node *node, List *list, Mutex *lock);
/* Call l->Expunge() and maybe free memory */
Segment *expungelib(Lib *lib, Library *l);
/* for RemLibrary() and RemDevice() */
void remlib(Lib *lib, Library *l, Mutex *lock);

/* Installed when tasking is possible */
void iObtainMut(Lib *lib, Mutex *ctx);
void iReleaseMut(Lib *lib, Mutex *ctx);

/* spin lock implementation */
void iObtainIntLockDisabled(Lib *lib, IntLock *const lock);
void iReleaseIntLockDisabled(Lib *lib, IntLock *const lock);

/* useful for port startup code */
void initlib(Lib *lib);

/* initialize and add a memory list after probing */
size_t probeaddmem(
  Lib *lib,
  char *bottom,
  char *top,
  size_t chunk,
  const char *name,
  int attr,
  int priority
);

/* initialize and add a memory list with known bounds */
void addmem0(Lib *lib, void *bottom, void *top);
void setexc(Lib *lib, unsigned long bootid);
void collectres(Lib *lib, const char *begin, const char *end);
void func0(Lib *lib);
void create_and_start_other_processors(Lib *lib);
void start_on_primary(Lib *lib, unsigned long id);
void start_on_secondary(Lib *lib, unsigned long id);
void runintservers(Lib *lib, int intnum);
void intserver_init(Lib *lib, int num);
ExecCPU *switch_tasks(Lib *lib, ExecCPU *cpu);
ExecCPU *switch_tasks_if_needed(Lib *lib, ExecCPU *cpu);
/* shall be called by local CPU when receiving IPI */
void announce_ipi(void);
void task_entry(Lib *lib);
ExecCPU *findcpu(Lib *lib, unsigned int id);
void check_canaries(Lib *lib, StackCanaries *can);

void kprintf(Lib *lib, const char *fmt, ...);
void kassert_hit(
  Lib *lib,
  const char *e,
  const char *file,
  int line,
  const char *func
);

/*
 * Static compile-time assert
 *
 * Desired properties
 * 1. No warnings when condition is true.
 * 2. Compilation error if condition is false.
 * 3. Compilation error if condition is not known at compile-time.
 * 4. Can be used inside and outside function scope.
 */

#define SASSERT3(cond, desc) enum { SASSERT_##desc = 1/((cond)?1:0) }
#define SASSERT2(cond, desc) SASSERT3(cond, _##desc)
#define SASSERT1(cond, desc) SASSERT2(cond, desc)
#define SASSERT(cond)        SASSERT1(cond, __LINE__)

#define ALIGNOF(type) offsetof(struct { char c; type memb; }, memb)

