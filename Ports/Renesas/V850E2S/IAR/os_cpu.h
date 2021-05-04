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
*                                         Renesas V850E2S Port
*
* File      : os_cpu.h
* Version   : V3.08.01
*********************************************************************************************************
* For       : Renesas V850E2S
* Toolchain : IAR EWV850 v3.7x and 3.8x
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

/*
*********************************************************************************************************
*                                               MACROS
* Note(s) : (1) Ensure that OSCtxSw is registered in the vector address of EITRAP0.
*********************************************************************************************************
*/

#define  OS_TASK_SW()           asm("trap 0x00")          /* See Note #1.                                          */

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


/*
*********************************************************************************************************
*                                        FUNCTION PROTOTYPES
* Note(s) :  (1) OS_CPU_TickInit() must be implemented by the user to initialize the timer, which will
*                be used as the tick interrupt. Moreover, OS_CPU_TickHandler() must be registered in the
*                proper vector address of timer that will be used as the tick.
*********************************************************************************************************
*/

void       OSCtxSw               (void);
void       OSIntCtxSw            (void);
void       OSStartHighRdy        (void);
                                                             /* See Note # 1.                                        */
void       OS_CPU_TickHandler    (void);
void       OS_CPU_TickInit       (CPU_INT32U  tick_per_sec);


#ifdef __cplusplus
}
#endif

#endif
