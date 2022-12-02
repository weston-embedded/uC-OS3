/*
*********************************************************************************************************
*                                              uC/OS-III
*                                        The Real-Time Kernel
*
*                    Copyright 2009-2022 Silicon Laboratories Inc. www.silabs.com
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
* Version : V3.08.02
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

        PUBLIC  OSCtxSw
        PUBLIC  OSIntCtxSw
        PUBLIC  OSStartHighRdy

        PUBLIC  OS_My_ISR

/*
*********************************************************************************************************
*                                        EXTERNAL DECLARATIONS
*********************************************************************************************************
*/

        EXTERN  OSRunning
        EXTERN  OSIntExit
        EXTERN  OSIntNestingCtr
        EXTERN  OSLockNesting
        EXTERN  OSPrioCur
        EXTERN  OSPrioHighRdy
        EXTERN  OSRdyGrp
        EXTERN  OSRdyTbl
        EXTERN  OSTaskSwHook
        EXTERN  OSTCBCurPtr
        EXTERN  OSTCBHighRdyPtr
        EXTERN  OSIntEnter

        RSEG    CODE:CODE:NOROOT(2)                 /* Align to power 2, 4 bytes.                     */


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

OSStartHighRdy:
       JSR       (OSTaskSwHook)             /* Invoke user defined context switch hook        */

       MOVEQ.L    #1, D4                    /* OSRunning = TRUE;                              */
       MOVE.B     D4, (OSRunning)           /* Indicates that we are multitasking             */

       MOVEA.L   (OSTCBHighRdyPtr), A1      /* Point to TCB of highest prio task ready to run */
       MOVEA.L   (A1), A7                   /* Get the stack pointer of the task to resume    */

       MOVEM.L   (A7), D0-D7/A0-A6          /* Store all the regs                             */
       LEA       (60, A7), A7               /* Advance the stack pointer                      */

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

OSCtxSw:
       LEA       (-60, A7), A7
       MOVEM.L    D0-D7/A0-A6, (A7)         /* Save the registers of the current task         */

       MOVEA.L   (OSTCBCurPtr), A1          /* Save stack pointer in the suspended task TCB   */
       MOVE.L     A7, (A1)

       JSR       (OSTaskSwHook)             /* Invoke user defined context switch hook        */

       MOVEA.L   (OSTCBHighRdyPtr), A1      /* OSTCBCurPtr = OSTCBHighRdyPtr                  */
       MOVE.L     A1, (OSTCBCurPtr)
       MOVEA.L   (A1), A7                   /* Get the stack pointer of the task to resume    */

       MOVE.B    (OSPrioHighRdy), D0        /* OSPrioCur = OSPrioHighRdy                      */
       MOVE.B     D0, (OSPrioCur)

       MOVEM.L   (A7), D0-D7/A0-A6          /* Restore the CPU registers                      */
       LEA       (60, A7), A7

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

OSIntCtxSw:
      JSR        (OSTaskSwHook)             /* Invoke user defined context switch hook        */

      MOVE.B     (OSPrioHighRdy), D0        /* OSPrioCur   = OSPrioHighRdy                    */
      MOVE.B      D0, (OSPrioCur)

      MOVEA.L    (OSTCBHighRdyPtr), A1      /* OSTCBCurPtr = OSTCBHighRdyPtr                  */
      MOVE.L      A1, (OSTCBCurPtr)
      MOVEA.L    (A1), A7                   /* SP          = OSTCBHighRdyPtr->OSTCBStkPtr     */

      MOVEM.L    (A7), D0-D7/A0-A6          /* Restore ALL CPU registers from new task stack  */
      LEA        (60, A7), A7

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

OS_My_ISR:
      MOVE.W     #0x2700, SR                /* Disable interrupts                             */

      LEA        (-60, A7), A7              /* Save processor registers onto stack            */
      MOVEM.L     D0-D7/A0-A6, (A7)

      OS_EMAC_SAVE                          /* Save the EMAC registers                        */

      MOVEQ.L    #0, D0                     /* OSIntNestingCtr++                              */
      MOVE.B     (OSIntNestingCtr), D0
      ADDQ.L     #1, D0
      MOVE.B      D0, (OSIntNestingCtr)

      CMPI.L     #1, D0                     /* if (OSIntNestingCtr == 1)                      */
      BNE.W      (OS_My_ISR_1)
      MOVEA.L    (OSTCBCurPtr), A1          /* OSTCBCurPtr-<OSTCBStkPtr = SP                  */
      MOVE.L      A7, (A1)

OS_My_ISR_1:

      JSR        (OS_My_ISR_Handler)        /* OS_My_ISR_Handler()                            */

      JSR        (OSIntExit)                /* Exit the ISR                                   */

      OS_EMAC_RESTORE                       /* Restore the EMAC registers                     */

      MOVEM.L    (A7), D0-D7/A0-A6          /* Restore processor registers from stack         */
      LEA        (60, A7), A7

      RTE                                   /* Return to task or nested ISR                   */


OS_My_ISR_Handler:
      RTS

      END
