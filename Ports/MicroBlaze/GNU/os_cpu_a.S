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
*                                           Xilinx MicroBlaze
*                                          GNU C/C++ Compiler
*
* File    : os_cpu_a.S
* Version : V3.08.01
*********************************************************************************************************
*/

#define _ASMLANGUAGE

/*
*********************************************************************************************************
*                                           LOCAL CONSTANTS
*********************************************************************************************************
*/

.equ    STK_OFFSET_RMSR,   0x00
.equ    STK_OFFSET_R02,    0x04
.equ    STK_OFFSET_R03,    0x08
.equ    STK_OFFSET_R04,    0x0C
.equ    STK_OFFSET_R05,    0x10
.equ    STK_OFFSET_R06,    0x14
.equ    STK_OFFSET_R07,    0x18
.equ    STK_OFFSET_R08,    0x1C
.equ    STK_OFFSET_R09,    0x20
.equ    STK_OFFSET_R10,    0x24
.equ    STK_OFFSET_R11,    0x28
.equ    STK_OFFSET_R12,    0x2C
.equ    STK_OFFSET_R13,    0x30
.equ    STK_OFFSET_R14,    0x34
.equ    STK_OFFSET_R15,    0x38
.equ    STK_OFFSET_R17,    0x3C
.equ    STK_OFFSET_R18,    0x40
.equ    STK_OFFSET_R19,    0x44
.equ    STK_OFFSET_R20,    0x48
.equ    STK_OFFSET_R21,    0x4C
.equ    STK_OFFSET_R22,    0x50
.equ    STK_OFFSET_R23,    0x54
.equ    STK_OFFSET_R24,    0x58
.equ    STK_OFFSET_R25,    0x5C
.equ    STK_OFFSET_R26,    0x60
.equ    STK_OFFSET_R27,    0x64
.equ    STK_OFFSET_R28,    0x68
.equ    STK_OFFSET_R29,    0x6C
.equ    STK_OFFSET_R30,    0x70
.equ    STK_OFFSET_R31,    0x74

.equ    STK_CTX_SIZE,      0x78

.equ    CPU_IE_BIT,        0x02


/*
*********************************************************************************************************
*                                          PUBLIC FUNCTIONS
*********************************************************************************************************
*/

    .globl     OSStartHighRdy
    .globl     OSCtxSw
    .globl     OSIntCtxSw

    .globl     _interrupt_handler
    .globl     OS_CPU_ISR


/*
*********************************************************************************************************
*                                         EXTERNAL FUNCTIONS
*********************************************************************************************************
*/
    .extern    OSIntEnter
    .extern    OSIntExit
    .extern    OS_CPU_IntHandler
    .extern    OSTaskSwHook


/*
*********************************************************************************************************
*                                         EXTERNAL VARIABLES
*********************************************************************************************************
*/
    .extern    OSRunning
    .extern    OSIntNestingCtr
    .extern    OSTCBCur
    .extern    OSTCBHighRdyPtr
    .extern    OSPrioCur
    .extern    OSPrioHighRdy

.text

/*
*********************************************************************************************************
*                                          OSStartHighRdy()
*
* Description: Starts the highest priority task that is available to run.  OSStartHighRdy() MUST:
*
*              a) Call OSTaskSwHook()
*              b) Set OSRunning to TRUE
*              c) Switch to the highest priority task.
*
*              The stack frame of the task to resume is assumed to look as follows:
*
*              OSTCBHighRdyPtr->OSTCBStkPtr + 0x00     RMSR   (IE=1)       (LOW Memory)
*                                           + 0x04     R2
*                                           + 0x08     R3
*                                           + 0x0C     R4
*                                           + 0x10     R5     (p_arg passed to task)
*                                           + 0x14     R6
*                                           + 0x18     R7
*                                           + 0x1C     R8
*                                           + 0x20     R9
*                                           + 0x24     R10
*                                           + 0x28     R11
*                                           + 0x2C     R12
*                                           + 0x30     R13
*                                           + 0x34     R14
*                                           + 0x38     R15
*                                           + 0x3C     R17
*                                           + 0x40     R18
*                                           + 0x44     R19
*                                           + 0x48     R20
*                                           + 0x4C     R21
*                                           + 0x50     R22
*                                           + 0x54     R23
*                                           + 0x58     R24
*                                           + 0x5C     R25
*                                           + 0x60     R26
*                                           + 0x64     R27
*                                           + 0x68     R28
*                                           + 0x6C     R29
*                                           + 0x70     R30
*                                           + 0x74     R31                 (HIGH MEMORY)
*********************************************************************************************************
*/

