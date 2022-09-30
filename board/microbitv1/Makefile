SUBDIR  :=
SUBDIR  += mod/exec
SUBDIR  += mod/test/exec
SUBDIR  += mod/strap
SUBDIR  += mod/expansion
SUBDIR  += mod/serial/nrf-uart

BUILD   ?= build

TARGET   = rui

RELS    :=
CONFIG  := cfg.mk

all:

include $(CONFIG)
include ../dir.mk
include $(MK)/tool.mk
include $(MK)/gcc.mk

define MOD_template
thisrel := $(BUILD)/$(2)/$(2).rel
RELS += $(BUILD)/$(2)/$(2).rel

template_$(1) :=
template_$(1) += +$$(MAKE)
template_$(1) += -C $(ROOTDIR)/$(1)
template_$(1) += BUILD=$(abspath $(BUILD))/$(2)
template_$(1) += CONFIG=$(abspath $(CONFIG))

.PHONY: $$(thisrel)
$$(thisrel):
	$$(template_$(1))

.PHONY: clean_$$(thisrel)
clean_$$(thisrel):
	$$(template_$(1)) clean
endef

getname = $(lastword $(shell grep MOD $(ROOTDIR)/$(1)/Makefile))

$(foreach d,$(SUBDIR),$(eval $(call MOD_template,$(d),$(call getname,$(d)))))

all: $(TARGET).elf
all: $(TARGET).elf.dis
all: $(TARGET).elf.lst
all: $(TARGET).elf.readelf

LDFLAGS += $(CPUFLAGS)
LDFLAGS += -nostdlib
LDFLAGS += -nostdinc
LDFLAGS += -Wl,-Map=$@.map
LDFLAGS += -T linkcmds-rom
# Needed because we have prefixed $a, $t, $d symbols
DISFLAGS += -D -j ".text" --disassembler-options=force-thumb

$(TARGET).elf: $(RELS)
	$(ECHO) " $(notdir $@)"
	$(CC) $(LDFLAGS) -qabs $(RELS) $(LDLIBS) -o $@

size: $(RELS) $(TARGET).elf

# replace file name and line number with mod name
.PHONY: stacks
stacks: $(RELS)
	cd $(BUILD); \
	 for i in *; do \
	   cat `find $$i -name "*.su"` | \
	    sed s/^.*:.*:.*:/$$i\\t/g; \
	 done | \
	  sort -k 3n | \
	  expand -t 16 | \
	  tail -n 30

.PHONY: clean
clean: $(addprefix clean_,$(RELS))
clean:
	rm -f $(TARGET).elf
	rm -f $(TARGET).elf.map
	rm -f $(TARGET).elf.dis
	rm -f $(TARGET).elf.lst
	rm -f $(TARGET).elf.readelf

