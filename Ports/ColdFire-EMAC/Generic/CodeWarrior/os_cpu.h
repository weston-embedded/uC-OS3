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
*                       Generic Coldfire with EMAC Port for CodeWarrior Compiler
*
* File    : os_cpu.h
* Version : V3.08.01
*********************************************************************************************************
*/

#ifndef OS_CPU_H
#define OS_CPU_H

#include  <cpu.h>

#ifdef    OS_CPU_GLOBALS
#define   OS_CPU_EXT
#else
#define   OS_CPU_EXT  extern
#endif

#ifdef __cplusplus
extern  "C" {
#endif

/*
**********************************************************************************************************
*                                          Miscellaneous
**********************************************************************************************************
*/

#define  OS_TASK_SW()          asm(TRAP #14;)                           /* Use Trap #14 to perform a Task Level Context Switch */

#define  OS_STK_GROWTH             1                                    /* Define stack growth: 1 = Down, 0 = Up               */


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

#if     (CPU_CFG_TS_TMR_EN > 0u)
#define  OS_TS_GET()               (CPU_TS)CPU_TS_TmrRd()               /* See Note #2a.                                          */
#else
#define  OS_TS_GET()                  (CPU_TS)0
#endif


/*
************************************************************************************************************************
*                                                   GLOBAL VARIABLES
************************************************************************************************************************
*/

OS_CPU_EXT  CPU_STK  *OS_CPU_ExceptStkBase;


/*
*********************************************************************************************************
*                                          ColdFire Specifics
*********************************************************************************************************
*/

#define  OS_INITIAL_SR        0x2000                                    /* Supervisor mode, interrupts enabled                 */

#define  OS_TRAP_NBR              14                                    /* OSCtxSw() invoked through TRAP #14                  */


/*
**********************************************************************************************************
*                                         Function Prototypes
**********************************************************************************************************
*/

void  OSStartHighRdy(void);
void  OSIntCtxSw    (void);
void  OSCtxSw       (void);


/*
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*/

#ifdef __cplusplus
}
#endif

#endif                                                          /* End of CPU cfg module inclusion.                     */
