/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2019-2022 Martin Åberg */

#include <stdint.h>
#include <priv.h>
#define dbg(...)

/*
 * A limitiation in this implementation is the assumption that
 * the size of MemAlign is a power-of-two. We check that with
 * the below static assert.
 *
 * Consider for example long double with size 12 and align 2.
 */
SASSERT(((sizeof (MemAlign) - 1) & (sizeof (MemAlign))) == 0);

union Header {
  struct {
    union Header *ptr;
    size_t nunits;
  } s;
  MemAlign x;
};
typedef union Header Header;

/* NOTE: assumes align argument is a power-of-two */
static void *alignup(const void *const addr, size_t align) {
  uintptr_t a = (uintptr_t) addr;
  uintptr_t mask = align - 1;
  return (void *) ((a+mask) & ~mask);
}

static void *aligndown(const void *const addr, size_t align) {
  uintptr_t a = (uintptr_t) addr;
  uintptr_t mask = align - 1;
  return (void *) (a & ~mask);
}

static size_t NUNITS(size_t numbytes) {
  if (numbytes == 0) {
    numbytes = 1;
  }
  return ((numbytes + sizeof (Header) - 1) / sizeof (Header));
}

typedef struct {
  MemHeader mh;
  /* 0: zero unit, 1: guard, 2: so Deallocate() works */
  Header mc[3];
} MemHeaderX;

MemHeader *iInitMemHeader(
  Lib *lib,
  void *base,
  size_t numbytes
) {
  size_t minsz;

  /* constrain range so top and bottom are aligned */
  minsz = 2 * (sizeof (MemAlign)-1) + sizeof (MemHeaderX);
  if (numbytes < minsz) {
    return NULL;
  }

  char *const lower   = base;
  char *const upper   = &lower[numbytes];
  char *const bottom  = alignup  (lower, sizeof (MemAlign));
  char *const top     = aligndown(upper, sizeof (MemAlign));

  MemHeaderX *const mhx = (MemHeaderX *) bottom;
  mhx->mh.node.type = NT_MEMHEADER;
  mhx->mh.node.name = NULL;
  mhx->mh.lower = lower;
  mhx->mh.upper = upper;

  /* Skip chunk 1 to avoid chunk 0 being merged */
  mhx->mc[0].s.ptr = &mhx->mc[0];
  mhx->mc[0].s.nunits = 0;
  mhx->mh.first = (MemChunk *) &mhx->mc[0];
  mhx->mh.free = 0;
  lDeallocate(lib, &mhx->mh, &mhx->mc[2],
   top - (char *) &mhx->mc[2]);

  return &mhx->mh;
}

/*
 * This memory allocator is based on an example in K&R. The
 * implementation depends on there always being a zero size
 * block which can never be merged.
 */
void *iAllocate(Lib *lib, MemHeader *mh, size_t numbytes) {
  Header *p;
  Header *prevp;
  Header *freep;
  size_t nunits;

  nunits = NUNITS(numbytes);
  freep = (Header *) mh->first;
  prevp = freep;
  for (p = prevp->s.ptr; ;prevp = p, p = p->s.ptr) {
    if (p->s.nunits >= nunits) {
      if (p->s.nunits == nunits) {
        prevp->s.ptr = p->s.ptr;
      } else {
        p->s.nunits -= nunits;
        p += p->s.nunits;
        p->s.nunits = nunits;
      }
      mh->first = (MemChunk *) prevp;
      mh->free -= nunits * sizeof (*p);
      return p;
    }
    if (p == freep) {
      break;
    }
  }
  return NULL;
}

void iDeallocate(
  Lib *lib,
  MemHeader *mh,
  void *block,
  size_t nbytes
) {
  Header *p;
  Header *bp;
  size_t nunits;

  nunits = NUNITS(nbytes);
  bp = (Header *) block;
  bp->s.nunits = nunits;
  mh->free += bp->s.nunits * sizeof (*p);
  Header *freep = (Header *) mh->first;
  for (p = freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr) {
    if (p >= p->s.ptr && (bp > p || bp < p->s.ptr)) {
      break;
    }
  }
  if (bp + bp->s.nunits == p->s.ptr) {
    dbg("merge with %p\n", (void *) p->s.ptr);
    bp->s.nunits += p->s.ptr->s.nunits;
    bp->s.ptr = p->s.ptr->s.ptr;
  } else {
    dbg("before %p\n", (void *) p->s.ptr);
    bp->s.ptr = p->s.ptr;
  }
  if (p + p->s.nunits == bp) {
    dbg("merge with %p\n", (void *) p);
    p->s.nunits += bp->s.nunits;
    p->s.ptr = bp->s.ptr;
  } else {
    dbg("after %p\n", (void *) p);
    p->s.ptr = bp;
  }
  mh->first = (MemChunk *) p;
}

