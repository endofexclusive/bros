HOST    = arm-all-bros-

BUILD = build/$(MULTIDIR)
DIST  = dist/$(MULTIDIR)

CFLAGS += $(MULTIFLAGS)
# Prevent GCC from to replace our function with a call to the same function...
CFLAGS += -fno-tree-loop-distribute-patterns

LIBNAME = miss

LIBSRCS  =
LIBSRCS += gcc-ssp.c
LIBSRCS += memcpy.c
LIBSRCS += memset.c

include ../mk/linklib.mk

