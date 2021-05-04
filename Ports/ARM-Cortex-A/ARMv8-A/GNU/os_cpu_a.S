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
*                                            ARMv8-A Port
*
* File      : os_cpu_a.S
* Version   : V3.08.01
*********************************************************************************************************
* For       : ARMv8-A Cortex-A
* Mode      : ARM64
* Toolchain : GNU
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                          PUBLIC FUNCTIONS
*********************************************************************************************************
*/
                                                                /* External references.                                 */
    .global  OSRunning
    .global  OSPrioCur
    .global  OSPrioHighRdy
    .global  OSTCBCurPtr
    .global  OSTCBHighRdyPtr
    .global  OSIntNestingCtr
    .global  OSIntExit
    .global  OSTaskSwHook
    .global  OS_CPU_ExceptStkBase
    .global  OS_CPU_ExceptHndlr

                                                                /* Functions declared in this file.                     */
    .global  OSStartHighRdy
    .global  OSCtxSw
    .global  OSIntCtxSw
    .global  OS_CPU_ARM_ExceptIrqHndlr
    .global  OS_CPU_SPSRGet
    .global  OS_CPU_SIMDGet


/*
*********************************************************************************************************
*                                               EQUATES
*********************************************************************************************************
*/

#ifndef OS_CPU_EL3
#define OS_CPU_EL3 1
#endif

#ifndef OS_CPU_SIMD
#define OS_CPU_SIMD 1
#endif