OSStartHighRdy:

    BRLID   r15, OSTaskSwHook           /* Call OSTaskSwHook()                                         */
    AND     r0,  r0, r0                 /* NO-OP                                                       */

    OR      r3,  r3, r0                 /* OSRunning = TRUE                                            */
    ADDIK   r3,  r0, 1
    SBI     r3,  r0, OSRunning

    LWI     r3,  r0, OSTCBHighRdyPtr    /* SP       = OSTCBHighRdyPtr->OSTCBStkPtr                     */
    LW      r1,  r0, r3

    LWI     r31, r1, STK_OFFSET_R31     /* *************** RESTORE TASK'S CONTEXT ***************      */
    LWI     r30, r1, STK_OFFSET_R30
    LWI     r29, r1, STK_OFFSET_R29
    LWI     r28, r1, STK_OFFSET_R28
    LWI     r27, r1, STK_OFFSET_R27
    LWI     r26, r1, STK_OFFSET_R26
    LWI     r25, r1, STK_OFFSET_R25
    LWI     r24, r1, STK_OFFSET_R24
    LWI     r23, r1, STK_OFFSET_R23
    LWI     r22, r1, STK_OFFSET_R22
    LWI     r21, r1, STK_OFFSET_R21
    LWI     r20, r1, STK_OFFSET_R20
    LWI     r19, r1, STK_OFFSET_R19
    LWI     r18, r1, STK_OFFSET_R18
    LWI     r17, r1, STK_OFFSET_R17
    LWI     r15, r1, STK_OFFSET_R15
    LWI     r14, r1, STK_OFFSET_R14
    LWI     r13, r1, STK_OFFSET_R13
    LWI     r12, r1, STK_OFFSET_R12
    LWI     r11, r1, STK_OFFSET_R11
    LWI     r10, r1, STK_OFFSET_R10
    LWI     r9,  r1, STK_OFFSET_R09
    LWI     r8,  r1, STK_OFFSET_R08
    LWI     r7,  r1, STK_OFFSET_R07
    LWI     r6,  r1, STK_OFFSET_R06
    LWI     r5,  r1, STK_OFFSET_R05
    LWI     r4,  r1, STK_OFFSET_R04
    LWI     r2,  r1, STK_OFFSET_R02

    LWI     r3,  r1, STK_OFFSET_RMSR    /* Get the saved RMSR                                          */
    ANDNI   r3,  r3, CPU_IE_BIT         /* Clear the IE bit (It will be set by the return from INT.)   */
    MTS     RMSR,r3
    LWI     r3,  r1, STK_OFFSET_R03     /* Restore R3 (was used a scratchpad register for RMSR)        */

    ADDIK   r1,  r1, STK_CTX_SIZE       /* Clean up the stack (i.e. de-allocate storage)               */

    RTID    r14, 0                      /* Branch to task level code enabling interrupts, IE=1         */
    AND     r0,  r0, r0                 /* NO-OP                                                       */

