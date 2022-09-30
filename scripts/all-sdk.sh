#!/bin/sh

for host in aarch64 arm avr riscv sparc; do
  ./scripts/sdk.tcl --host=$host-all-bros --prefix=sdk/$host
done

