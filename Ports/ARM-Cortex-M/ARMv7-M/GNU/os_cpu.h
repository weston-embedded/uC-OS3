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
*                                             ARMv7-M Port
*
* File      : os_cpu.h
* Version   : V3.08.00
*********************************************************************************************************
* For       : ARMv7-M Cortex-M
* Mode      : Thumb-2 ISA
* Toolchain : GNU C Compiler
*********************************************************************************************************
* Note(s)   : (1) This port supports the ARM Cortex-M3, Cortex-M4 and Cortex-M7 architectures.
*             (2) It has been tested with the following Hardware Floating Point Unit.
*                 (a) Single-precision: FPv4-SP-D16-M and FPv5-SP-D16-M
*                 (b) Double-precision: FPv5-D16-M
*********************************************************************************************************
*/

#ifndef  OS_CPU_H
#define  OS_CPU_H

#ifdef   OS_CPU_GLOBALS
#define  OS_CPU_EXT
#else
#define  OS_CPU_EXT  extern
#endif


/*
*********************************************************************************************************
*                                     EXTERNAL C LANGUAGE LINKAGE
*
* Note(s) : (1) C++ compilers MUST 'extern'ally declare ALL C function prototypes & variable/object
*               declarations for correct C language linkage.
*********************************************************************************************************
*/

#ifdef __cplusplus
extern  "C" {                                    /* See Note #1.                                       */
#endif


/*
*********************************************************************************************************
*                                               DEFINES
* Note(s) : (1) Determines the interrupt programmable priority levels. This is normally specified in the
*               Microcontroller reference manual. 4-bits gives us 16 programmable priority levels.
*********************************************************************************************************
*/

#if (defined(__VFP_FP__) && !defined(__SOFTFP__))
#define  OS_CPU_ARM_FP_EN              1u
#else
#define  OS_CPU_ARM_FP_EN              0u
#endif

#ifndef CPU_CFG_KA_IPL_BOUNDARY
#error  "CPU_CFG_KA_IPL_BOUNDARY         not #define'd in 'cpu_cfg.h'    "
#else
#if (CPU_CFG_KA_IPL_BOUNDARY == 0u)
#error  "CPU_CFG_KA_IPL_BOUNDARY        should be > 0 "
#endif
#endif

#ifndef CPU_CFG_NVIC_PRIO_BITS
#error  "CPU_CFG_NVIC_PRIO_BITS         not #define'd in 'cpu_cfg.h'    "   /* See Note # 1            */
#else
#if (CPU_CFG_KA_IPL_BOUNDARY >= (1u << CPU_CFG_NVIC_PRIO_BITS))
#error  "CPU_CFG_KA_IPL_BOUNDARY        should not be set to higher than max programable priority level "
#endif
#endif


/*
*********************************************************************************************************
*                                               MACROS
*********************************************************************************************************
*/

#define  OS_TASK_SW()               OSCtxSw()

#define  OS_TASK_SW_SYNC()          __asm__ __volatile__ ("isb" : : : "memory")


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

#if (CPU_CFG_TS_32_EN    > 0u) && \
    (CPU_CFG_TS_TMR_SIZE < CPU_WORD_SIZE_32)
                                                            /* CPU_CFG_TS_TMR_SIZE MUST be >= 32-bit (see Note #2b).  */
#error  "cpu_cfg.h, CPU_CFG_TS_TMR_SIZE MUST be >= CPU_WORD_SIZE_32"
#endif


/*
*********************************************************************************************************
*                                          GLOBAL VARIABLES
*********************************************************************************************************
*/

OS_CPU_EXT  CPU_STK  *OS_CPU_ExceptStkBase;


/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

                                                  /* See OS_CPU_A.ASM                                  */
void  OSCtxSw               (void);
void  OSIntCtxSw            (void);
void  OSStartHighRdy        (void);

                                                  /* See OS_CPU_C.C                                    */
void  OS_CPU_SysTickInit    (CPU_INT32U   cnts);
void  OS_CPU_SysTickInitFreq(CPU_INT32U   cpu_freq);

void  OS_CPU_SysTickHandler (void);
void  OS_CPU_PendSVHandler  (void);

#if (OS_CPU_ARM_FP_EN > 0u)
void  OS_CPU_FP_Reg_Push    (CPU_STK     *stkPtr);
void  OS_CPU_FP_Reg_Pop     (CPU_STK     *stkPtr);
#endif


/*
*********************************************************************************************************
*                                   EXTERNAL C LANGUAGE LINKAGE END
*********************************************************************************************************
*/

#ifdef __cplusplus
}                                                 /* End of 'extern'al C lang linkage.                 */
#endif


/*
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*/

#endif
