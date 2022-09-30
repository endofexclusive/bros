# SPDX-License-Identifier: GPL-2.0
# Copyright 2020-2022 Martin Åberg

# This make fragment is used to build modules which can either
# be put in ROM or scatter-loaded and relocated at load-time.

all:
HOST            ?= sparc-all-bros-
CPUFLAGS        ?=

include $(MK)/tool.mk
include $(MK)/gcc.mk

# "-L" etc
LDFLAGS += $(CPUFLAGS)
LDFLAGS += -nostdlib
LDFLAGS += -nostdinc
LDFLAGS += --entry _start
LDFLAGS += -Wl,-Map=$(abspath $(BUILD)/$(MOD).map)

# "Library flags or names"
LDLIBS  += -lbros
LDLIBS  += -lmiss
LDLIBS  += -lgcc

MODNAME         ?= $(basename $(MOD))

all: $(BUILD)/$(MOD)
all: $(BUILD)/$(MOD).rel
all: $(BUILD)/$(MOD).dis
all: $(BUILD)/$(MOD).lst
all: $(BUILD)/$(MOD).undef

size: $(BUILD)/$(MOD)
undef: $(BUILD)/$(MOD).undef
lsundef: $(BUILD)/$(MOD).undef
stat: lsundef
stat: size
stat: stack

OBJS := $(addsuffix .o,$(SRCS))
OBJS := $(addprefix $(OBJDIR)/,$(OBJS))
OBJ0 := $(firstword $(OBJS))
DEPS := $(patsubst %.o,%.d,$(OBJS))
SUS  := $(patsubst %.o,%.su,$(filter-out %.S.o,$(OBJS)))

stack: $(SUS)
$(SUS): $(OBJS)

MODARCHIVE  := $(BUILD)/$(MOD).a

$(MODARCHIVE): $(OBJS)

$(BUILD)/$(MOD): $(MODARCHIVE)
	$(ECHO) " $@"
	mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) $(abspath $(OBJ0) $(MODARCHIVE)) $(LDLIBS) \
	 -o $(abspath $@)

$(BUILD)/$(MOD).rel: $(BUILD)/$(MOD)
	$(ECHO) " $@"
	$(OBJCOPY) \
	 --prefix-symbol=$(basename $(basename $(notdir $@)))_ \
	 $(abspath $<) $(abspath $@)

.PHONY: install
install: $(BUILD)/$(MOD)
	mkdir -p $(DIST)
	$(INSTALL_DATA) $(abspath $(BUILD)/$(MOD)) $(abspath $(DIST))/

.PHONY: install-strip
install-strip: $(BUILD)/$(MOD)
	mkdir -p $(DIST)
	$(INSTALL_DATA) -s $(abspath $(BUILD)/$(MOD)) $(abspath $(DIST))/

.PHONY: clean
clean:
	rm -f $(BUILD)/$(MOD)
	rm -f $(addprefix $(BUILD)/$(MOD),.a .dis .lst .map .rel)
	rm -f $(addprefix $(BUILD)/$(MOD),.undef .readelf)
	rm -f $(OBJS) $(GENS)
	rm -f $(DEPS)
	rm -f $(SUS)
	rmdir -p $(dir $(GENS)) 2> /dev/null || true
	rmdir -p $(dir $(OBJS)) 2> /dev/null || true

FUNCDEF ?= func.tcl

optemplate.c: $(FUNCDEF) $(GENHEADER)
	$(ECHO) " $@"
	mkdir -p $(dir $@)
	$(GENHEADER) optemplate < $(abspath $<) > $(abspath $@)

internal.h: $(FUNCDEF) $(GENHEADER)
	$(ECHO) " $@"
	mkdir -p $(dir $@)
	$(GENHEADER) internal < $(abspath $<) > $(abspath $@)

.PHONY: func
func: optemplate.c
func: internal.h

.PHONY: clean-func
clean-func:
	echo rm -f pub/$(MODNAME)/op.h
	rm -f internal.h
	rm -f optemplate.c

-include $(DEPS)