/*
*********************************************************************************************************
*                                              OSCtxSw()
*
* Description: Performs the Context switch from a task.  This function is ALWAYS called with interrupts
*              DISABLED.
*
*              OSCtxSw() must implement the following pseudo-code:
*
*                  Save ALL CPU registers;
*                  OSTCBCur->OSTCBStkPtr = SP;
*                  OSTaskSwHook();
*                  OSPrioCur = OSPrioHighRdy;
*                  OSTCBCur  = OSTCBHighRdyPtr;
*                  SP        = OSTCBHighRdyPtr->OSTCBStkPtr;
*                  Restore ALL the CPU registers;
*                  if (IE bit of saved RMSR is 0) {
*                      Return from function call;
*                  } else {
*                      Set IE bit of RMSR to 0;
*                      Return from interrupt whcih sets IE back to 1;
*                  }
*
*
*              The stack frame of the task to suspend will look as follows when OSCtxSw() is done:
*
*              OSTCBCur->OSTCBStkPtr        + 0x00     RMSR   (See Note 1) (LOW Memory)
*                                           + 0x04     R2
*                                           + 0x08     R3
*                                           + 0x0C     R4
*                                           + 0x10     R5     (p_arg passed to task)
*                                           + 0x14     R6
*                                           + 0x18     R7
*                                           + 0x1C     R8
*                                           + 0x20     R9
*                                           + 0x24     R10
*                                           + 0x28     R11
*                                           + 0x2C     R12
*                                           + 0x30     R13
*                                           + 0x34     R14
*                                           + 0x38     R15
*                                           + 0x3C     R17
*                                           + 0x40     R18
*                                           + 0x44     R19
*                                           + 0x48     R20
*                                           + 0x4C     R21
*                                           + 0x50     R22
*                                           + 0x54     R23
*                                           + 0x58     R24
*                                           + 0x5C     R25
*                                           + 0x60     R26
*                                           + 0x64     R27
*                                           + 0x68     R28
*                                           + 0x6C     R29
*                                           + 0x70     R30
*                                           + 0x74     R31                 (HIGH MEMORY)
*
*              The stack frame of the task to resume looks as follows:
*
*              OSTCBHighRdyPtr->OSTCBStkPtr + 0x00     RMSR   (See Note 2) (LOW Memory)
*                                           + 0x04     R2
*                                           + 0x08     R3
*                                           + 0x0C     R4
*                                           + 0x10     R5
*                                           + 0x14     R6
*                                           + 0x18     R7
*                                           + 0x1C     R8
*                                           + 0x20     R9
*                                           + 0x24     R10
*                                           + 0x28     R11
*                                           + 0x2C     R12
*                                           + 0x30     R13
*                                           + 0x34     R14
*                                           + 0x38     R15
*                                           + 0x3C     R17
*                                           + 0x40     R18
*                                           + 0x44     R19
*                                           + 0x48     R20
*                                           + 0x4C     R21
*                                           + 0x50     R22
*                                           + 0x54     R23
*                                           + 0x58     R24
*                                           + 0x5C     R25
*                                           + 0x60     R26
*                                           + 0x64     R27
*                                           + 0x68     R28
*                                           + 0x6C     R29
*                                           + 0x70     R30
*                                           + 0x74     R31                 (HIGH MEMORY)
*
* Note(s)    : 1) OSCtxSw() is ALWAYS called with IE set to 0 (i.e. interrupts disabled).
*              2) If the task frame was saved by OSCtxSw(), IE would be set to 0.
*                 If the task frame was saved by an ISR,    IE would be set to 1.
*********************************************************************************************************
*/

