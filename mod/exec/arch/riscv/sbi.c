/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022 Martin Åberg */

#include <sbi/sbi.h>

extern struct sbiret riscv_ecall(
  unsigned long a0,
  unsigned long a1,
  unsigned long a2,
  unsigned long a3,
  unsigned long a4,
  unsigned long a5,
  unsigned long a6,
  unsigned long a7
);

static struct sbiret sbi_call(
  unsigned long a0,
  unsigned long a1,
  unsigned long a2,
  unsigned long a3,
  unsigned long a4,
  unsigned long a5,
  unsigned int fid,
  unsigned int eid
) {
  return riscv_ecall(a0, a1, a2, a3, a4, a5, fid, eid);
}

static struct sbiret base_ecall(int fid) {
  return sbi_call(0, 0, 0, 0, 0, 0, fid, SBI_EID_BASE);
}

struct sbiret sbi_get_spec_version(void) {
  return base_ecall(SBI_FID_BASE_GET_SPEC_VERSION);
}

struct sbiret sbi_get_impl_id(void) {
  return base_ecall(SBI_FID_BASE_GET_IMPL_ID);
}

struct sbiret sbi_get_impl_version(void) {
  return base_ecall(SBI_FID_BASE_GET_IMPL_VERSION);
}

struct sbiret sbi_probe_extension(long extension_id) {
  return sbi_call(extension_id, 0, 0, 0, 0, 0,
   SBI_FID_BASE_PROBE_EXTENSION, SBI_EID_BASE);
}

struct sbiret sbi_get_mvendorid(void) {
  return base_ecall(SBI_FID_BASE_GET_MVENDORID);
}

struct sbiret sbi_get_marchid(void) {
  return base_ecall(SBI_FID_BASE_GET_MARCHID);
}

struct sbiret sbi_get_mimpid(void) {
  return base_ecall(SBI_FID_BASE_GET_MIMPID);
}

long sbi_console_putchar(int ch) {
  struct sbiret ret;
  ret = sbi_call(ch, 0, 0, 0, 0, 0, 0, SBI_LEGACY_CONSOLE_PUTCHAR);
  return ret.error;
}

long sbi_console_getchar(void) {
  struct sbiret ret;
  ret = sbi_call(0, 0, 0, 0, 0, 0, 0, SBI_LEGACY_CONSOLE_GETCHAR);
  return ret.error;
}

struct sbiret sbi_set_timer(unsigned long long stime_value) {
  unsigned long lo = stime_value;
  unsigned long hi;
  hi = 0;
  if (__riscv_xlen == 32) {
    hi = stime_value >> 32;
  }
  return sbi_call(lo, hi, 0, 0, 0, 0,
   SBI_FID_TIME_SET_TIMER, SBI_EID_TIME);
}

struct sbiret sbi_send_ipi(
  unsigned long hart_mask,
  unsigned long hart_mask_base
) {
  return sbi_call(hart_mask, hart_mask_base, 0, 0, 0, 0,
   SBI_FID_IPI_SEND_IPI, SBI_EID_IPI);
}

struct sbiret sbi_remote_fence_i(
  unsigned long hart_mask,
  unsigned long hart_mask_base
) {
  return sbi_call(hart_mask, hart_mask_base, 0, 0, 0, 0,
   SBI_FID_RFNC_REMOTE_FENCE_I, SBI_EID_RFNC);
}

struct sbiret sbi_hart_start(
  unsigned long hartid,
  unsigned long start_addr,
  unsigned long opaque
) {
  return sbi_call(hartid, start_addr, opaque, 0, 0, 0,
   SBI_FID_HSM_HART_START, SBI_EID_HSM);
}

struct sbiret sbi_hart_get_status(unsigned long hartid) {
  return sbi_call(hartid, 0, 0, 0, 0, 0,
   SBI_FID_HSM_HART_GET_STATUS, SBI_EID_HSM);
}

struct sbiret sbi_system_reset(
  unsigned int reset_type,
  unsigned int reset_reason
) {
  return sbi_call(reset_type, reset_reason, 0, 0, 0, 0,
   SBI_FID_SRST_SYSTEM_RESET, SBI_EID_SRST);
}