void iAddMemHeader(
  Lib *lib,
  MemHeader *mh,
  const char *name,
  int attr,
  int pri
) {
  mh->node.pri = pri;
  mh->node.name = (char *) name;
  mh->attr = attr;
  enqnode(lib, &mh->node, &lib->memlist, &lib->memlock);
}

void *iAllocMem(Lib *lib, size_t numbytes, int attr) {
  List *list;
  void *p;

  p = NULL;
  list = &lib->memlist;

  lObtainMutex(lib, &lib->memlock);
  for (Node *node = list->head; node->succ; node = node->succ) {
    MemHeader *mh = (MemHeader *) node;
    if ((mh->attr & attr) == (attr & MEMF_TYPE)) {
      p = lAllocate(lib, mh, numbytes);
      if (p) {
        break;
      }
    }
  }
  lReleaseMutex(lib, &lib->memlock);

  if (p && (attr & MEMF_CLEAR)) {
    lMemSet(lib, p, 0, numbytes);
  }

  return p;
}

void iFreeMem(Lib *lib, void *ptr, size_t numbytes) {
  int deallocated;
  List *list;

  if (ptr == NULL) {
    return;
  }

  list = &lib->memlist;
  deallocated = 0;

  lObtainMutex(lib, &lib->memlock);
  for (Node *node = list->head; node->succ; node = node->succ) {
    MemHeader *mh = (MemHeader *) node;
    if (mh->lower <= ptr && ptr <= mh->upper) {
      lDeallocate(lib, mh, ptr, numbytes);
      deallocated = 1;
      break;
    }
  }
  if (deallocated == 0) {
    lAlert(lib, AT_DeadEnd | AN_ExecLib | AG_MemList);
  }
  lReleaseMutex(lib, &lib->memlock);
}

void *iAllocVec(Lib *lib, size_t numbytes, int attr) {
  MemAlign *p;

  numbytes += sizeof *p;
  p = lAllocMem(lib, numbytes, attr);
  if (p == NULL) {
    return NULL;
  }
  p->the_size_t = numbytes;
  p++;
  return p;
}

void iFreeVec(Lib *lib, void *ptr) {
  MemAlign *p;

  if (ptr == NULL) {
    return;
  }
  p = ptr;
  p--;
  lFreeMem(lib, p, p->the_size_t);
}

size_t iAvailMem(Lib *lib, int attr) {
  List *list;
  size_t sum;

  sum = 0;
  list = &lib->memlist;

  lObtainMutex(lib, &lib->memlock);
  for (Node *node = list->head; node->succ; node = node->succ) {
    MemHeader *mh = (MemHeader *) node;
    if ((mh->attr & attr) == (attr & MEMF_TYPE)) {
      sum += mh->free;
    }
  }
  lReleaseMutex(lib, &lib->memlock);

  return sum;
}

MemList *iAllocEntry(
  Lib *lib,
  const MemRequest *req,
  int numentries
) {
  MemList *ml;
  size_t mlsz;

  mlsz = sizeof (*ml) + numentries * sizeof (MemEntry);
  ml = lAllocMem(lib, mlsz, MEMF_CLEAR | MEMF_ANY);
  if (!ml) {
    return NULL;
  }
  ml->node.type = NT_MEMLIST;
  ml->num = numentries;

  for (int i = 0; i < numentries; i++) {
    size_t rlen;
    void *addr;

    rlen = req->size;
    addr = lAllocMem(lib, rlen, req->attr);
    if (!addr) {
      /* Alloc request not satisfied */
      iFreeEntry(lib, ml);
      return NULL;
    }
    ml->entry[i].ptr = addr;
    ml->entry[i].size = rlen;
    req++;
  }

  return ml;
}

void iFreeEntry(Lib *lib, MemList *ml) {
  size_t mlsz;
  const MemEntry *me;

  if (ml == NULL) {
    return;
  }
  me = &ml->entry[0];
  for (int i = 0; i < ml->num; i++) {
    lFreeMem(lib, me->ptr, me->size);
    me++;
  }
  mlsz = sizeof (*ml) + ml->num * sizeof (MemEntry);
  lFreeMem(lib, ml, mlsz);
}

