/*
*********************************************************************************************************
*                                              uC/OS-III
*                                        The Real-Time Kernel
*
*                    Copyright 2009-2021 Silicon Laboratories Inc. www.silabs.com
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
*                                         Synopsys ARC EM6 Port
*
* File      : os_cpu.h
* Version   : V3.08.01
*********************************************************************************************************
* For       : Synopsys ARC EM6
* Mode      : Little-Endian, 32 registers, FPU, Code Density, Loop Counter, Stack Check
* Toolchain : MetaWare C/C++ Clang-based Compiler
*********************************************************************************************************
*/

#ifndef  OS_CPU_H
#define  OS_CPU_H

#ifdef   OS_CPU_GLOBALS
#define  OS_CPU_EXT
#else
#define  OS_CPU_EXT  extern
#endif

#ifdef __cplusplus
extern  "C" {
#endif

#include  <os_cfg_app.h>


/*
*********************************************************************************************************
*                                         CONFIGURATION PRAGMAS
*********************************************************************************************************
*/

#if (!defined(OS_CFG_ISR_STK_SIZE) || (OS_CFG_ISR_STK_SIZE == 0u))
#error  "OS_CFG_APP.H, Invalid OS_CFG_ISR_STK_SIZE. This port uses a separate stack for interrupts."
#endif


/*
*********************************************************************************************************
*                                               OS_TASK_SW
*
* Note(s): OS_TASK_SW()  invokes the task level context switch.
*
*          (1) On some processors, this corresponds to a call to OSCtxSw() which is an assembly language
*              function that performs the context switch.
*
*          (2) On some processors, you need to simulate an interrupt using a 'software interrupt' or a
*              TRAP instruction.  Some compilers allow you to add in-line assembly language as shown.
*********************************************************************************************************
*/

#define  OS_TASK_SW()                               OSCtxSw()


/*
*********************************************************************************************************
*                                        TIMESTAMP CONFIGURATION
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
#define  OS_TS_GET()               (CPU_TS)CPU_TS_TmrRd()       /* See Note #2a.                                        */
#else
#define  OS_TS_GET()               (CPU_TS)0u
#endif

#if (CPU_CFG_TS_32_EN    > 0u) && \
    (CPU_CFG_TS_TMR_SIZE < CPU_WORD_SIZE_32)
                                                                /* CPU_CFG_TS_TMR_SIZE MUST be >= 32-bit (see Note #2b).*/
#error  "cpu_cfg.h, CPU_CFG_TS_TMR_SIZE MUST be >= CPU_WORD_SIZE_32"
#endif


/*
*********************************************************************************************************
*                                OS TICK INTERRUPT PRIORITY CONFIGURATION
*
* Note(s) : (1) For systems that don't need any high, real-time priority interrupts; the tick interrupt
*               should be configured as the highest priority interrupt but won't adversely affect system
*               operations.
*
*           (2) For systems that need one or more high, real-time interrupts; these should be configured
*               higher than the tick interrupt which MAY delay execution of the tick interrupt.
*
*               (a) If the higher priority interrupts do NOT continually consume CPU cycles but only
*                   occasionally delay tick interrupts, then the real-time interrupts can successfully
*                   handle their intermittent/periodic events with the system not losing tick interrupts
*                   but only increasing the jitter.
*
*               (b) If the higher priority interrupts consume enough CPU cycles to continually delay the
*                   tick interrupt, then the CPU/system is most likely over-burdened & can't be expected
*                   to handle all its interrupts/tasks. The system time reference gets compromised as a
*                   result of losing tick interrupts.
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            GLOBAL VARIABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                          FUNCTION PROTOTYPES
*********************************************************************************************************
*/

                                                                /* See OS_CPU_A.S                                       */
void        OSCtxSw                    (void);
void        OSIntCtxSw                 (void);

void        OSStartHighRdy             (void);

void        OS_CPU_EM6_ExceptionHandler(void);
void        OS_CPU_EM6_InterruptHandler(void);
void        OS_CPU_EM6_TrapHandler     (void);
                                                                /* Must be implemented in the BSP.                      */
CPU_INT64U  OS_CPU_GetStackRegion      (void);
void        OS_CPU_ExceptionHandler    (CPU_INT32U  ecr);
void        OS_CPU_InterruptHandler    (CPU_INT08U  int_id);


#ifdef __cplusplus
}
#endif

#endif
