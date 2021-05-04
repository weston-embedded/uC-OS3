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
*                                        ATMEL AVR32 AP7000 Port
*
* File      : os_cpu_a.asm
* Version   : V3.08.01
*********************************************************************************************************
* Toolchain : IAR AVR32
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             ASM HEADER
*********************************************************************************************************
*/

        MODULE  OS_CPU_A

        RSEG    CODE32:CODE


;$PAGE
/*
*********************************************************************************************************
*                                               DEFINES
*********************************************************************************************************
*/

OS_CPU_SR_OFFSET           EQU       0                      /* Status  Register offset in System Register             */
OS_CPU_SR_GM_OFFSET        EQU      16                      /* Status  Register, Global Interrupt Mask Offset         */
OS_CPU_SR_MX_OFFSET        EQU      22                      /* Status  Register, Execution Mode Mask offset           */
OS_CPU_RSR_SUP_OFFSET      EQU      20                      /* Status  Register, to return on Supervisor Context      */
OS_CPU_RAR_SUP_OFFSET      EQU      52                      /* Address Register, to return on Supervisor Context      */
OS_CPU_RSR_INT0_OFFSET     EQU      24                      /* Status  Register, to return on INT0       Context      */
OS_CPU_RAR_INT0_OFFSET     EQU      56                      /* Address Register, to return on INT0       Context      */
OS_CPU_RSR_INT1_OFFSET     EQU      28                      /* Status  Register, to return on INT1       Context      */
OS_CPU_RAR_INT1_OFFSET     EQU      60                      /* Address Register, to return on INT1       Context      */
OS_CPU_RSR_INT2_OFFSET     EQU      32                      /* Status  Register, to return on INT2       Context      */
OS_CPU_RAR_INT2_OFFSET     EQU      64                      /* Address Register, to return on INT2       Context      */
OS_CPU_RSR_INT3_OFFSET     EQU      36                      /* Status  Register, to return on INT3       Context      */
OS_CPU_RAR_INT3_OFFSET     EQU      68                      /* Address Register, to return on INT3       Context      */


/*
*********************************************************************************************************
*                                         PUBLIC DECLARATIONS
*********************************************************************************************************
*/

        PUBLIC  OSCtxSw
        PUBLIC  OSCtxRestore
        PUBLIC  OSIntCtxRestore
        PUBLIC  OSInt0ISRHandler
        PUBLIC  OSInt1ISRHandler
        PUBLIC  OSInt2ISRHandler
        PUBLIC  OSInt3ISRHandler

        EXTERN  OSTCBCurPtr
        EXTERN  OSTCBHighRdyPtr
        EXTERN  OSTaskSwHook
        EXTERN  OSPrioHighRdy
        EXTERN  OSPrioCur
        EXTERN  OSIntNestingCtr
        EXTERN  OSIntExit
        EXTERN  BSP_INTC_IntGetHandler


;$PAGE
/*
*********************************************************************************************************
*                                      TASK LEVEL CONTEXT SWITCH
*
* Description: This function is called when a task makes a higher priority task ready-to-run.
*
* Arguments  : None.
*
* Note(s)    : 1) Upon entry,
*                 OSTCBCurPtr     points to the OS_TCB of the task to suspend
*                 OSTCBHighRdyPtr points to the OS_TCB of the task to resume
*
*              2) The saved context of the task to resume looks as follows:
*
*                 OSTCBHighRdyPtr->OSTCBStkPtr --> SP   -->
*                                                  SR   ----/   (Low memory)
*                                                  PC
*                                                  R12
*                                                  .
*                                                  .
*                                                  R0
*                                                  LR   ----\   (High memory)
*
*                 where the stack pointer points to the task start address.
*
*              3) OSCtxSw() has to save all registers from the suspended task. PC and SR are retrieved from
*                 RAR_SUP and RSR_SUP. Only then the Stack Pointer of the task to suspend is saved on OSTCBCurPtr.
*
*              4) OSCtxSw() MUST:
*                      a) Save processor registers then,
*                      b) Save current task's stack pointer into the current task's OS_TCB,
*                      c) Call OSTaskSwHook(),
*                      d) Set OSTCBCurPtr = OSTCBHighRdyPtr,
*                      e) Set OSPrioCur   = OSPrioHighRdy,
*                      f) Switch to the highest priority task.
*
*                      pseudo-code:
*                           void  OSCtxSw (void)
*                           {
*                               Save processor registers;
*
*                               OSTCBCurPtr->OSTCBStkPtr = SP;
*
*                               OSTaskSwHook();
*
*                               OSTCBCurPtr = OSTCBHighRdyPtr;
*                               OSPrioCur   = OSPrioHighRdy;
*
*                               OSCtxRestore(OSTCBHighRdyPtr->OSTCBStkPtr);
*                           }
*********************************************************************************************************
*/

