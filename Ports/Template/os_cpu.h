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
*                                         $$$$ TEMPLATE
*                                         $$$$ Insert CPU Name
*                                         $$$$ Insert Compiler Name
*
* Filename : os_cpu.h
* Version  : $$$$ V3.08.00
*********************************************************************************************************
* Note(s)  : (1) This file is used to create a uC/OS-III port.  You can use this template as a
*                starting point instead of typing everything from scratch.
*
*                You should replace anything that is preceded by '$$$$' with target specific code.
*********************************************************************************************************
*/

#ifndef _OS_CPU_H
#define _OS_CPU_H

#ifdef  OS_CPU_GLOBALS
#define OS_CPU_EXT
#else
#define OS_CPU_EXT  extern
#endif

#ifdef __cplusplus
extern  "C" {
#endif

/*
*********************************************************************************************************
*                                               MACROS
*
* Note(s): OS_TASK_SW()  invokes the task level context switch.
*
*          (1) On some processors, this corresponds to a call to OSCtxSw() which is an assemply language
*              function that performs the context switch.
*
*          (2) On some processors, you need to simulate an interrupt using a 'sowfate interrupt' or a
*              TRAP instruction.  Some compilers allow you to add in-line assembly language as shown.
*********************************************************************************************************
*/

#define  OS_TASK_SW()                              /* $$$$ Simulate interrupt                          */

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
*                   the timestamp.  You would use CPU_TS_TmrRd() if this function returned the value of
*                   a 32-bit free running timer 0x00000000 to 0xFFFFFFFF then roll over to 0x00000000.
*
*               (b) CPU_TS_TmrRd() MUST be configured to be greater or equal to 32-bits to avoid
*                   truncation of TS.
*
*               (c) The Timer must be an up counter.
*********************************************************************************************************
*/

#if      OS_CFG_TS_EN == 1u
#define  OS_TS_GET()               (CPU_TS)CPU_TS_Get32()   /* See Note #2a.                                          */
#else
#define  OS_TS_GET()               (CPU_TS)0u
#endif


/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

void  OSCtxSw        (void);
void  OSIntCtxSw     (void);
void  OSStartHighRdy (void);


#ifdef __cplusplus
}
#endif

#endif
