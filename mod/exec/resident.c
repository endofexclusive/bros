/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

#include <priv.h>

#if 1
#define info(...) kprintf(lib, __VA_ARGS__)
#else
#define info(...)
#endif

Resident *iFindResident(Lib *lib, const char *name) {
  ResidentNode *res;
  res = (ResidentNode *) lFindName(lib, &lib->reslist, name);
  if (res == NULL) {
    return NULL;
  }
  return (Resident *) res->res;
}

void *iInitResident(Lib *lib, const Resident *res, Segment *segment) {
  Library *l;

  info("%s: \"%s\"\n", __func__, res->info.name);

  if ((res->flags & RTF_AUTOINIT) == 0) {
    int ret;
    void *posdata;

    posdata = NULL;
    if (res->init.direct.possize) {
      posdata = lAllocMem(
        lib,
        res->init.direct.possize,
        MEMF_CLEAR | MEMF_ANY
      );
      if (posdata == NULL) {
        return NULL;
      }
    }
    ret = res->init.direct.f(lib, posdata, segment);
    if (ret) {
      return NULL;
    }
    return lib;
  }

  l = lMakeLibrary(lib, &res->info, &res->init.iauto, segment);
  if (l == NULL) {
    return NULL;
  }
  switch (l->node.type) {
    case NT_LIBRARY:
      lAddLibrary(lib, l);
      break;
    case NT_DEVICE:
      lAddDevice(lib, (Device *) l);
      break;
  }

  return l;
}

void iInitCode(Lib *lib, int level) {
  List *list;

  info("%s: level %d\n", __func__, level);
  list = &lib->reslist;
  for (Node *node = list->head; node->succ; node = node->succ) {
    const Resident *res;
    ResidentNode *rn;

    rn = (ResidentNode *) node;
    res = rn->res;
    if ((res->flags & RTF_LEVEL) == level) {
      lInitResident(lib, res, NULL);
    }
  }
}

/* Find any Resident between p and end and put on ExecBase list. */
void collectres(Lib *lib, const char *p, const char *end) {
  List *list;

  list = &lib->reslist;
  while (p < end) {
    const Resident *res;
    ResidentNode *rn;

    res = (const Resident *) p;
    if (res->matchword != RTC_MATCHWORD || res->matchtag != res) {
      p += ALIGNOF(Resident);
      continue;
    }
    info("- %-32s (%s)\n", res->info.name, res->info.idstring);

    rn = (ResidentNode *) iFindName(lib, list, res->info.name);
    if (rn) {
      if (res->info.version < rn->res->info.version) {
        p = res->endskip;
        continue;
      }
      iRemove(lib, &rn->node);
    } else {
      rn = iAllocMem(lib, sizeof *rn, MEMF_ANY);
    }
    rn->node.name = res->info.name;
    rn->node.pri = res->pri;
    rn->res = (Resident *) res;
    iEnqueue(lib, list, &rn->node);
    p = res->endskip;
  }
}

