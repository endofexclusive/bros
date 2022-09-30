# SPDX-License-Identifier: GPL-2.0
# Copyright 2020-2022 Martin Åberg

# This make fragment is used to build self-standing applications
# which are scatter-loaded and relocated at load-time.

all:
HOST            ?= sparc-all-bros-
# HOST            ?= arm-all-bros-
# CPUFLAGS        ?= -mcpu=cortex-m0

MOD := $(PROG).elf

include $(MK)/tool.mk
include $(MK)/gcc.mk

# "-L" etc
LDFLAGS += $(CPUFLAGS)
LDFLAGS += -Wl,-Map=$(BUILD)/$(MOD).map

# "Library flags or names"
LDLIBS  =

all: $(BUILD)/$(PROG)
all: $(BUILD)/$(MOD)
all: $(BUILD)/$(MOD).dis
all: $(BUILD)/$(MOD).lst
all: $(BUILD)/$(MOD).undef
all: $(BUILD)/$(MOD).readelf
all: $(BUILD)/$(PROG).dis
all: $(BUILD)/$(PROG).lst
all: $(BUILD)/$(PROG).undef
all: $(BUILD)/$(PROG).readelf

size: $(BUILD)/$(MOD)
undef: $(BUILD)/$(MOD).undef
lsundef: $(BUILD)/$(MOD).undef

OBJS := $(addsuffix .o,$(SRCS))
OBJS := $(addprefix $(OBJDIR)/,$(OBJS))
OBJ0 := $(firstword $(OBJS))
DEPS := $(patsubst %.o,%.d,$(OBJS))

MODARCHIVE  := $(BUILD)/$(PROG).a

$(MODARCHIVE): $(OBJS)

$(BUILD)/$(PROG): $(BUILD)/$(MOD)
	$(ECHO) " $@"
	mkdir -p $(dir $@)
	$(STRIP) --strip-debug --strip-unneeded $^ -o $@

$(BUILD)/$(MOD): $(MODARCHIVE)
	$(ECHO) " $@"
	mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) $(OBJ0) $(MODARCHIVE) $(LDLIBS) -o $@

.PHONY: install
install: $(BUILD)/$(PROG)
	mkdir -p $(DIST)
	$(INSTALL_DATA) $(BUILD)/$(PROG) $(DIST)/

.PHONY: clean
clean:
	rm -f $(BUILD)/$(MOD)
	rm -f $(BUILD)/$(PROG)
	rm -f $(MODARCHIVE)
	rm -f $(addprefix $(BUILD)/$(MOD),.dis .lst .map)
	rm -f $(addprefix $(BUILD)/$(MOD),.undef .readelf)
	rm -f $(OBJS)
	rm -f $(DEPS)
	rmdir -p $(dir $(OBJS)) 2> /dev/null || true

-include $(DEPS)

