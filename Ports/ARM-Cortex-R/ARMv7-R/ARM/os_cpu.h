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
*                                             ARMv7-R Port
*
* File      : os_cpu.h
* Version   : V3.08.00
*********************************************************************************************************
* For       : ARMv7-R Cortex-R
* Mode      : ARM or Thumb
* Toolchain : ARM C Compiler
*********************************************************************************************************
* Note(s)   : (1) This port supports the entire 32-bit ARM Cortex-R line from the R4 to the R8
*                 with every possible VFP/NEON coprocessor option.
*
*             (2) To support the various FPUs three versions of os_cpu_a.s are provided.
*                 Only one of them must be used at a time as outlined below.
*
*                 os_cpu_a_vfp-none.s
*                   Suitable when there is no VFP/NEON support or they are deactivated.
*                   Can also be used when saving the VFP/NEON register bank isn't required.
*
*                 os_cpu_a_vfp-d16.s
*                   Suitable for cpus with VFP support and 16 double word registers.
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
*                                          EXCEPTION DEFINES
*********************************************************************************************************
*/

                                                            /* ARM exception IDs                                      */
#define  OS_CPU_ARM_EXCEPT_RESET                      0x00u
#define  OS_CPU_ARM_EXCEPT_UNDEF_INSTR                0x01u
#define  OS_CPU_ARM_EXCEPT_SWI                        0x02u
#define  OS_CPU_ARM_EXCEPT_PREFETCH_ABORT             0x03u
#define  OS_CPU_ARM_EXCEPT_DATA_ABORT                 0x04u
#define  OS_CPU_ARM_EXCEPT_ADDR_ABORT                 0x05u
#define  OS_CPU_ARM_EXCEPT_IRQ                        0x06u
#define  OS_CPU_ARM_EXCEPT_FIQ                        0x07u
#define  OS_CPU_ARM_EXCEPT_NBR                        0x08u


/*
*********************************************************************************************************
*                                               MACROS
*********************************************************************************************************
*/

#define  OS_TASK_SW()               OSCtxSw()

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
#define  OS_TS_GET()               (CPU_TS)CPU_TS_TmrRd()   /* See Note #2a.                                          */
#else
#define  OS_TS_GET()               (CPU_TS)0u
#endif


/*
*********************************************************************************************************
*                                          GLOBAL VARIABLES
*********************************************************************************************************
*/

OS_CPU_EXT  CPU_STK     *OS_CPU_ExceptStkBase;                  /* Exception stack base                                 */
OS_CPU_EXT  CPU_INT32U   OS_CPU_ARM_DRegCnt;                    /* VFP/NEON register count                              */


/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

void        OSCtxSw                             (void);
void        OSIntCtxSw                          (void);
void        OSStartHighRdy                      (void);

void        OS_CPU_ARM_ExceptUndefInstrHndlr    (void);
void        OS_CPU_ARM_ExceptSwiHndlr           (void);
void        OS_CPU_ARM_ExceptPrefetchAbortHndlr (void);
void        OS_CPU_ARM_ExceptDataAbortHndlr     (void);

void        OS_CPU_ARM_ExceptIrqHndlr           (void);
void        OS_CPU_ARM_ExceptFiqHndlr           (void);

void        OS_CPU_ExceptHndlr                  (CPU_INT32U  src_id);

CPU_INT32U  OS_CPU_ARM_DRegCntGet               (void);


#ifdef __cplusplus
}
#endif

#endif
