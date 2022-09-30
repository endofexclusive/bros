/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022 Martin Åberg */

#ifndef SBI_SBI_H
#define SBI_SBI_H

/*
 * Based on:
 *   "RISC-V Supervisor Binary Interface Specification Version
 *   1.0.0, March 22, 2022: Ratified"
 */

/* Chapter 3. Binary Encoding */
struct sbiret {
  long error;
  long value;
};
/* Table 1. Standard SBI Errors */
#define SBI_SUCCESS                0
#define SBI_ERR_FAILED            -1
#define SBI_ERR_NOT_SUPPORTED     -2
#define SBI_ERR_INVALID_PARAM     -3
#define SBI_ERR_DENIED            -4
#define SBI_ERR_INVALID_ADDRESS   -5
#define SBI_ERR_ALREADY_AVAILABLE -6
#define SBI_ERR_ALREADY_STARTED   -7
#define SBI_ERR_ALREADY_STOPPED   -8


/* Chapter 4. Base Extension */
#define SBI_EID_BASE 0x00000010
enum {
  SBI_FID_BASE_GET_SPEC_VERSION = 0,
  SBI_FID_BASE_GET_IMPL_ID,
  SBI_FID_BASE_GET_IMPL_VERSION,
  SBI_FID_BASE_PROBE_EXTENSION,
  SBI_FID_BASE_GET_MVENDORID,
  SBI_FID_BASE_GET_MARCHID,
  SBI_FID_BASE_GET_MIMPID,
};
struct sbiret sbi_get_spec_version(void);
struct sbiret sbi_get_impl_id(void);
struct sbiret sbi_get_impl_version(void);
struct sbiret sbi_probe_extension(long extension_id);
struct sbiret sbi_get_mvendorid(void);
struct sbiret sbi_get_marchid(void);
struct sbiret sbi_get_mimpid(void);


/* Chapter 5. Legacy Extensions */
#define SBI_LEGACY_CONSOLE_PUTCHAR 0x01
#define SBI_LEGACY_CONSOLE_GETCHAR 0x02
long sbi_console_putchar(int ch);
long sbi_console_getchar(void);


/* Chapter 6. Timer Extension */
#define SBI_EID_TIME 0x54494D45
enum {
  SBI_FID_TIME_SET_TIMER = 0,
};
struct sbiret sbi_set_timer(unsigned long long stime_value);


/* Chapter 7. IPI Extension "sPI: s-mode IPI" */
#define SBI_EID_IPI 0x00735049
enum {
  SBI_FID_IPI_SEND_IPI = 0,
};
struct sbiret sbi_send_ipi(
  unsigned long hart_mask,
  unsigned long hart_mask_base
);


/* Chapter 8. RFENCE Extension */
#define SBI_EID_RFNC 0x52464E43
enum {
  SBI_FID_RFNC_REMOTE_FENCE_I = 0,
};
struct sbiret sbi_remote_fence_i(
  unsigned long hart_mask,
  unsigned long hart_mask_base
);


/* Chapter 9. Hart State Management Extension */
#define SBI_EID_HSM 0x0048534D
enum {
  SBI_FID_HSM_HART_START = 0,
  SBI_FID_HSM_HART_STOP,
  SBI_FID_HSM_HART_GET_STATUS,
  SBI_FID_HSM_HART_SUSPEND,
};
enum {
  SBI_HSM_HART_STATUS_STARTED = 0,
  SBI_HSM_HART_STATUS_STOPPED,
  SBI_HSM_HART_STATUS_START_PENDING,
  SBI_HSM_HART_STATUS_STOP_PENDING,
  SBI_HSM_HART_STATUS_SUSPENDED,
  SBI_HSM_HART_STATUS_SUSPEND_PENDING,
  SBI_HSM_HART_STATUS_RESUME_PENDING,
};
struct sbiret sbi_hart_start(
  unsigned long hartid,
  unsigned long start_addr,
  unsigned long opaque
);
struct sbiret sbi_hart_get_status(unsigned long hartid);


/* Chapter 10. System Reset Extension */
#define SBI_EID_SRST 0x53525354
enum {
  SBI_FID_SRST_SYSTEM_RESET = 0,
};
enum {
  SBI_SRST_TYPE_SHUTDOWN = 0,
  SBI_SRST_TYPE_COLD_REBOOT,
  SBI_SRST_TYPE_WARM_REBOOT,
};
enum {
  SBI_SRST_REASON_NO_REASON = 0,
  SBI_SRST_REASON_SYSTEM_FAILURE,
};
struct sbiret sbi_system_reset(
  unsigned int reset_type,
  unsigned int reset_reason
);


/* Chapter 11. Performance Monitoring Unit Extension */
#define SBI_EID_PMU 0x00504D55

#endif

