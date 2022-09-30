# SPDX-License-Identifier: GPL-2.0
# Copyright 2020-2022 Martin Åberg

# This make fragment is used to build linker libraries.

BUILD ?= build/
DIST ?= dist/
ifneq ($(strip $(BUILD)),)
  ifneq ($(strip $(BUILD)),$(dir $(strip $(BUILD))))
    override BUILD:=$(BUILD)/
  endif
endif

CC      = $(HOST)gcc
AR      = $(HOST)ar
OBJDUMP = $(HOST)objdump

ECHO    = @echo
CP      = cp
MKDIR   = mkdir -p

CFLAGS          += -g
CFLAGS          += -Os
CFLAGS          += -fno-strict-aliasing
CFLAGS          += -fstack-usage
# CFLAGS          += -ffreestanding
CFLAGS          += -std=c99
CFLAGS          += -pedantic
CFLAGS          += -Wall
CFLAGS          += -Wextra
CFLAGS          += -fanalyzer
CFLAGS          += -Wno-unused-parameter
CFLAGS          += -Wmissing-prototypes
CFLAGS          += -Wstrict-prototypes
CFLAGS          += -Wstack-usage=256
CFLAGS          += -Wno-implicit-fallthrough
CFLAGS          += -Wundef

CPPFLAGS        += -MD
ASFLAGS         += $(CPUFLAGS)
ASFLAGS         += -g

LIBFILE = $(BUILD)/lib$(LIBNAME).a
LIBDISS := $(addsuffix .dis,$(LIBFILE))
OBJDIR=$(BUILD)/obj

LIBOBJS := $(LIBSRCS:.c=.c.o)
LIBOBJS := $(LIBOBJS:.S=.S.o)
LIBOBJS := $(addprefix $(OBJDIR)/,$(LIBOBJS))

OBJOBJS := $(OBJSRC:.c=.c.o)
OBJOBJS := $(addprefix $(OBJDIR)/,$(OBJOBJS))
OBJDISS := $(addsuffix .dis,$(OBJOBJS))

DOCDIR=$(BUILD)/doc

MANOBJS := $(MANSRCS:.3=)
MANOBJS := $(addsuffix .txt,$(MANOBJS))
MANOBJS := $(addprefix $(DOCDIR)/,$(MANOBJS))

.PHONY: all
all: $(LIBFILE)
all: $(LIBDISS)
all: $(OBJOBJS)
all: $(OBJDISS)
doc: $(MANOBJS)

$(OBJDIR)/%.c.o: %.c
	$(ECHO) " $@"
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $(abspath $<) -o $(abspath $@)

$(OBJDIR)/%.S.o: %.S
	$(ECHO) " $@"
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(ASFLAGS) -c $(abspath $<) -o $(abspath $@)

%.a:
	$(ECHO) " $@"
	mkdir -p $(dir $@)
	$(AR) rc $(abspath $@) $(abspath $^)

%.dis: %
	$(ECHO) " $@"
	mkdir -p $(dir $@)
	$(OBJDUMP) -d -r $(DISFLAGS) $(abspath $<) > $(abspath $@)

$(DOCDIR)/%.txt: %.3
	$(MKDIR) $(dir $@)
	$(ECHO) " $(notdir $@)"
	man $< | col -bx > $@

$(DOCDIR)/%.txt: %
	$(MKDIR) $(dir $@)
	$(ECHO) " $(notdir $@)"
	man $< | col -bx > $@

$(LIBFILE): $(LIBOBJS)

.PHONY: install
install: $(LIBFILE) $(OBJOBJS)
	$(MKDIR) $(DIST)
	$(CP) $(abspath $(LIBFILE) $(OBJOBJS)) $(abspath $(DIST))

.PHONY: clean
clean:
	$(RM) $(LIBFILE)
	$(RM) $(LIBDISS)
	$(RM) $(LIBOBJS)
	$(RM) $(LIBOBJS:.o=.d)
	$(RM) $(LIBOBJS:.o=.su)
	$(RM) $(OBJOBJS)
	$(RM) $(OBJOBJS:.o=.d)
	$(RM) $(OBJOBJS:.o=.su)
	$(RM) $(OBJDISS)
	$(RM) $(MANOBJS)
	$(RM) $(CLEAN_EXTRA)
	rmdir -p $(dir $(LIBOBJS)) 2> /dev/null || true
	rmdir -p $(dir $(MANOBJS)) 2> /dev/null || true

-include $(LIBOBJS:.o=.d)
-include $(OBJOBJS:.o=.d)

