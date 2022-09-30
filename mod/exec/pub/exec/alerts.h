/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2020-2022 Martin Åberg */

#ifndef EXEC_ALERTS_H
#define EXEC_ALERTS_H

/*
 * This file contains defines for using with the Alert()
 * function. Build an alert argument by combining one of the
 * AT_, AN_, AG_ and AO_ fields. The lower 8 bits are caller
 * defined.
 *
 * Example: "timer.device failed to open expansion.library, and
 * can recover (exit cleanly)"
 * Alert(AT_Recovery | AN_TimerDev | AG_OpenLib | AO_Expansion)
 *
 * Example: "CPU exception num which exec.library does not handle"
 * Alert(AT_DeadEnd | AN_ExecLib | AG_Exception | num)
 */

/* Severity */
#define AT_DeadEnd      0x80000000U /* Alert() shall not return */
#define AT_Recovery     0x00000000U /* Alert() allowed to return */

/* The alerting module */
#define AN_Application  0x01000000U
#define AN_Library      0x02000000U
#define AN_Device       0x03000000U
#define AN_Driver       0x04000000U
#define AN_FileSystem   0x05000000U
#define AN_ExecLib      0x10000000U
#define AN_FileLib      0x11000000U
#define AN_TimerDev     0x12000000U
#define AN_BioLib       0x13000000U
#define AN_RamHandler   0x14000000U
#define AN_Strap        0x15000000U
#define AN_Expansion    0x16000000U

/* The condition or action involved */
#define AG_NoMemory     0x00010000U
#define AG_NoImpl       0x00020000U /* not implemented */
#define AG_Assert       0x00030000U /* Assert hit */
#define AG_OpenLib      0x00040000U
#define AG_CreateTask   0x00050000U
#define AG_RemTask      0x00060000U
#define AG_Mount        0x00070000U
#define AG_Exception    0x00080000U /* Unhandled exception */
#define AG_Panic        0x00090000U
#define AG_MemList      0x000a0000U
#define AG_StackCheck   0x000b0000U
#define AG_Canaries     0x000c0000U
#define AG_NoCPU        0x000d0000U

/* Details of the action or condition */
#define AO_ExecLib      0x00001000U
#define AO_FileLib      0x00001100U
#define AO_TimerDev     0x00001200U
#define AO_Expansion    0x00001600U

#endif

