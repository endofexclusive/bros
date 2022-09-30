/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022 Martin Åberg */

#include <sbi/sbi.h>
#include <priv.h>
#include <arch_priv.h>

void kprint_cpu(Lib *lib) {
  unsigned long value;
  long v;
  unsigned int specver;

  kprintf(lib, "  SBI");

  specver = sbi_get_spec_version().value;
  kprintf(lib, " spec_version %u.%u", specver>>24,
   specver & 0xffffff);
  kprintf(lib, "  impl_id %ld", sbi_get_impl_id().value);
  kprintf(lib, "  impl_version %ld\n",
   sbi_get_impl_version().value);

  value = sbi_get_mvendorid().value;
  kprintf(lib, "%11s: %08x", "mvendorid", (unsigned) value);

  value = sbi_get_marchid().value;
  kprintf(lib, "%11s: " PR_REG "", "marchid", value);

  value = sbi_get_mimpid().value;
  kprintf(lib, "%11s: " PR_REG "\n", "mimpid", value);

  kprintf(lib, "  SBI extensions:");
  v = sbi_probe_extension(SBI_EID_TIME).value;
  kprintf(lib, "%s", v ? " TIME" : "");
  v = sbi_probe_extension(SBI_EID_IPI).value;
  kprintf(lib, "%s", v ? " 'sPI: s-mode IPI'" : "");
  v = sbi_probe_extension(SBI_EID_RFNC).value;
  kprintf(lib, "%s", v ? " RFNC" : "");
  v = sbi_probe_extension(SBI_EID_HSM).value;
  kprintf(lib, "%s", v ? " HSM" : "");
  v = sbi_probe_extension(SBI_EID_SRST).value;
  kprintf(lib, "%s", v ? " SRST" : "");
  v = sbi_probe_extension(SBI_EID_PMU).value;
  kprintf(lib, "%s", v ? " PMU" : "");
  for (int eid = 0x00; eid < 0x10; eid++) {
    v = sbi_probe_extension(eid).value;
    if (v) {
      kprintf(lib, " %02x", (unsigned) eid);
    }
  }
  kprintf(lib, "\n");
}

