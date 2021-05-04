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
*                                         Generic ColdFire Port
*
* File    : os_cpu_a.asm
* Version : V3.08.01
*********************************************************************************************************
* Note(s) : 1) This port uses the MOVEM.L (A7),D0-D7/A0-A6, LEA 60(A7)A7 construct instead of
*              the traditional 68xxx MOVEM.L (A7)+,D0-D7/A0-A6.  It is perfectly in order to
*              push/pop individual registers using MOVEM.L (A7)+,D2, etc. but it's a bit slower
*              (and more verbose).
*
*           2) The LEA instruction is required because the ColdFire cannot push multiple
*              registers directly to the stack.
*********************************************************************************************************
*/

#include  <os_cpu_i.asm>

/*
*********************************************************************************************************
*                                         PUBLIC DECLARATIONS
*********************************************************************************************************
*/

        .global  _OSCtxSw
        .global  _OSIntCtxSw
        .global  _OSStartHighRdy

        .global  _OS_My_ISR

/*
*********************************************************************************************************
*                                        EXTERNAL DECLARATIONS
*********************************************************************************************************
*/

        .extern  _OSRunning
        .extern  _OSIntExit
        .extern  _OSIntNestingCtr
        .extern  _OSLockNesting
        .extern  _OSPrioCur
        .extern  _OSPrioHighRdy
        .extern  _OSRdyGrp
        .extern  _OSRdyTbl
        .extern  _OSTaskSwHook
        .extern  _OSTCBCurPtr
        .extern  _OSTCBHighRdyPtr
        .extern  _OSIntEnter

        .text
        .align 4


/*
*********************************************************************************************************
*                              START HIGHEST PRIORITY TASK READY-TO-RUN
*
* Description: This function is called by OSStart() to start the highest priority task that
*              was created by your application before calling OSStart().
*
* Arguments  : None.
*
* Note(s)    : 1) The stack frame is assumed to look as follows:
*
*                 OSTCBHighRdyPtr->OSTCBStkPtr +  0  ---->  D0         Low Memory
*                                              +  4         D1
*                                              +  8         D2
*                                              + 12         D3
*                                              + 16         D4
*                                              + 20         D5
*                                              + 24         D6
*                                              + 28         D7
*                                              + 32         A0
*                                              + 36         A1
*                                              + 40         A2
*                                              + 44         A3
*                                              + 48         A4
*                                              + 52         A5
*                                              + 56         A6
*                                              + 60         Format, Vector, OS_INITIAL_SR
*                                              + 64         task
*                                              + 68         task
*                                              + 72         p_arg       High Memory
*
*              2) OSStartHighRdy() MUST:
*                    a) Call OSTaskSwHook() then,
*                    b) Set OSRunning to TRUE,
*                    c) Switch to the highest priority task.
*********************************************************************************************************
*/

_OSStartHighRdy:
       JSR        _OSTaskSwHook             /* Invoke user defined context switch hook        */

       MOVEQ.L   #1, D4                     /* OSRunning = TRUE;                              */
       MOVE.B     D4, _OSRunning            /* Indicates that we are multitasking             */

       MOVE.L    (_OSTCBHighRdyPtr), A1     /* Point to TCB of highest prio task ready to run */
       MOVE.L    (A1),A7                    /* Get the stack pointer of the task to resume    */

       MOVEM.L   (A7), D0-D7/A0-A6          /* Store all the regs                             */
       LEA        60(A7), A7                /* Advance the stack pointer                      */

       RTE                                  /* Return to task                                 */


/*
*********************************************************************************************************
*                                      TASK LEVEL CONTEXT SWITCH
*
* Description: This function is called when a task makes a higher prio task ready-to-run.
*
* Arguments  : None.
*
* Note(s)    : 1) Upon entry,
*                  OSTCBCurPtr     points to the OS_TCB of the task to suspend
*                  OSTCBHighRdyPtr points to the OS_TCB of the task to resume
*
*              2) The stack frame of the task to suspend looks as follows (the registers
*                 for task to suspend need to be saved):
*
*                                           SP +  0  ---->  Format, Vector, SR   Low Memory
*                                              +  4         PC of task           High Memory
*
*              3) The stack frame of the task to resume looks as follows:
*
*                 OSTCBHighRdyPtr->OSTCBStkPtr +  0  ---->  D0                   Low Memory
*                                              +  4         D1
*                                              +  8         D2
*                                              + 12         D3
*                                              + 16         D4
*                                              + 20         D5
*                                              + 24         D6
*                                              + 28         D7
*                                              + 32         A0
*                                              + 36         A1
*                                              + 40         A2
*                                              + 44         A3
*                                              + 48         A4
*                                              + 52         A5
*                                              + 56         A6
*                                              + 60         SR of task
*                                              + 64         PC of task           High Memory
*********************************************************************************************************
*/

