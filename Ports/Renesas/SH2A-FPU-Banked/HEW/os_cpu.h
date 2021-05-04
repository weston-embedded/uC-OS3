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
*                           Renesas SH2A-FPU Specific code (RESBANK version)
*                                   Renesas SH SERIES C/C++ Compiler
*
* File    : os_cpu.h
* Version : V3.08.01
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
************************************************************************************************************************
*                                                CONFIGURATION DEFAULTS
************************************************************************************************************************
*/

#ifndef  OS_CPU_FPU_EN
#define  OS_CPU_FPU_EN    1u                                /* Harware floating point support enabled by default      */
#endif

/*
************************************************************************************************************************
*                                                        MACROS
************************************************************************************************************************
*/

#define  OS_TASK_SW_VECT           33u                      /* Interrupt vector # used for context switch             */

#define  OS_TASK_SW()              trapa(OS_TASK_SW_VECT);

#if      OS_CFG_TS_EN == 1u
#define  OS_TS_GET()               (CPU_TS)CPU_TS_Get32()
#else
#define  OS_TS_GET()               (CPU_TS)0u
#endif

/*
************************************************************************************************************************
*                                                 FUNCTION PROTOTYPES
************************************************************************************************************************
*/

CPU_INT32U  OS_Get_GBR       (void);
void        OS_C_ISR_Save    (void);
void        OS_C_ISR_Restore (void);
void        OSCtxSw          (void);
void        OSIntCtxSw       (void);
void        OSStartHighRdy   (void);
void        OSTickISR        (void);


#ifdef __cplusplus
}
#endif

#endif