OSCtxSw:
                                        /* *************** SAVE CURRENT TASK'S CONTEXT *************** */
    ADDIK   r1,  r1, -STK_CTX_SIZE      /* Allocate storage for saving registers onto stack            */

    SWI     r2,  r1,  STK_OFFSET_R02    /* Save the remaining registers onto the task's stack          */
    SWI     r3,  r1,  STK_OFFSET_R03
    SWI     r4,  r1,  STK_OFFSET_R04
    SWI     r5,  r1,  STK_OFFSET_R05
    SWI     r6,  r1,  STK_OFFSET_R06
    SWI     r7,  r1,  STK_OFFSET_R07
    SWI     r8,  r1,  STK_OFFSET_R08
    SWI     r9,  r1,  STK_OFFSET_R09
    SWI     r10, r1,  STK_OFFSET_R10
    SWI     r11, r1,  STK_OFFSET_R11
    SWI     r12, r1,  STK_OFFSET_R12
    SWI     r13, r1,  STK_OFFSET_R13
    SWI     r14, r1,  STK_OFFSET_R14
    SWI     r15, r1,  STK_OFFSET_R15
    SWI     r17, r1,  STK_OFFSET_R17
    SWI     r18, r1,  STK_OFFSET_R18
    SWI     r19, r1,  STK_OFFSET_R19
    SWI     r20, r1,  STK_OFFSET_R20
    SWI     r21, r1,  STK_OFFSET_R21
    SWI     r22, r1,  STK_OFFSET_R22
    SWI     r23, r1,  STK_OFFSET_R23
    SWI     r24, r1,  STK_OFFSET_R24
    SWI     r25, r1,  STK_OFFSET_R25
    SWI     r26, r1,  STK_OFFSET_R26
    SWI     r27, r1,  STK_OFFSET_R27
    SWI     r28, r1,  STK_OFFSET_R28
    SWI     r29, r1,  STK_OFFSET_R29
    SWI     r30, r1,  STK_OFFSET_R30
    SWI     r31, r1,  STK_OFFSET_R31

    MFS     r3,  RMSR                   /* save the MSR (See Note 1)                                   */
    SWI     r3,  r1,  STK_OFFSET_RMSR

    LWI     r3,  r0, OSTCBCurPtr        /* OSTCBCur->OSTCBStkPtr = SP                                  */
    SW      r1,  r0, r3

    BRLID   r15, OSTaskSwHook           /* Call OSTaskSwHook()                                         */
    AND     r0,  r0, r0                 /* NO-OP                                                       */

    LBUI    r3,  r0, OSPrioHighRdy      /* OSPrioCur = OSPrioHighRdy                                   */
    SBI     r3,  r0, OSPrioCur

    LWI     r3,  r0, OSTCBHighRdyPtr    /* OSTCBCur  = OSTCBHighRdyPtr                                 */
    SWI     r3,  r0, OSTCBCurPtr

    LW      r1,  r0, r3                 /* SP = OSTCBHighRdyPtr->OSTCBStkPtr                           */

    LWI     r31, r1, STK_OFFSET_R31     /* **************** RESTORE NEW TASK'S CONTEXT *************** */
    LWI     r30, r1, STK_OFFSET_R30
    LWI     r29, r1, STK_OFFSET_R29
    LWI     r28, r1, STK_OFFSET_R28
    LWI     r27, r1, STK_OFFSET_R27
    LWI     r26, r1, STK_OFFSET_R26
    LWI     r25, r1, STK_OFFSET_R25
    LWI     r24, r1, STK_OFFSET_R24
    LWI     r23, r1, STK_OFFSET_R23
    LWI     r22, r1, STK_OFFSET_R22
    LWI     r21, r1, STK_OFFSET_R21
    LWI     r20, r1, STK_OFFSET_R20
    LWI     r19, r1, STK_OFFSET_R19
    LWI     r18, r1, STK_OFFSET_R18
    LWI     r17, r1, STK_OFFSET_R17
    LWI     r15, r1, STK_OFFSET_R15
    LWI     r14, r1, STK_OFFSET_R14
    LWI     r13, r1, STK_OFFSET_R13
    LWI     r12, r1, STK_OFFSET_R12
    LWI     r11, r1, STK_OFFSET_R11
    LWI     r10, r1, STK_OFFSET_R10
    LWI     r9,  r1, STK_OFFSET_R09
    LWI     r8,  r1, STK_OFFSET_R08
    LWI     r7,  r1, STK_OFFSET_R07
    LWI     r6,  r1, STK_OFFSET_R06
    LWI     r5,  r1, STK_OFFSET_R05
    LWI     r4,  r1, STK_OFFSET_R04
    LWI     r2,  r1, STK_OFFSET_R02

    LWI     r3,  r1, STK_OFFSET_RMSR    /* Get the saved RMSR                                          */
    ANDI    r3,  r3, CPU_IE_BIT         /* See if IE is 0 (Saved by OSCtxSw()) or 1 (Saved by ISR)     */
    BNEI    r3,  OSCtxSw_SavedByISR     /* Branch if ISR saved context                                 */

                                        /* *********** The context was saved by OSCtxSw() ************ */
    LWI     r3,  r1, STK_OFFSET_RMSR    /* Get the saved RMSR                                          */
    MTS     RMSR,r3
    LWI     r3,  r1, STK_OFFSET_R03     /* Restore R3 (was used a scratchpad register for RMSR)        */

    ADDIK   r1,  r1, STK_CTX_SIZE       /* Clean up the stack (deallocate storage)                     */

    RTSD    r15, 8                      /* Context was saved by OSCtxSw()                              */
    AND     r0,  r0, r0                 /* NO-OP                                                       */

