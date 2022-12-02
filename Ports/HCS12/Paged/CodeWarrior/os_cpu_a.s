;********************************************************************************************************
;                                              uC/OS-III
;                                        The Real-Time Kernel
;
;                    Copyright 2009-2022 Silicon Laboratories Inc. www.silabs.com
;
;                                 SPDX-License-Identifier: APACHE-2.0
;
;               This software is subject to an open source license and is distributed by
;                Silicon Laboratories Inc. pursuant to the terms of the Apache License,
;                    Version 2.0 available at www.apache.org/licenses/LICENSE-2.0.
;
;********************************************************************************************************

;********************************************************************************************************
;
;                                         MC9S12 Specific code
;                                          Banked Memory Model
;
; File    : os_cpu_a.s
; Version : V3.08.02
;********************************************************************************************************

NON_BANKED:       section

;********************************************************************************************************
;                                           I/O PORT ADDRESSES
;********************************************************************************************************

PPAGE:            equ    $0030         ; Addres of PPAGE register (assuming MC9S12 (non XGATE part)

;********************************************************************************************************
;                                          PUBLIC DECLARATIONS
;********************************************************************************************************

    xdef   OSStartHighRdy
    xdef   OSCtxSw
    xdef   OSIntCtxSw
    xdef   OSTickISR

;********************************************************************************************************
;                                         EXTERNAL DECLARATIONS
;********************************************************************************************************

    xref   OSIntExit
    xref   OSIntNestingCtr
    xref   OSPrioCur
    xref   OSPrioHighRdy
    xref   OSRunning
    xref   OSTaskSwHook
    xref   OSTCBCurPtr
    xref   OSTCBHighRdyPtr
    xref   OSTickISR_Handler
    xref   OSTimeTick


;********************************************************************************************************
;                               START HIGHEST PRIORITY TASK READY-TO-RUN
;
; Description : This function is called by OSStart() to start the highest priority task that was created
;               by your application before calling OSStart().
;
; Arguments   : none
;
; Note(s)     : 1) The stack frame is assumed to look as follows:
;
;                  OSTCBHighRdyPtr->OSTCBStkPtr +  0       PPAGE
;                                               +  1       CCR
;                                               +  2       B
;                                               +  3       A
;                                               +  4       X (H)
;                                               +  5       X (L)
;                                               +  6       Y (H)
;                                               +  7       Y (L)
;                                               +  8       PC(H)
;                                               +  9       PC(L)
;
;               2) OSStartHighRdy() MUST:
;                      a) Call OSTaskSwHook() then,
;                      b) Set OSRunning to TRUE,
;                      c) Switch to the highest priority task by loading the stack pointer of the
;                         highest priority task into the SP register and execute an RTI instruction.
;********************************************************************************************************

OSStartHighRdy:
    call   OSTaskSwHook                ;  4~, Invoke user defined context switch hook

    ldab   #$01                        ;  2~, Indicate that we are multitasking
    stab   OSRunning                   ;  4~

    ldx    OSTCBHighRdyPtr             ;  3~, Point to TCB of highest priority task ready to run
    lds    0,x                         ;  3~, Load SP into 68HC12

    pula                               ;  3~, Get value of PPAGE register
    staa   PPAGE                       ;  3~, Store into CPU's PPAGE register

    rti                                ;  4~, Run task

;********************************************************************************************************
;                                       TASK LEVEL CONTEXT SWITCH
;
; Description : This function is called when a task makes a higher priority task ready-to-run.
;
; Arguments   : none
;
; Note(s)     : 1) Upon entry,
;                  OSTCBCurPtr     points to the OS_TCB of the task to suspend
;                  OSTCBHighRdyPtr points to the OS_TCB of the task to resume
;
;               2) The stack frame of the task to suspend looks as follows:
;
;                  SP +  0       PC(H)
;                     +  1       PC(L)
;
;               3) The stack frame of the task to resume looks as follows:
;
;                  OSTCBHighRdyPtr->OSTCBStkPtr +  0       PPAGE
;                                               +  1       CCR
;                                               +  2       B
;                                               +  3       A
;                                               +  4       X (H)
;                                               +  5       X (L)
;                                               +  6       Y (H)
;                                               +  7       Y (L)
;                                               +  8       PC(H)
;                                               +  9       PC(L)
;********************************************************************************************************