/*
*********************************************************************************************************
*                                     CODE GENERATION DIRECTIVES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                           REGISTER MACROS
*********************************************************************************************************
*/

    .macro OS_CPU_ARM_REG_POP

        #if OS_CPU_SIMD == 1
        LDP  x28, x29, [sp], #16
        MSR  FPSR, x28
        MSR  FPCR, x29

        LDP  q0, q1, [sp], #32
        LDP  q2, q3, [sp], #32
        LDP  q4, q5, [sp], #32
        LDP  q6, q7, [sp], #32
        LDP  q8, q9, [sp], #32
        LDP  q10, q11, [sp], #32
        LDP  q12, q13, [sp], #32
        LDP  q14, q15, [sp], #32
        LDP  q16, q17, [sp], #32
        LDP  q18, q19, [sp], #32
        LDP  q20, q21, [sp], #32
        LDP  q22, q23, [sp], #32
        LDP  q24, q25, [sp], #32
        LDP  q26, q27, [sp], #32
        LDP  q28, q29, [sp], #32
        LDP  q30, q31, [sp], #32
        #endif

        LDP  x0, x1, [sp], #16
        #if OS_CPU_EL3 == 1
        MSR  SPSR_EL3, x1
        #else
        MSR  SPSR_EL1, x1
        #endif

        LDP  x30, x0, [sp], #16
        #if OS_CPU_EL3 == 1
        MSR  ELR_EL3, x0
        #else
        MSR  ELR_EL1, x0
        #endif

        LDP  x0, x1, [sp], #16
        LDP  x2, x3, [sp], #16
        LDP  x4, x5, [sp], #16
        LDP  x6, x7, [sp], #16
        LDP  x8, x9, [sp], #16
        LDP  x10, x11, [sp], #16
        LDP  x12, x13, [sp], #16
        LDP  x14, x15, [sp], #16
        LDP  x16, x17, [sp], #16
        LDP  x18, x19, [sp], #16
        LDP  x20, x21, [sp], #16
        LDP  x22, x23, [sp], #16
        LDP  x24, x25, [sp], #16
        LDP  x26, x27, [sp], #16
        LDP  x28, x29, [sp], #16
    .endm

    .macro OS_CPU_ARM_REG_PUSH
        STP  x28, x29, [sp, #-16]!
        STP  x26, x27, [sp, #-16]!
        STP  x24, x25, [sp, #-16]!
        STP  x22, x23, [sp, #-16]!
        STP  x20, x21, [sp, #-16]!
        STP  x18, x19, [sp, #-16]!
        STP  x16, x17, [sp, #-16]!
        STP  x14, x15, [sp, #-16]!
        STP  x12, x13, [sp, #-16]!
        STP  x10, x11, [sp, #-16]!
        STP  x8, x9, [sp, #-16]!
        STP  x6, x7, [sp, #-16]!
        STP  x4, x5, [sp, #-16]!
        STP  x2, x3, [sp, #-16]!
        STP  x0, x1, [sp, #-16]!

        #if OS_CPU_EL3 == 1
        MRS  x0, ELR_EL3
        #else
        MRS  x0, ELR_EL1
        #endif

        STP  x30, x0, [sp, #-16]!

        #if OS_CPU_EL3 == 1
        MRS  x0, SPSR_EL3
        #else
        MRS  x0, SPSR_EL1
        #endif

        MOV  x1, x0
        STP  x0, x1, [sp, #-16]!

        #if OS_CPU_SIMD == 1
        STP  q30, q31, [sp, #-32]!
        STP  q28, q29, [sp, #-32]!
        STP  q26, q27, [sp, #-32]!
        STP  q24, q25, [sp, #-32]!
        STP  q22, q23, [sp, #-32]!
        STP  q20, q21, [sp, #-32]!
        STP  q18, q19, [sp, #-32]!
        STP  q16, q17, [sp, #-32]!
        STP  q14, q15, [sp, #-32]!
        STP  q12, q13, [sp, #-32]!
        STP  q10, q11, [sp, #-32]!
        STP  q8, q9, [sp, #-32]!
        STP  q6, q7, [sp, #-32]!
        STP  q4, q5, [sp, #-32]!
        STP  q2, q3, [sp, #-32]!
        STP  q0, q1, [sp, #-32]!

        MRS  x28, FPSR
        MRS  x29, FPCR
        STP  x28, x29, [sp, #-16]!
        #endif
    .endm

    .macro OS_CPU_ARM_REG_PUSHF
        STP  x28, x29, [sp, #-16]!
        STP  x26, x27, [sp, #-16]!
        STP  x24, x25, [sp, #-16]!
        STP  x22, x23, [sp, #-16]!
        STP  x20, x21, [sp, #-16]!
        STP  x18, x19, [sp, #-16]!
        SUB  sp, sp, #144

        MOV  x0, x30
        STP  x30, x0, [sp, #-16]!

        #if OS_CPU_EL3 == 1
        MOV  x0, #0x0000020D
        #else
        MOV  x0, #0x00000205
        #endif
        MOV  x1, x0
        STP  x0, x1, [sp, #-16]!

        #if OS_CPU_SIMD == 1
        SUB  sp, sp, #256
        STP  q14, q15, [sp, #-32]!
        STP  q12, q13, [sp, #-32]!
        STP  q10, q11, [sp, #-32]!
        STP  q8, q9, [sp, #-32]!
        SUB  sp, sp, #128

        MRS  x28, FPSR
        MRS  x29, FPCR
        STP  x28, x29, [sp, #-16]!
        #endif
    .endm


/*
*********************************************************************************************************
*                                         START MULTITASKING
*                                      void OSStartHighRdy(void)
*
* Note(s) : 1) OSStartHighRdy() MUST:
*              a) Call OSTaskSwHook() then,
*              b) Set OSRunning to OS_STATE_OS_RUNNING,
*              c) Switch to the highest priority task.
*********************************************************************************************************
*/

OSStartHighRdy:

    BL   OSTaskSwHook

    LDR  x0, =OSRunning
    MOV  w1, #1
    STRB w1, [x0]

    LDR  x0, =OSTCBHighRdyPtr
    LDR  x1, [x0]
    LDR  x2, [x1]
    MOV  sp, x2

    #if OS_CPU_SIMD == 1
    LDP  x28, x29, [sp], #16
    MSR  FPSR, x28
    MSR  FPCR, x29

    LDP  q0, q1, [sp], #32
    LDP  q2, q3, [sp], #32
    LDP  q4, q5, [sp], #32
    LDP  q6, q7, [sp], #32
    LDP  q8, q9, [sp], #32
    LDP  q10, q11, [sp], #32
    LDP  q12, q13, [sp], #32
    LDP  q14, q15, [sp], #32
    LDP  q16, q17, [sp], #32
    LDP  q18, q19, [sp], #32
    LDP  q20, q21, [sp], #32
    LDP  q22, q23, [sp], #32
    LDP  q24, q25, [sp], #32
    LDP  q26, q27, [sp], #32
    LDP  q28, q29, [sp], #32
    LDP  q30, q31, [sp], #32
    #endif

    LDP  x0,  x1, [sp], #16
    LDP  x30, x3, [sp], #16

    MRS  x4, CurrentEL
    LSR  x4, x4, #2
    CMP  x4, #3 /* EL3 */
    B.EQ  OSStartHighRdy_EL3
    CMP  x4, #2 /* EL2 */
    B.EQ  OSStartHighRdy_EL2
    CMP  x4, #1 /* EL1 */
    B.EQ  OSStartHighRdy_EL1

    B . /* Can't run the kernel from EL0 */

OSStartHighRdy_EL3:
    MSR  SPSR_EL3, x1
    MSR  ELR_EL3, x3
    B  OSStartHighRdy_Restore

OSStartHighRdy_EL2:
    MSR  SPSR_EL2, x1
    MSR  ELR_EL2, x3
    B  OSStartHighRdy_Restore

OSStartHighRdy_EL1:
    MSR  SPSR_EL1, x1
    MSR  ELR_EL1, x3
    B  OSStartHighRdy_Restore

OSStartHighRdy_Restore:
    #if OS_CPU_EL3 == 0
    MOV  x0, sp
    SUB  x0, x0, #240
    MSR  SP_EL1, x0
    #endif

    LDP  x0, x1, [sp], #16
    LDP  x2, x3, [sp], #16
    LDP  x4, x5, [sp], #16
    LDP  x6, x7, [sp], #16
    LDP  x8, x9, [sp], #16
    LDP  x10, x11, [sp], #16
    LDP  x12, x13, [sp], #16
    LDP  x14, x15, [sp], #16
    LDP  x16, x17, [sp], #16
    LDP  x18, x19, [sp], #16
    LDP  x20, x21, [sp], #16
    LDP  x22, x23, [sp], #16
    LDP  x24, x25, [sp], #16
    LDP  x26, x27, [sp], #16
    LDP  x28, x29, [sp], #16
    ERET

/*
*********************************************************************************************************
*                       PERFORM A CONTEXT SWITCH (From task level) - OSCtxSw()
*
* Note(s) : 1) OSCtxSw() is called in SVC mode with BOTH FIQ and IRQ interrupts DISABLED.
*
*           2) The pseudo-code for OSCtxSw() is:
*              a) Save the current task's context onto the current task's stack,
*              b) OSTCBCurPtr->StkPtr = SP*
*              c) OSTaskSwHook()*
*              d) OSPrioCur           = OSPrioHighRdy*
*              e) OSTCBCurPtr         = OSTCBHighRdyPtr*
*              f) SP                  = OSTCBHighRdyPtr->StkPtr*
*              g) Restore the new task's context from the new task's stack,
*              h) Return to new task's code.
*
*           3) Upon entry:
*              OSTCBCurPtr      points to the OS_TCB of the task to suspend,
*              OSTCBHighRdyPtr  points to the OS_TCB of the task to resume.
*********************************************************************************************************
*/

OSCtxSw:

    OS_CPU_ARM_REG_PUSHF

    LDR x0, =OSTCBCurPtr
    LDR x1, [x0]
    MOV x2, sp
    STR x2, [x1]


    BL OSTaskSwHook


    LDR x0, =OSPrioCur
    LDR x1, =OSPrioHighRdy
    LDRB w2, [x1]
    STRB w2, [x0]


    LDR x0, =OSTCBCurPtr
    LDR x1, =OSTCBHighRdyPtr
    LDR x2, [x1]
    STR x2, [x0]


    LDR x0, [x2]
    MOV sp, x0

    OS_CPU_ARM_REG_POP
    ERET


/*
*********************************************************************************************************
*                   PERFORM A CONTEXT SWITCH (From interrupt level) - OSIntCtxSw()
*
* Note(s) : 1) OSIntCtxSw() is called in SVC mode with BOTH FIQ and IRQ interrupts DISABLED.
*
*           2) The pseudo-code for OSCtxSw() is:
*              a) OSTaskSwHook()*
*              b) OSPrioCur   = OSPrioHighRdy*
*              c) OSTCBCurPtr = OSTCBHighRdyPtr*
*              d) SP          = OSTCBHighRdyPtr->OSTCBStkPtr*
*              e) Restore the new task's context from the new task's stack,
*              f) Return to new task's code.
*
*           3) Upon entry:
*              OSTCBCurPtr      points to the OS_TCB of the task to suspend,
*              OSTCBHighRdyPtr  points to the OS_TCB of the task to resume.
*********************************************************************************************************
*/

OSIntCtxSw:

    BL OSTaskSwHook

    LDR x0, =OSPrioCur
    LDR x1, =OSPrioHighRdy
    LDRB w2, [x1]
    STRB w2, [x0]


    LDR x0, =OSTCBCurPtr
    LDR x1, =OSTCBHighRdyPtr
    LDR x2, [x1]
    STR x2, [x0]


    LDR x0, [x2]
    MOV sp, x0

    OS_CPU_ARM_REG_POP
    ERET


/*
*********************************************************************************************************
*                                        ARMv8-A IRQ EXCEPTION
*********************************************************************************************************
*/

OS_CPU_ARM_ExceptIrqHndlr:

    OS_CPU_ARM_REG_PUSH

    LDR  x0, =OSIntNestingCtr
    LDRB w1, [x0]
    ADD  w1, w1, #1
    STRB w1, [x0]
    CMP  w1, #1
    BNE  OS_CPU_ARM_ExceptHndlr_BreakExcept

    LDR  x0, =OSTCBCurPtr
    LDR  x1, [x0]
    MOV  x2, sp
    STR  x2, [x1]

    LDR  x0, =OS_CPU_ExceptStkBase
    LDR  x1, [x0]
    MOV  sp, x1

    BL   OS_CPU_ExceptHndlr

    BL   OSIntExit

    LDR  x0, =OSTCBCurPtr
    LDR  x1, [x0]
    LDR  x2, [x1]
    MOV  sp, x2

    OS_CPU_ARM_REG_POP
    ERET


OS_CPU_ARM_ExceptHndlr_BreakExcept:

    BL   OS_CPU_ExceptHndlr

    LDR  x0, =OSIntNestingCtr
    LDRB w1, [x0]
    SUB  w1, w1, #1
    STRB w1, [x0]

    OS_CPU_ARM_REG_POP

    ERET


/*
*********************************************************************************************************
*                                           HELPER ROUTINES
*
* Note(s) : 1) OS_CPU_SPSRGet and OS_CPU_SIMDGet are used by os_cpu_c.c.
*********************************************************************************************************
*/
OS_CPU_SPSRGet:
    #if OS_CPU_EL3 == 1
    MOV x0, #0x0000000D
    #else
    MOV x0, #0x00000005
    #endif

    RET


OS_CPU_SIMDGet:
    #if OS_CPU_SIMD == 1
    MOV x0, #1
    #else
    MOV x0, #0
    #endif

    RET