OSCtxSw_SavedByISR:
                                        /* ************ The context was saved by an ISR ************** */
    LWI     r3,  r1, STK_OFFSET_RMSR    /* Get the saved RMSR                                          */
    ANDNI   r3,  r3, CPU_IE_BIT         /* Clear the IE bit (It will be set by the return from INT.)   */
    MTS     RMSR,r3
    LWI     r3,  r1, STK_OFFSET_R03     /* Restore R3 (was used a scratchpad register for RMSR)        */

    ADDIK   r1,  r1, STK_CTX_SIZE       /* Clean up the stack (deallocate storage)                     */

    RTID    r14, 0                      /* Context was saved by ISR, return address is in R14, Set IE  */
    AND     r0,  r0, r0                 /* NO-OP                                                       */

/*
*********************************************************************************************************
*                                            OSIntCtxSw()
*
* Description: Performs the Context Switch from an ISR.
*
*              OSIntCtxSw() must implement the following pseudo-code:
*
*                  OSTaskSwHook();
*                  OSPrioCur = OSPrioHighRdy;
*                  OSTCBCur  = OSTCBHighRdyPtr;
*                  SP        = OSTCBHighRdyPtr->OSTCBStkPtr;
*                  Restore ALL the CPU registers;
*                  if (IE bit of saved RMSR is 0) {
*                      Return from function call;
*                  } else {
*                      Set IE bit of RMSR to 0;
*                      Return from interrupt;
*                  }
*
*              Upon entry, the registers of the task being suspended have already been saved onto that
*              task's stack and the SP for the task has been saved in its OS_TCB by the ISR.
*
*              The stack frame of the task to resume is assumed to look as follows:
*
*              OSTCBHighRdyPtr->OSTCBStkPtr + 0x00     RMSR   (See Note 1) (LOW Memory)
*                                           + 0x04     R2
*                                           + 0x08     R3
*                                           + 0x0C     R4
*                                           + 0x10     R5
*                                           + 0x14     R6
*                                           + 0x18     R7
*                                           + 0x1C     R8
*                                           + 0x20     R9
*                                           + 0x24     R10
*                                           + 0x28     R11
*                                           + 0x2C     R12
*                                           + 0x30     R13
*                                           + 0x34     R14
*                                           + 0x38     R15
*                                           + 0x3C     R17
*                                           + 0x40     R18
*                                           + 0x44     R19
*                                           + 0x48     R20
*                                           + 0x4C     R21
*                                           + 0x50     R22
*                                           + 0x54     R23
*                                           + 0x58     R24
*                                           + 0x5C     R25
*                                           + 0x60     R26
*                                           + 0x64     R27
*                                           + 0x68     R28
*                                           + 0x6C     R29
*                                           + 0x70     R30
*                                           + 0x74     R31                 (HIGH MEMORY)
*
* Note(s)    : 1) If the task frame was saved by OSCtxSw(), IE would be set to 0.
*                 If the task frame was saved by an ISR,    IE would be set to 1.
*********************************************************************************************************
*/