_OSCtxSw:
       LEA       -60(A7), A7
       MOVEM.L    D0-D7/A0-A6, (A7)         /* Save the registers of the current task         */

       MOVE.L    (_OSTCBCurPtr), A1         /* Save stack pointer in the suspended task TCB   */
       MOVE.L     A7, (A1)

       JSR        _OSTaskSwHook             /* Invoke user defined context switch hook        */

       MOVE.L    (_OSTCBHighRdyPtr), A1     /* OSTCBCurPtr = OSTCBHighRdyPtr                  */
       MOVE.L     A1, (_OSTCBCurPtr)
       MOVE.L    (A1), A7                   /* Get the stack pointer of the task to resume    */

       MOVE.B    (_OSPrioHighRdy), D0       /* OSPrioCur = OSPrioHighRdy                      */
       MOVE.B     D0, (_OSPrioCur)

       MOVEM.L   (A7), D0-D7/A0-A6          /* Restore the CPU registers                      */
       LEA        60(A7), A7

       RTE                                  /* Run task                                       */


/*
*********************************************************************************************************
*                                   INTERRUPT LEVEL CONTEXT SWITCH
*
*
* Description: This function is provided for backward compatibility and to satisfy OSIntExit() in
*              OS_CORE.C.
*
* Arguments  : None.
*********************************************************************************************************
*/

_OSIntCtxSw:
      JSR         _OSTaskSwHook             /* Invoke user defined context switch hook        */

      MOVE.B     (_OSPrioHighRdy), D0       /* OSPrioCur   = OSPrioHighRdy                    */
      MOVE.B      D0, (_OSPrioCur)

      MOVE.L     (_OSTCBHighRdyPtr), A1     /* OSTCBCurPtr = OSTCBHighRdyPtr                  */
      MOVE.L      A1, (_OSTCBCurPtr)
      MOVE.L     (A1), A7                   /* SP          = OSTCBHighRdyPtr->OSTCBStkPtr     */

      MOVEM.L    (A7), D0-D7/A0-A6          /* Restore ALL CPU registers from new task stack  */
      LEA         60(A7), A7

      RTE                                   /* Run task                                       */


/*
*********************************************************************************************************
*                                             GENERIC ISR
*
* Description: This function is a template ISR.
*
* Arguments  : None.
*
* Note(s)    : 1) You MUST save ALL the CPU registers as shown below
*
*              2) You MUST increment 'OSIntNestingCtr' and NOT call OSIntEnter()
*
*              3) You MUST use OSIntExit() to exit an ISR.
*********************************************************************************************************
*/

_OS_My_ISR:
      MOVE.W     #0x2700, SR                /* Disable interrupts                             */

      LEA        -60(A7), A7                /* Save processor registers onto stack            */
      MOVEM.L     D0-D7/A0-A6, (A7)

      OS_EMAC_SAVE                          /* Save the EMAC registers                        */

      MOVEQ.L    #0, D0                     /* OSIntNestingCtr++                              */
      MOVE.B     (_OSIntNestingCtr), D0
      ADDQ.L     #1, D0
      MOVE.B      D0, (_OSIntNestingCtr)

      CMPI.L     #1, D0                     /* if (OSIntNestingCtr == 1)                      */
      BNE         _OS_My_ISR_1
      MOVE.L     (_OSTCBCurPtr), A1         /* OSTCBCurPtr-<OSTCBStkPtr = SP                  */
      MOVE.L      A7, (A1)

_OS_My_ISR_1:

      JSR         _OS_My_ISR_Handler        /* OS_My_ISR_Handler()                            */

      JSR         _OSIntExit                /* Exit the ISR                                   */

      OS_EMAC_RESTORE                       /* Restore the EMAC registers                     */

      MOVEM.L    (A7), D0-D7/A0-A6          /* Restore processor registers from stack         */
      LEA         60(A7), A7

      RTE                                   /* Return to task or nested ISR                   */


_OS_My_ISR_Handler:
      RTS

      .end
