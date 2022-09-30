# SPDX-License-Identifier: GPL-2.0
# Copyright 2020-2022 Martin Åberg

# This make fragment sets definitions for the build-system

BUILD           ?= build
DIST            ?= dist/
OBJDIR           = $(BUILD)/obj
GENDIR           = $(BUILD)/gen

MAKEFLAGS       += --no-builtin-rules
MAKEFLAGS       += --no-builtin-variables

.SUFFIXES:

ECHO            := @echo
GENHEADER       := $(ROOTDIR)/scripts/genheader.tcl

INSTALL_PROGRAM  = install
INSTALL_DATA     = install -m 644