OSCtxSw:
    ldaa   PPAGE                       ;  3~, Get current value of PPAGE register
    psha                               ;  2~, Push PPAGE register onto current task's stack

    ldy    OSTCBCurPtr                 ;  3~, OSTCBCurPtr->OSTCBStkPtr = Stack Pointer
    sts    0,y                         ;  3~,

    call   OSTaskSwHook                ;  4~, Call user task switch hook

    ldx    OSTCBHighRdyPtr             ;  3~, OSTCBCurPtr  = OSTCBHighRdyPtr
    stx    OSTCBCurPtr                 ;  3~

    ldab   OSPrioHighRdy               ;  3~, OSPrioCur = OSPrioHighRdy
    stab   OSPrioCur                   ;  3~

    lds    0,x                         ;  3~, Load SP into 68HC12

    pula                               ;  3~, Get value of PPAGE register
    staa   PPAGE                       ;  3~, Store into CPU's PPAGE register

    rti                                ;  8~, Run task

;********************************************************************************************************
;                                    INTERRUPT LEVEL CONTEXT SWITCH
;
; Description : This function is called by OSIntExit() to perform a context switch to a task that has
;               been made ready-to-run by an ISR. The PPAGE register of the preempted task has already
;               been stacked during the start of the ISR that is currently running.
;
; Arguments   : none
;********************************************************************************************************

OSIntCtxSw:
    call   OSTaskSwHook               ;  4~, Call user task switch hook

    ldx    OSTCBHighRdyPtr            ;  3~, OSTCBCurPtr  = OSTCBHighRdyPtr
    stx    OSTCBCurPtr                ;  3~

    ldab   OSPrioHighRdy              ;  3~, OSPrioCur = OSPrioHighRdy
    stab   OSPrioCur                  ;  3~

    lds    0,x                        ;  3~, Load the SP of the next task

    pula                              ;  3~, Get value of PPAGE register
    staa   PPAGE                      ;  3~, Store into CPU's PPAGE register

    rti                               ;  8~, Run task

;********************************************************************************************************
;                                           SYSTEM TICK ISR
;
; Description : This function is the ISR used to notify uC/OS-III that a system tick has occurred.  You
;               must setup the S12XE's interrupt vector table so that an OUTPUT COMPARE interrupt
;               vectors to this function.
;
; Arguments   : none
;
; Notes       :  1) The 'tick ISR' assumes the we are using the Output Compare specified by OS_TICK_OC
;                   (see APP_CFG.H and this file) to generate a tick that occurs every OS_TICK_OC_CNTS
;                   (see APP_CFG.H) which corresponds to the number of FRT (Free Running Timer)
;                   counts to the next interrupt.
;
;                2) All USER interrupts should be modeled EXACTLY like this where the only
;                   line to be modified is the call to your ISR_Handler and perhaps the call to
;                   the label name OSTickISR1.
;********************************************************************************************************

OSTickISR:
    ldaa   PPAGE                       ;  3~, Get current value of PPAGE register
    psha                               ;  2~, Push PPAGE register onto current task's stack

    inc    OSIntNestingCtr             ;  4~, Notify uC/OS-III about ISR

    ldab   OSIntNestingCtr             ;  4~, if (OSIntNestingCtr == 1) {
    cmpb   #$01                        ;  2~
    bne    OSTickISR1                  ;  3~

    ldy    OSTCBCurPtr                 ;  3~,     OSTCBCurPtr->OSTCBStkPtr = Stack Pointer
    sts    0,y                         ;  3~, }

OSTickISR1:
    call   OSTickISR_Handler

;   cli                                ;  2~, Enable interrupts to allow interrupt nesting

    call   OSIntExit                   ;  6~+, Notify uC/OS-III about end of ISR

    pula                               ;  3~, Get value of PPAGE register
    staa   PPAGE                       ;  3~, Store into CPU's PPAGE register

    rti                                ;  12~, Return from interrupt, no higher priority tasks ready.
