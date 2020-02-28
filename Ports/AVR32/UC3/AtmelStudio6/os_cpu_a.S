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
*                                         ATMEL AVR32 UC3 Port
*
* File      : os_cpu_a.S
* Version   : V3.08.00
*********************************************************************************************************
* Toolchain : Atmel Studios
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             ASM HEADER
*********************************************************************************************************
*/

        .file    "OS_CPU_A"

        .section .text, "ax"


#$PAGE
/*
*********************************************************************************************************
*                                               DEFINES
*********************************************************************************************************
*/

        .equ     OS_CPU_SR_GM_OFFSET,                     16    /* Status  Register, Global Interrupt Mask Offset       */


/*
*********************************************************************************************************
*                                         PUBLIC DECLARATIONS
*********************************************************************************************************
*/

        .global  OSCtxSw
        .global  OSCtxRestore
        .global  OSIntCtxRestore
        .global  OSIntISRHandler
        .global  OSFastIntISRHandler

        .extern  OSTCBCurPtr
        .extern  OSTCBHighRdyPtr
        .extern  OSTaskSwHook
        .extern  OSPrioHighRdy
        .extern  OSPrioCur
        .extern  OSIntNestingCtr
        .extern  OSIntExit
        .extern  BSP_INTC_IntGetHandler


#$PAGE
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
*              2) The stack frame of the task to suspend looks as follows:
*
*                                        SP --> SR           (Low Memory)
*                                               PC           (High memory)
*
*              3) The saved context of the task to resume looks as follows:
*
*                 OSTCBHighRdyPtr->OSTCBStkPtr --> SP   -->
*                                                  R7   ----/   (Low memory)
*                                                  .
*                                                  .
*                                                  R0
*                                                  SR
*                                                  PC
*                                                  LR
*                                                  R12
*                                                  .
*                                                  .
*                                                  R8   ----\   (High memory)
*
*                 where the stack pointer points to the task start address.
*
*              4) OSCtxSw() has to save all registers from the suspended task. Since PC and SR are
*                 already on the stack, they need to be moved after R0-R7.  Then, the remaining
*                 registers (R8-R12, LR) are pushed into the stack frame.  Only then the Stack
*                 Pointer of the task to suspend is saved on OSTCBCurPtr.
*
*              5) OSCtxSw() MUST:
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
        PUSHM   R10-R12, LR                                     /* Save R10-R12, LR into stack                              */
        LD.D    R10, SP[4*4]                                    /* Copy SR and PC from bottom of stack                      */
        PUSHM   R10-R11                                         /* Save SR and PC into stack                                */
        ST.D    SP[6*4], R8                                     /* Save R8 and R9 into stack                                */
        PUSHM   R0-R7                                           /* Save R0-R7 into stack                                    */

        MOV     R8, LO(OSTCBCurPtr)
        ORH     R8, HI(OSTCBCurPtr)                           /*  OSTCBCurPtr                                             */
        LD.W    R9, R8[0]                                       /* *OSTCBCurPtr                                             */
        ST.W    R9[0], SP                                       /*  OSTCBCurPtr->OSTCBStkPtr = SP                           */

        RCALL   OSTaskSwHook

        MOV     R12, LO(OSTCBHighRdyPtr)
        ORH     R12, HI(OSTCBHighRdyPtr)                      /*  OSTCBHighRdyPtr                                         */
        LD.W    R10, R12[0]                                     /* *OSTCBHighRdyPtr                                         */
        MOV     R8,  LO(OSTCBCurPtr)
        ORH     R8,  HI(OSTCBCurPtr)                          /*  OSTCBCurPtr                                             */
        ST.W    R8[0], R10                                      /*  OSTCBCurPtr = OSTCBHighRdyPtr                           */

        MOV     R12, LO(OSPrioHighRdy)
        ORH     R12, HI(OSPrioHighRdy)
        LD.UB   R11, R12[0]                                     /* *OSPrioHighRdy                                           */
        MOV     R12, LO(OSPrioCur)
        ORH     R12, HI(OSPrioCur)
        ST.B    R12[0], R11                                     /*  OSPrioCur = OSPrioHighRdy                               */

        LD.W    R12, R10[0]                                     /* Retrieve OSTCBHighRdyPtr->OSTCBStkPtr                    */

        LDM     R12, R0-R7                                      /* Restore R0-R7                                            */
        LD.D    R8,  R12[14*4]                                  /* Restore R8-R9                                            */
        LD.D    R10, R12[ 8*4]                                  /* Retrieve PC and SR from stack frame                      */
        ST.D    R12[14*4], R10                                  /* Store PC and SR at bottom of stack frame                 */
        SUB     R12, -10*4                                      /* Move Stack Pointer Reference to LR                       */
        SUB     SP, R12, -4*4                                   /* Restore Stack Pointer                                    */
        LDM     R12, R10-R12, LR                                /* Restore R10-R12, LR                                      */

        RETS                                                    /* Restore PC and SR {restore task}                         */