OSCtxSw:
        STMTS   --SP, LR                                        /* Save LR into stack                                       */
        STMTS   --SP, R0-R12                                    /* Save R0-R12 into stack                                   */

        MFSR    R9, OS_CPU_RSR_SUP_OFFSET                       /* Retrieve SR from previous Context                        */
        MFSR    R8, OS_CPU_RAR_SUP_OFFSET                       /* Retrieve PC from previous Context                        */
        PUSHM   R8-R9                                           /* Save PC, SR into stack                                   */

        MOV     R8, LWRD(OSTCBCurPtr)
        ORH     R8, HWRD(OSTCBCurPtr)                           /*  OSTCBCurPtr                                             */
        LD.W    R9, R8[0]                                       /* *OSTCBCurPtr                                             */
        ST.W    R9[0], SP                                       /*  OSTCBCurPtr->OSTCBStkPtr = SP                           */

        RCALL   OSTaskSwHook

        MOV     R12, LWRD(OSTCBHighRdyPtr)
        ORH     R12, HWRD(OSTCBHighRdyPtr)                      /*  OSTCBHighRdyPtr                                         */
        LD.W    R10, R12[0]                                     /* *OSTCBHighRdyPtr                                         */
        MOV     R8,  LWRD(OSTCBCurPtr)
        ORH     R8,  HWRD(OSTCBCurPtr)                          /*  OSTCBCurPtr                                             */
        ST.W    R8[0], R10                                      /*  OSTCBCurPtr = OSTCBHighRdyPtr                           */

        MOV     R12, LWRD(OSPrioHighRdy)
        ORH     R12, HWRD(OSPrioHighRdy)
        LD.UB   R11, R12[0]                                     /* *OSPrioHighRdy                                           */
        MOV     R12, LWRD(OSPrioCur)
        ORH     R12, HWRD(OSPrioCur)
        ST.B    R12[0], R11                                     /*  OSPrioCur = OSPrioHighRdy                               */

        LD.W    R12, R10[0]                                     /* Retrieve OSTCBHighRdyPtr->OSTCBStkPtr                    */

        LDM     R12, R0-R1                                      /* Retrieve PC and SR from stack frame                      */
        MTSR    OS_CPU_RSR_SUP_OFFSET, R1                       /* Store SR to return from Supervisor Context               */
        MTSR    OS_CPU_RAR_SUP_OFFSET, R0                       /* Store PC to return from Supervisor Context               */
        SUB     SP, R12, -2*4                                   /* Restore Stack Pointer                                    */

        LDMTS   SP++, R0-R12                                    /* Restore R0-R12                                           */
        LDMTS   SP++, LR                                        /* Restore LR                                               */
        RETS                                                    /* Restore PC and SR {restore task}                         */


;$PAGE
/*
*********************************************************************************************************
*                                      RESTORE CONTEXT FUNCTIONS
*
* Description: These functions are used to perform context switch.
*
*              void  OSCtxRestore (CPU_STK  *p_stk)
*                     Restore Stack Pointer
*                     Restore SR and PC to Supervisor context
*                     Restore R0-R12, LR
*                     Return from Supervisor Call       {context switch}
*
*              void  OSIntCtxRestore (CPU_STK  *p_stk)
*                     Restore Stack Pointer
*                     Restore SR and PC to proper INTX context
*                     Restore R0-R12, LR
*                     Return from INT                   {context switch}
*
* Arguments  : p_stk        Stack address for the context to be restored
*********************************************************************************************************
*/

