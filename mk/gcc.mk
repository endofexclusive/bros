# SPDX-License-Identifier: GPL-2.0
# Copyright 2020-2022 Martin Åberg

# assignments and rules for GCC+binutils

CC              := $(HOST)gcc
AR              := $(HOST)ar
NM              := $(HOST)nm
OBJCOPY         := $(HOST)objcopy
OBJDUMP         := $(HOST)objdump
READELF         := $(HOST)readelf
SIZE            := $(HOST)size
STRIP           := $(HOST)strip

CFLAGS          += $(CPUFLAGS)
CFLAGS          += -g
CFLAGS          += -Os
CFLAGS          += -fno-strict-aliasing
CFLAGS          += -fstack-usage
CFLAGS          += -ffreestanding
CFLAGS          += -std=c99
CFLAGS          += -pedantic
CFLAGS          += -Wall
CFLAGS          += -Wextra
# CFLAGS          += -Wconversion
CFLAGS          += -fanalyzer
CFLAGS          += -Wno-unused-parameter
CFLAGS          += -Wmissing-prototypes
CFLAGS          += -Wstrict-prototypes
CFLAGS          += -Wstack-usage=256
CFLAGS          += -Wno-implicit-fallthrough
CFLAGS          += -Wundef

CPPFLAGS        += -MD
CPPFLAGS        += $(INCFLAGS)

INCFLAGS         = $(addprefix -I,$(abspath $(INCDIR)))

ASFLAGS         += $(CPUFLAGS)
ASFLAGS         += -g
ASFLAGS         += $(addprefix -Xassembler ,$(INCFLAGS))

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

%.lst: %
	$(ECHO) " $@"
	mkdir -p $(dir $@)
	$(OBJDUMP) -d -r -S $(DISFLAGS) $(abspath $<) > $(abspath $@)

%.hex: %
	$(ECHO) " $@"
	mkdir -p $(dir $@)
	$(OBJCOPY) -O ihex $(abspath $<) $(abspath $@)

%.srec: %
	$(ECHO) " $@"
	mkdir -p $(dir $@)
	$(OBJCOPY) -O srec $(abspath $<) $(abspath $@)

%.readelf: %
	$(ECHO) " $@"
	mkdir -p $(dir $@)
	$(READELF) -a $(abspath $^) > $(abspath $@)

# Generate a file listing undefined symbols
%.undef: %
	$(ECHO) " $@"
	mkdir -p $(dir $@)
	$(NM) --undefined-only $(abspath $<) > $(abspath $@)

# Generate error if there are undefined symbols
.PHONY: undef
undef:
	cat $(abspath $^) | wc -m | grep 0 > /dev/null

.PHONY: lsundef
lsundef:
	cat /dev/null $(abspath $^) | sort

.PHONY: size
size:
	$(SIZE) $^

# Stack statistics from GCC .su files
.PHONY: stack
stack:
	sort -k 2n /dev/null $(abspath $^) | \
	 sed s/^.*:.*:.*://g | \
	 expand -t 16 | \
	 tail -n 20

