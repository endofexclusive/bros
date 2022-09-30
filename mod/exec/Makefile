# SPDX-License-Identifier: GPL-2.0
# Copyright 2021-2022 Martin Åberg

MOD     := exec.library
INCDIR  :=
SRCS     =

-include $(CONFIG)
-include $(EXEC_CHIPDIR)/mk

SRCS    += createtask.c
SRCS    += dev.c
SRCS    += func0.c
SRCS    += kassert.c
SRCS    += lib.c
SRCS    += lists.c
SRCS    += mem.c
SRCS    += memmove.c
SRCS    += msg.c
SRCS    += mutex.c
SRCS    += rawdofmt.c
SRCS    += resident.c
SRCS    += start.c
SRCS    += task.c
SRCS    += optemplate.c

INCDIR  += include

include ../dir.mk
include $(MK)/mod.mk

$(SRCS): internal.h

