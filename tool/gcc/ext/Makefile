# SPDX-License-Identifier: GPL-2.0
# Copyright 2017-2022 Martin Åberg

# WARNING: This script removes files in the current directory.
# Should be used with make -C

# Version Configuration
BINUTILS_VER    = 2.39
GCC_VER         = 12.2.0

BINDIR  = binutils-$(BINUTILS_VER)
GCCDIR  = gcc-$(GCC_VER)

GNURL   = http://ftpmirror.gnu.org/gnu
BINTAR  = $(BINDIR).tar.xz
GCCTAR  = $(GCCDIR).tar.xz

all:
	@echo "use '$(MAKE) extdep' to fetch and extract all dependencies"

extdep: extract-all
fetch-all:      f-binutils f-gcc
extract-all:    x-binutils x-gcc

clean:
	rm -rf $(BINDIR)
	rm -rf $(GCCDIR)

distclean:
	rm -rf $(BINTAR) $(BINDIR)
	rm -rf $(GCCTAR) $(GCCDIR)

f-binutils: $(BINTAR)
$(BINTAR):
	curl -L -R $(GNURL)/binutils/$(BINTAR) -o $@

x-binutils: $(BINTAR)
	tar xf $(BINTAR)
	tar -C binutils-replace -c -f - . | tar -C $(BINDIR) -x -v

f-gcc: $(GCCTAR)
$(GCCTAR):
	curl -L -R $(GNURL)/gcc/$(GCCDIR)/$(GCCTAR) -o $@

x-gcc: $(GCCTAR)
	tar xf $(GCCTAR)
	tar -C gcc-replace -c -f - . | tar -C $(GCCDIR) -x -v