OSCtxRestore:
                                                                /* --- Context Restore Code                                 */
        MOV     SP, R12                                         /* Restore SP (Stack Pointer)                               */

        LDM     SP++, R0-R1                                     /* Retrieve PC and SR from stack frame                      */
        MTSR    OS_CPU_RSR_SUP_OFFSET, R1                       /* Store SR to return from Supervisor Context               */
        MTSR    OS_CPU_RAR_SUP_OFFSET, R0                       /* Store PC to return from Supervisor Context               */
        LDMTS   SP++, R0-R12                                    /* Restore R0-R12                                           */
        LDMTS   SP++, LR                                        /* Restore LR                                               */
        RETS                                                    /* Restore PC and SR {restore task}                         */


OSIntCtxRestore:
        MOV     SP, R12                                         /* Restore SP (Stack Pointer)                               */
        LDM     SP++, R0-R1                                     /* Retrieve PC and SR from stack frame                      */

        MFSR    R11, OS_CPU_SR_OFFSET                           /* Retrieve current SR                                      */
        BFEXTU  R11, R11, OS_CPU_SR_MX_OFFSET, 3                /* Retrieve current execution mode                          */

        CP.W    R11, 5                                          /* check execution mode                                     */
        BREQ    OSIntCtxRestore_INT3_Restore                    /* if (R11 == INT3) then goto INT3                          */
        CP.W    R11, 4                                          /* check execution mode                                     */
        BREQ    OSIntCtxRestore_INT2_Restore                    /* if (R11 == INT2) then goto INT2                          */
        CP.W    R11, 3                                          /* check execution mode                                     */
        BREQ    OSIntCtxRestore_INT1_Restore                    /* if (R11 == INT1) then goto INT1                          */
        CP.W    R11, 2                                          /* check execution mode                                     */
        BREQ    OSIntCtxRestore_INT0_Restore                    /* if (R11 == INT0) then goto INT0                          */
        BRAL    OSIntCtxRestore_Err__Restore                    /* goto Err if no INT context was detected                  */

OSIntCtxRestore_INT0_Restore:
        MTSR    OS_CPU_RSR_INT0_OFFSET, R1                      /* Store SR to return from INT0 Context                     */
        MTSR    OS_CPU_RAR_INT0_OFFSET, R0                      /* Store PC to return from INT0 Context                     */
        LDMTS   SP++, R0-R12                                    /* Restore R0-R12 from stack                                */
        LDMTS   SP++, LR                                        /* Restore LR from stack                                    */
        RETE                                                    /* Return from event handler                                */
OSIntCtxRestore_INT1_Restore:
        MTSR    OS_CPU_RSR_INT1_OFFSET, R1                      /* Store SR to return from INT1 Context                     */
        MTSR    OS_CPU_RAR_INT1_OFFSET, R0                      /* Store PC to return from INT1 Context                     */
        LDMTS   SP++, R0-R12                                    /* Restore R0-R12 from stack                                */
        LDMTS   SP++, LR                                        /* Restore LR from stack                                    */
        RETE                                                    /* Return from event handler                                */
OSIntCtxRestore_INT2_Restore:
        MTSR    OS_CPU_RSR_INT2_OFFSET, R1                      /* Store SR to return from INT2 Context                     */
        MTSR    OS_CPU_RAR_INT2_OFFSET, R0                      /* Store PC to return from INT2 Context                     */
        LDMTS   SP++, R0-R12                                    /* Restore R0-R12 from stack                                */
        LDMTS   SP++, LR                                        /* Restore LR from stack                                    */
        RETE                                                    /* Return from event handler                                */
OSIntCtxRestore_INT3_Restore:
        MTSR    OS_CPU_RSR_INT3_OFFSET, R1                      /* Store SR to return from INT3 Context                     */
        MTSR    OS_CPU_RAR_INT3_OFFSET, R0                      /* Store PC to return from INT3 Context                     */
        LDMTS   SP++, R0-R12                                    /* Restore R0-R12 from stack                                */
        LDMTS   SP++, LR                                        /* Restore LR from stack                                    */
        RETE                                                    /* Return from event handler                                */