OSIntCtxSw:

    BRLID   r15, OSTaskSwHook           /* Call OSTaskSwHook()                                         */
    AND     r0,  r0, r0                 /* NO-OP                                                       */

    LBUI    r3,  r0, OSPrioHighRdy      /* OSPrioCur = OSPrioHighRdy                                   */
    SBI     r3,  r0, OSPrioCur

    LWI     r3,  r0, OSTCBHighRdyPtr    /* OSTCBCur  = OSTCBHighRdyPtr                                  */
    SWI     r3,  r0, OSTCBCurPtr

    LW      r1,  r0, r3                 /* SP = OSTCBHighRdyPtr->OSTCBStkPtr                            */

    LWI     r31, r1, STK_OFFSET_R31     /* **************** RESTORE NEW TASK'S CONTEXT *************** */
    LWI     r30, r1, STK_OFFSET_R30
    LWI     r29, r1, STK_OFFSET_R29
    LWI     r28, r1, STK_OFFSET_R28
    LWI     r27, r1, STK_OFFSET_R27
    LWI     r26, r1, STK_OFFSET_R26
    LWI     r25, r1, STK_OFFSET_R25
    LWI     r24, r1, STK_OFFSET_R24
    LWI     r23, r1, STK_OFFSET_R23
    LWI     r22, r1, STK_OFFSET_R22
    LWI     r21, r1, STK_OFFSET_R21
    LWI     r20, r1, STK_OFFSET_R20
    LWI     r19, r1, STK_OFFSET_R19
    LWI     r18, r1, STK_OFFSET_R18
    LWI     r17, r1, STK_OFFSET_R17
    LWI     r15, r1, STK_OFFSET_R15
    LWI     r14, r1, STK_OFFSET_R14
    LWI     r13, r1, STK_OFFSET_R13
    LWI     r12, r1, STK_OFFSET_R12
    LWI     r11, r1, STK_OFFSET_R11
    LWI     r10, r1, STK_OFFSET_R10
    LWI     r9,  r1, STK_OFFSET_R09
    LWI     r8,  r1, STK_OFFSET_R08
    LWI     r7,  r1, STK_OFFSET_R07
    LWI     r6,  r1, STK_OFFSET_R06
    LWI     r5,  r1, STK_OFFSET_R05
    LWI     r4,  r1, STK_OFFSET_R04
    LWI     r2,  r1, STK_OFFSET_R02

    LWI     r3,  r1, STK_OFFSET_RMSR    /* Get the saved RMSR                                          */
    ANDI    r3,  r3, CPU_IE_BIT         /* See if IE is 0 (Saved by OSCtxSw()) or 1 (Saved by ISR)     */
    BNEI    r3,  OSIntCtxSw_SavedByISR  /* Branch if ISR saved context                                 */

                                        /* *********** The context was saved by OSCtxSw() ************ */
    LWI     r3,  r1, STK_OFFSET_RMSR    /* Get the saved RMSR                                          */
    MTS     RMSR,r3
    LWI     r3,  r1, STK_OFFSET_R03     /* Restore R3 (was used a scratchpad register for RMSR)        */

    ADDIK   r1,  r1, STK_CTX_SIZE       /* Clean up the stack (deallocate storage)                     */

    RTSD    r15, 8                      /* Context was saved by OSCtxSw()                              */
    AND     r0,  r0, r0                 /* NO-OP                                                       */

OSIntCtxSw_SavedByISR:                  /* ************ The context was saved by an ISR ************** */
    LWI     r3,  r1, STK_OFFSET_RMSR    /* Get the saved RMSR                                          */
    ANDNI   r3,  r3, CPU_IE_BIT         /* Clear the IE bit (It will be set by the return from INT.)   */
    MTS     RMSR,r3
    LWI     r3,  r1, STK_OFFSET_R03     /* Restore R3 (was used a scratchpad register for RMSR)        */

    ADDIK   r1,  r1, STK_CTX_SIZE       /* Clean up the stack (deallocate storage)                     */

    RTID    r14, 0                      /* Context was saved by ISR, return address is in R14          */
    AND     r0,  r0, r0                 /* NO-OP                                                       */

