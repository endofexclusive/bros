/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

#ifndef EXEC_MEMORY_H
#define EXEC_MEMORY_H

#include <stddef.h>
#include <exec/lists.h>

#define MEMF_ANY        0x0000
/* can be used for DMA */
#define MEMF_DMA        0x0001
/* can contain CPU instructions */
#define MEMF_EXEC       0x0002
#define MEMF_CLEAR      0x4000
#define MEMF_TYPE       0x00FF
#define MEMF_HOW        (~MEMF_TYPE)

/* All allocated memory is aligned to MemAlign. */
typedef union {
  long long      the_longlong;
  double         the_double;
  size_t         the_size_t;
  void          *the_pvoidp;
  void         (*the_funcp)(void);
} MemAlign;

/*
 * MemChunk is the memory allocator unit size.
 * - Any user allocated memory comes in multiples of MemChunk.
 * - MemChunk has MemAlign alignment.
 * - MemChunk may be larger than MemAlign but not smaller.
 */
struct MemChunk;

struct MemHeader {
  struct Node      node;
  short            attr;
  struct MemChunk *first;
  void            *lower;
  void            *upper; /* upper memory bound+1 */
  size_t           free;  /* total number of free bytes */
};

/* Input to AllocEntry() */
struct MemRequest {
  short    attr;
  size_t   size;
};

struct MemEntry {
  void    *ptr;
  size_t   size;
};

/* Output from AllocEntry(), input to FreeEntry() */
struct MemList {
  struct Node      node;
  int              num;
  struct MemEntry  entry[];
};

#endif