OSIntCtxRestore_Err__Restore:                                   /* Error detected (hold execution)                          */
        BREAKPOINT


;$PAGE
/*
*********************************************************************************************************
*                                        OS INTERRUPT HANDLER
*
* Description: This function handles the OS specific steps to wrap an user-defined function to be called
*              on an interrupt.
*
*                   prototype:  void  OSIntXISRHandler(CPU_FNCT_VOID p_usr_isr);
*
* Notes      : 1) OSIntXISRHandler() MUST:
*                   a) disable interrupts,
*                   b) save registers (R0-R12,LR,PC,SR),
*                   c) increment OSIntNestingCtr,
*                   d) if (OSIntNestingCtr == 1) OSTCBCurPtr->OSTCBStkPtr = SP,
*                   e) call user-defined function,
*                   f) call OSIntExit(),
*                   g) restore registers (R0-R12,LR,PC,SR),
*                   h) return from interrupt.
*********************************************************************************************************
*/

OSInt0ISRHandler:
                                                                /* --- Save stack frame                                     */
        MFSR    R9, OS_CPU_RSR_INT0_OFFSET                      /* Retrieve SR from INT0 Context                            */
        MFSR    R8, OS_CPU_RAR_INT0_OFFSET                      /* Retrieve PC from INT0 Context                            */
        PUSHM   R8-R9                                           /* Save PC, SR into stack                                   */

        MOV     R11, LWRD(OSIntNestingCtr)
        ORH     R11, HWRD(OSIntNestingCtr)
        LD.UB   R10, R11[0]
        SUB     R10, -1
        ST.B    R11[0], R10                                     /* OSIntNestingCtr++                                        */

        CP.W    R10, 1                                          /* Test OSIntNestingCtr                                     */
        BRNE    OSInt0ISRHandler_1

        MOV     R11, LWRD(OSTCBCurPtr)                          /* if (OSIntNestingCtr == 1) {                              */
        ORH     R11, HWRD(OSTCBCurPtr)
        LD.W    R10, R11[0]
        ST.W    R10[0], SP                                      /*     OSTCBCurPtr->OSTCBStkPtr = SP;                       */
                                                                /* }                                                        */
OSInt0ISRHandler_1:
        ICALL   R12                                             /* call user ISR function (address stored on R12)           */
        RCALL   OSIntExit
                                                                /* --- Restore stack frame                                  */
        LDM     SP++, R0-R1                                     /* Retrieve PC and SR from stack frame                      */
        LDMTS   SP++, R0-R12                                    /* Restore R0-R12 from stack                                */
        LDMTS   SP++, LR                                        /* Restore LR from stack                                    */
        RETE                                                    /* Return from event handler                                */


;$PAGE
OSInt1ISRHandler:
                                                                /* --- Save stack frame                                     */
        MFSR    R9, OS_CPU_RSR_INT1_OFFSET                      /* Retrieve SR from INT1 Context                            */
        MFSR    R8, OS_CPU_RAR_INT1_OFFSET                      /* Retrieve PC from INT1 Context                            */
        PUSHM   R8-R9                                           /* Save PC, SR into stack                                   */

        MOV     R11, LWRD(OSIntNestingCtr)
        ORH     R11, HWRD(OSIntNestingCtr)
        LD.UB   R10, R11[0]
        SUB     R10, -1
        ST.B    R11[0], R10                                     /* OSIntNestingCtr++                                        */

        CP.W    R10, 1                                          /* Test OSIntNestingCtr                                     */
        BRNE    OSInt1ISRHandler_1

        MOV     R11, LWRD(OSTCBCurPtr)                          /* if (OSIntNestingCtr == 1) {                              */
        ORH     R11, HWRD(OSTCBCurPtr)
        LD.W    R10, R11[0]
        ST.W    R10[0], SP                                      /*     OSTCBCurPtr->OSTCBStkPtr = SP;                       */
                                                                /* }                                                        */