/*
*********************************************************************************************************
*                                            OS_CPU_ISR()
*
* Description: This routine is intended to be the target of the Interrupt processing functionality that
*              occurs when the MicroBlaze is interrupted. The address, 'XOSExternalInterruptHandler', is
*              used as the branch destination in the code that is executed at addresses 0x10 and 0x14 in
*              the MicroBlaze vector table assuming that the vector table is in RAM
*
*              The XPS interrupt vector is replaced by OS_CPU_ISR() by executing the code from a C function:
*
*                 *(INT32U *)0x00000010 = 0xB0000000 | ((INT32U)OS_CPU_ISR >> 16);
*                 *(INT32U *)0x00000014 = 0xB8080000 | ((INT32U)OS_CPU_ISR & 0x0000FFFF);
*
*              The interrupted task context is saved onto its stack as follows:
*
*              OSTCBCur->OSTCBStkPtr     + 0x00     RMSR   (See Note 1) (LOW Memory)
*                                        + 0x04     R2
*                                        + 0x08     R3
*                                        + 0x0C     R4
*                                        + 0x10     R5     (p_arg passed to task)
*                                        + 0x14     R6
*                                        + 0x18     R7
*                                        + 0x1C     R8
*                                        + 0x20     R9
*                                        + 0x24     R10
*                                        + 0x28     R11
*                                        + 0x2C     R12
*                                        + 0x30     R13
*                                        + 0x34     R14
*                                        + 0x38     R15
*                                        + 0x3C     R17
*                                        + 0x40     R18
*                                        + 0x44     R19
*                                        + 0x48     R20
*                                        + 0x4C     R21
*                                        + 0x50     R22
*                                        + 0x54     R23
*                                        + 0x58     R24
*                                        + 0x5C     R25
*                                        + 0x60     R26
*                                        + 0x64     R27
*                                        + 0x68     R28
*                                        + 0x6C     R29
*                                        + 0x70     R30
*                                        + 0x74     R31                 (HIGH MEMORY)
*
* Note(s)    : 1) The IE bit is saved onto the stack 'set' since the code must return to the interrupted
*                 task with interrupts enabled.
*********************************************************************************************************
*/

_interrupt_handler:
OS_CPU_ISR:
                                             /* ********** SAVE INTERRUPTED TASK'S CONTEXT *********** */
    ADDIK   r1,  r1, -STK_CTX_SIZE           /* Allocate storage for saving registers onto stack       */

    SWI     r2,  r1,  STK_OFFSET_R02
    SWI     r3,  r1,  STK_OFFSET_R03
    SWI     r4,  r1,  STK_OFFSET_R04
    SWI     r5,  r1,  STK_OFFSET_R05
    SWI     r6,  r1,  STK_OFFSET_R06
    SWI     r7,  r1,  STK_OFFSET_R07
    SWI     r8,  r1,  STK_OFFSET_R08
    SWI     r9,  r1,  STK_OFFSET_R09
    SWI     r10, r1,  STK_OFFSET_R10
    SWI     r11, r1,  STK_OFFSET_R11
    SWI     r12, r1,  STK_OFFSET_R12
    SWI     r13, r1,  STK_OFFSET_R13
    SWI     r14, r1,  STK_OFFSET_R14
    SWI     r15, r1,  STK_OFFSET_R15
    SWI     r17, r1,  STK_OFFSET_R17
    SWI     r18, r1,  STK_OFFSET_R18
    SWI     r19, r1,  STK_OFFSET_R19
    SWI     r20, r1,  STK_OFFSET_R20
    SWI     r21, r1,  STK_OFFSET_R21
    SWI     r22, r1,  STK_OFFSET_R22
    SWI     r23, r1,  STK_OFFSET_R23
    SWI     r24, r1,  STK_OFFSET_R24
    SWI     r25, r1,  STK_OFFSET_R25
    SWI     r26, r1,  STK_OFFSET_R26
    SWI     r27, r1,  STK_OFFSET_R27
    SWI     r28, r1,  STK_OFFSET_R28
    SWI     r29, r1,  STK_OFFSET_R29
    SWI     r30, r1,  STK_OFFSET_R30
    SWI     r31, r1,  STK_OFFSET_R31

    MFS     r3,  RMSR                        /* save the MSR                                           */
    ORI     r3,  r3, CPU_IE_BIT              /* Set IE to 1 to return to interrupted task with INT en. */
    SWI     r3,  r1, STK_OFFSET_RMSR         /* MSR is at top of frame                                 */

    LBUI    r3,  r0, OSIntNestingCtr         /* if (OSIntNestingCtr == 0) {                            */
    BNEI    r3,  OS_CPU_ISR_1

    LWI     r3,  r0, OSTCBCurPtr             /*     OSTCBCur->OSTCBStkPtr = SP                         */
    SW      r1,  r0, r3                      /* }                                                      */

