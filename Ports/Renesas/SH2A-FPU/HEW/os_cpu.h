/*
*********************************************************************************************************
*                                              uC/OS-III
*                                        The Real-Time Kernel
*
*                    Copyright 2009-2020 Silicon Laboratories Inc. www.silabs.com
*
*                                 SPDX-License-Identifier: APACHE-2.0
*
*               This software is subject to an open source license and is distributed by
*                Silicon Laboratories Inc. pursuant to the terms of the Apache License,
*                    Version 2.0 available at www.apache.org/licenses/LICENSE-2.0.
*
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                    Renesas SH-2A-FPU Specific code
*                                   Renesas SH SERIES C/C++ Compiler
*
* File    : os_cpu.h
* Version : V3.08.00
*********************************************************************************************************
*/

#ifndef  OS_CPU_H
#define  OS_CPU_H

#ifdef   OS_CPU_GLOBALS
#define  OS_CPU_EXT
#else
#define  OS_CPU_EXT  extern
#endif

#include <machine.h>


#ifdef __cplusplus
extern  "C" {
#endif

/*
*********************************************************************************************************
*                                       CONFIGURATION DEFAULTS
*********************************************************************************************************
*/

#ifndef  OS_CPU_FPU_EN
#define  OS_CPU_FPU_EN              1u                      /* HW floating point support enabled by default           */
#endif

/*
*********************************************************************************************************
*                                               MACROS
*********************************************************************************************************
*/

#define  OS_TASK_SW_VECT           33u                      /* Interrupt vector # used for context switch             */

#define  OS_TASK_SW()              trapa(OS_TASK_SW_VECT);

/*
*********************************************************************************************************
*                                       TIMESTAMP CONFIGURATION
*
* Note(s) : (1) OS_TS_GET() is generally defined as CPU_TS_Get32() to allow CPU timestamp timer to be of
*               any data type size.
*
*           (2) For architectures that provide 32-bit or higher precision free running counters
*               (i.e. cycle count registers):
*
*               (a) OS_TS_GET() may be defined as CPU_TS_TmrRd() to improve performance when retrieving
*                   the timestamp.
*
*               (b) CPU_TS_TmrRd() MUST be configured to be greater or equal to 32-bits to avoid
*                   truncation of TS.
*********************************************************************************************************
*/

#if      OS_CFG_TS_EN == 1u
#define  OS_TS_GET()               (CPU_TS)CPU_TS_Get32()   /* See Note #2a.                                          */
#else
#define  OS_TS_GET()               (CPU_TS)0u
#endif


/*
*********************************************************************************************************
*                                          GLOBAL VARIABLES
*********************************************************************************************************
*/

OS_CPU_EXT  CPU_STK  *OS_CPU_ExceptStkBase;
OS_CPU_EXT  CPU_STK  *OS_CPU_ExceptStkPtr;


#ifdef __cplusplus
}
#endif

#endif