OSInt1ISRHandler_1:
        ICALL   R12                                             /* call user ISR function (address stored on R12)           */
        RCALL   OSIntExit
                                                                /* --- Restore stack frame                                  */
        LDM     SP++, R0-R1                                     /* Retrieve PC and SR from stack frame                      */
        LDMTS   SP++, R0-R12                                    /* Restore R0-R12 from stack                                */
        LDMTS   SP++, LR                                        /* Restore LR from stack                                    */
        RETE                                                    /* Return from event handler                                */


;$PAGE
OSInt2ISRHandler:
                                                                /* --- Save stack frame                                     */
        MFSR    R9, OS_CPU_RSR_INT2_OFFSET                      /* Retrieve SR from INT2 Context                            */
        MFSR    R8, OS_CPU_RAR_INT2_OFFSET                      /* Retrieve PC from INT2 Context                            */
        PUSHM   R8-R9                                           /* Save PC, SR into stack                                   */

        MOV     R11, LWRD(OSIntNestingCtr)
        ORH     R11, HWRD(OSIntNestingCtr)
        LD.UB   R10, R11[0]
        SUB     R10, -1
        ST.B    R11[0], R10                                     /* OSIntNestingCtr++                                        */

        CP.W    R10, 1                                          /* Test OSIntNestingCtr                                     */
        BRNE    OSInt2ISRHandler_1

        MOV     R11, LWRD(OSTCBCurPtr)                          /* if (OSIntNestingCtr == 1) {                              */
        ORH     R11, HWRD(OSTCBCurPtr)
        LD.W    R10, R11[0]
        ST.W    R10[0], SP                                      /*     OSTCBCurPtr->OSTCBStkPtr = SP;                       */
                                                                /* }                                                        */
OSInt2ISRHandler_1:
        ICALL   R12                                             /* call user ISR function (address stored on R12)           */
        RCALL   OSIntExit
                                                                /* --- Restore stack frame                                  */
        LDM     SP++, R0-R1                                     /* Retrieve PC and SR from stack frame                      */
        LDMTS   SP++, R0-R12                                    /* Restore R0-R12 from stack                                */
        LDMTS   SP++, LR                                        /* Restore LR from stack                                    */
        RETE                                                    /* Return from event handler                                */


;$PAGE
OSInt3ISRHandler:
                                                                /* --- Save stack frame                                     */
        MFSR    R9, OS_CPU_RSR_INT3_OFFSET                      /* Retrieve SR from INT3 Context                            */
        MFSR    R8, OS_CPU_RAR_INT3_OFFSET                      /* Retrieve PC from INT3 Context                            */
        PUSHM   R8-R9                                           /* Save PC, SR into stack                                   */

        MOV     R11, LWRD(OSIntNestingCtr)
        ORH     R11, HWRD(OSIntNestingCtr)
        LD.UB   R10, R11[0]
        SUB     R10, -1
        ST.B    R11[0], R10                                     /* OSIntNestingCtr++                                        */

        CP.W    R10, 1                                          /* Test OSIntNestingCtr                                     */
        BRNE    OSInt3ISRHandler_1

        MOV     R11, LWRD(OSTCBCurPtr)                          /* if (OSIntNestingCtr == 1) {                              */
        ORH     R11, HWRD(OSTCBCurPtr)
        LD.W    R10, R11[0]
        ST.W    R10[0], SP                                      /*     OSTCBCurPtr->OSTCBStkPtr = SP;                       */
                                                                /* }                                                        */
OSInt3ISRHandler_1:
        ICALL   R12                                             /* call user ISR function (store on R12)                    */
        RCALL   OSIntExit
                                                                /* --- Restore stack frame                                  */
        LDM     SP++, R0-R1                                     /* Retrieve PC and SR from stack frame                      */
        LDMTS   SP++, R0-R12                                    /* Restore R0-R12 from stack                                */
        LDMTS   SP++, LR                                        /* Restore LR from stack                                    */
        RETE                                                    /* Return from event handler                                */


;$PAGE
/*
*********************************************************************************************************
*                                     CPU ASSEMBLY PORT FILE END
*********************************************************************************************************
*/

                END