OS_CPU_ISR_1:
    LBUI    r3,  r0, OSIntNestingCtr
    ADDIK   r3,  r3, 1                       /* OSIntNestingCtr+;                                      */
    SBI     r3,  r0, OSIntNestingCtr

    BRLID   r15, BSP_IntHandler              /* Call the provided C level interrupt handler            */
    AND     r0,  r0, r0                      /* NO-OP                                                  */

    BRLID   r15, OSIntExit                   /* OSIntExit()                                            */
    AND     r0,  r0, r0                      /* NO-OP                                                  */

    LWI     r31, r1, STK_OFFSET_R31          /* ********* RESTORE INTERRUPTED TASK'S CONTEXT ********* */
    LWI     r30, r1, STK_OFFSET_R30
    LWI     r29, r1, STK_OFFSET_R29
    LWI     r28, r1, STK_OFFSET_R28
    LWI     r27, r1, STK_OFFSET_R27
    LWI     r26, r1, STK_OFFSET_R26
    LWI     r25, r1, STK_OFFSET_R25
    LWI     r24, r1, STK_OFFSET_R24
    LWI     r23, r1, STK_OFFSET_R23
    LWI     r22, r1, STK_OFFSET_R22
    LWI     r21, r1, STK_OFFSET_R21
    LWI     r20, r1, STK_OFFSET_R20
    LWI     r19, r1, STK_OFFSET_R19
    LWI     r18, r1, STK_OFFSET_R18
    LWI     r17, r1, STK_OFFSET_R17
    LWI     r15, r1, STK_OFFSET_R15
    LWI     r14, r1, STK_OFFSET_R14
    LWI     r13, r1, STK_OFFSET_R13
    LWI     r12, r1, STK_OFFSET_R12
    LWI     r11, r1, STK_OFFSET_R11
    LWI     r10, r1, STK_OFFSET_R10
    LWI     r9,  r1, STK_OFFSET_R09
    LWI     r8,  r1, STK_OFFSET_R08
    LWI     r7,  r1, STK_OFFSET_R07
    LWI     r6,  r1, STK_OFFSET_R06
    LWI     r5,  r1, STK_OFFSET_R05
    LWI     r4,  r1, STK_OFFSET_R04
    LWI     r2,  r1, STK_OFFSET_R02

    LWI     r3,  r1, STK_OFFSET_RMSR         /* Get RMSR                                               */
    ANDNI   r3,  r3, CPU_IE_BIT              /* Clear IE to prevent interrupts until stack is cleaned  */
    MTS     RMSR,r3

    LWI     r3,  r1, STK_OFFSET_R03          /* Restore R3 (was used a scratchpad register for RMSR)   */

    ADDIK   r1,  r1, STK_CTX_SIZE            /* Clean up the stack                                     */

    RTID    r14, 0                           /* Return from interrupt with interrupts enabled          */
    AND     r0,  r0, r0                      /* NO-OP                                                  */