#$PAGE
/*
*********************************************************************************************************
*                                      RESTORE CONTEXT FUNCTIONS
*
* Description: These functions are used to perform context switch.
*
*              void  OSCtxRestore (CPU_STK  *p_stk)
*                     Restore R0-R9
*                     Move SR and PC to bottom of Stack frame
*                     Restore Stack Pointer to point to SR and PC
*                     Restore R10-R12, LR
*                     Return from Supervisor Call       {context switch}
*
*              void  OSIntCtxRestore (CPU_STK  *p_stk)
*                     Restore Stack Pointer
*                     Restore R0-R7
*                     Return from INT                   {context switch}
*
* Arguments  : p_stk        Stack address for the context to be restored
*********************************************************************************************************
*/

OSCtxRestore:
        LDM     R12, R0-R7                                      /* Restore R0-R7                                            */
        LD.D    R8,  R12[14*4]                                  /* Restore R8-R9                                            */
        LD.D    R10, R12[ 8*4]                                  /* Retrieve PC and SR from stack frame                      */
        ST.D    R12[14*4], R10                                  /* Store PC and SR at bottom of stack frame                 */
        SUB     R12, -10*4                                      /* Move Stack Pointer Reference to LR                       */
        SUB     SP, R12, -4*4                                   /* Restore Stack Pointer                                    */
        LDM     R12, R10-R12, LR                                /* Restore R10-R12, LR                                      */
        RETS                                                    /* Restore PC and SR {restore task}                         */


OSIntCtxRestore:
        MOV     SP, R12                                         /* Restore SP (Stack Pointer)                               */
        POPM    R0-R7                                           /* Restore R0-R7                                            */
        RETE                                                    /* Restore R8-R12, LR, PC and SR {restore task}             */


#$PAGE
/*
*********************************************************************************************************
*                                        OS INTERRUPT HANDLER
*
* Description: This function handles the OS specific steps to wrap an user-defined function to be called
*              on an interrupt.
*
*                   prototype:  void  OSIntISRHandler(CPU_FNCT_PTR  p_user_isr);
*
* Notes      : 1) OSIntISRHandler() MUST:
*                   a) save remaining registers (R0-R7),
*                   b) increment OSIntNestingCtr,
*                   c) if (OSIntNestingCtr == 1) OSTCBCurPtr->OSTCBStkPtr = SP,
*                   d) re-enable interrupts,
*                   e) call user-defined function,
*                   f) call OSIntExit(),
*                   g) restore remaining registers (R0-R7),
*                   h) return from interrupt.
*
*              2) Interrupts MUST be disabled when this function is called.
*********************************************************************************************************
*/

OSIntISRHandler:
        PUSHM   R0-R7                                           /* Save R0-R7 into stack                                    */

        MOV     R11, LO(OSIntNestingCtr)
        ORH     R11, HI(OSIntNestingCtr)
        LD.UB   R10, R11[0]
        SUB     R10, -1
        ST.B    R11[0], R10                                     /* OSIntNestingCtr++                                        */

        CP.W    R10, 1                                          /* Test OSIntNestingCtr                                     */
        BRNE    OSIntISRHandler_1

        MOV     R11, LO(OSTCBCurPtr)                          /* if (OSIntNestingCtr == 1) {                              */
        ORH     R11, HI(OSTCBCurPtr)
        LD.W    R10, R11[0]
        ST.W    R10[0], SP                                      /*     OSTCBCurPtr->OSTCBStkPtr = SP;                       */
                                                                /* }                                                        */
OSIntISRHandler_1:
        CSRF    OS_CPU_SR_GM_OFFSET                             /* Clear global interrupt mask (enable interrupts)          */

        ICALL   R12                                             /* call user ISR function                                   */
        RCALL   OSIntExit

        POPM    R0-R7                                           /* Restore R0-R7 from stack                                 */
        RETE


#$PAGE
/*
*********************************************************************************************************
*                                 OS FAST (non-OS) INTERRUPT HANDLER
*
* Description: This function handles OS specific steps to wrap an user-defined function to be called on
*              a fast (non-OS) interrupt.
*
*                   prototype:  void  OSFastIntISRHandler(CPU_FNCT_PTR p_user_isr);
*
* Notes      : 1) OSFastIntISRHandler() MUST:
*                   a) increment OSIntNestingCtr,
*                   b) re-enable interrupts,
*                   c) call user-defined function,
*                   d) disable interrupts,
*                   e) decrement OSIntNestingCtr,
*                   f) return from interrupt.
*
*              2) Interrupts MUST be disabled when this function is called.
*********************************************************************************************************
*/

OSFastIntISRHandler:
        MOV     R11, LO(OSIntNestingCtr)
        ORH     R11, HI(OSIntNestingCtr)
        LD.UB   R10, R11[0]
        SUB     R10, -1
        ST.B    R11[0], R10                                     /* OSIntNestingCtr++                                        */

        CSRF    OS_CPU_SR_GM_OFFSET                             /* Clear global interrupt mask (enable interrupts)          */

        ICALL   R12                                             /* call user ISR function                                   */

        SSRF    OS_CPU_SR_GM_OFFSET                             /* Set global interrupt mask (disable interrupts)           */
        NOP
        NOP

        MOV     R11, LO(OSIntNestingCtr)
        ORH     R11, HI(OSIntNestingCtr)
        LD.UB   R10, R11[0]
        SUB     R10, 1
        ST.B    R11[0], R10                                     /* OSIntNestingCtr--                                        */

        RETE


#$PAGE
/*
*********************************************************************************************************
*                                     CPU ASSEMBLY PORT FILE END
*********************************************************************************************************
*/

