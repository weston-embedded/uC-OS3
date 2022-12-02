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
;                                        ASSEMBLY LANGUAGE PORT
;                                         Renesas V850E2M Port
;
; File      : os_cpu_a.asm
; Version   : V3.08.02
;********************************************************************************************************
; For       : Renesas V850E2M
; Toolchain : CubeSuite+ V1.00.01
;             CX compiler V1.20
;********************************************************************************************************

                                                                ; Include macros from 'os_cpu_a.inc'
    $INCLUDE  (os_cpu_a.inc)

;********************************************************************************************************
;                                           PUBLIC FUNCTIONS
;********************************************************************************************************
                                                                ; External References
    .extern  _OSPrioCur
    .extern  _OSPrioHighRdy
    .extern  _OSTCBCurPtr
    .extern  _OSTCBHighRdyPtr
    .extern  _OSTaskSwHook
    .extern  _OSTimeTick
                                                                ; Functions declared in this file
    .public   _OSStartHighRdy
    .public   _OSIntCtxSw
    .public   _OSCtxSw
    .public   _OS_CPU_TickHandler

;********************************************************************************************************
;                                      CODE GENERATION DIRECTIVES
;********************************************************************************************************

    .cseg  text
    .align  4

;PAGE
;********************************************************************************************************
;                                         START MULTITASKING
;
; Description : This function is called by OSStart() to start the highest priority task that was created
;               by your application before calling OSStart().
;
; Arguments   : none
;
; Note(s)     : 1) OSStartHighRdy() MUST:
;                  a) Call OSTaskSwHook() then,
;                  b) Switch to the highest priority task.
;********************************************************************************************************

_OSStartHighRdy:
    jarl  _OSTaskSwHook, lp              ; Call OSTaskSwHook();

    mov   #_OSTCBHighRdyPtr, r11         ; SWITCH TO HIGHEST PRIORITY TASK:
    ld.w  0[r11]        , r11
    ld.w  0[r11]        , sp

    OS_CTX_RESTORE sp                    ; Restore Task Context

    eiret

;PAGE
;********************************************************************************************************
;                         PERFORM A CONTEXT SWITCH (From task level) - OSCtxSw()
;
; Description : This function is called when a task makes a higher priority task ready-to-run.
;
; Note(s)     : 1) The pseudo-code for OSCtxSw() is:
;                  a) Save the current task's context onto the current task's stack,
;                  b) OSTCBCurPtr->StkPtr = SP;
;                  c) OSTaskSwHook();
;                  d) OSPrioCur           = OSPrioHighRdy;
;                  e) OSTCBCurPtr         = OSTCBHighRdyPtr;
;                  f) SP                  = OSTCBHighRdyPtr->StkPtr;
;                  g) Restore the new task's context from the new task's stack,
;                  h) Return to new task's code.
;
;               2) Upon entry:
;                  OSTCBCurPtr      points to the OS_TCB of the task to suspend,
;                  OSTCBHighRdyPtr  points to the OS_TCB of the task to resume.
;********************************************************************************************************

_OSCtxSw:
    OS_CTX_SAVE sp                       ; Save current Task context

    mov   #_OSTCBCurPtr, r11             ; OSTCBCurPtr->OSTCBStkPtr = SP;
    ld.w  0[r11]       , r11
    st.w  sp           , 0[r11]

    jarl  _OSTaskSwHook, lp              ; OSTaskSwHook();

    mov   #_OSPrioHighRdy, r11           ; OSPrioCur = OSPrioHighRdy;
    ld.b  0[r11]         , r12
    mov   #_OSPrioCur    , r11
    st.b  r12            , 0[r11]

    mov   #_OSTCBHighRdyPtr, r11         ; OSTCBCurPtr = OSTCBHighRdyPtr;
    ld.w  0[r11]           , r12
    mov   #_OSTCBCurPtr    , r11
    st.w  r12              , 0[r11]

    ld.w  0[r12], sp                     ; SP = OSTCBHighRdyPtr->OSTCBStkPtr;

    OS_CTX_RESTORE sp                    ; Restore new Task's context

    eiret                                ; return from trap


;PAGE
;********************************************************************************************************
;                     PERFORM A CONTEXT SWITCH (From interrupt level) - OSIntCtxSw()
;
; Description : This function is called when an ISR makes a higher priority task ready-to-run.
;
; Note(s)     : 1) The pseudo-code for OSIntCtxSw() is:
;                  a) OSTaskSwHook();
;                  b) OSPrioCur   = OSPrioHighRdy;
;                  c) OSTCBCurPtr = OSTCBHighRdyPtr;
;                  d) SP          = OSTCBHighRdyPtr->OSTCBStkPtr;
;                  e) Restore the new task's context from the new task's stack,
;                  f) Return to new task's code.
;
;               2) Upon entry:
;                  OSTCBCurPtr      points to the OS_TCB of the task to suspend,
;                  OSTCBHighRdyPtr  points to the OS_TCB of the task to resume.
;********************************************************************************************************

_OSIntCtxSw:
    jarl  _OSTaskSwHook, lp              ; OSTaskSwHook();

    mov   #_OSPrioHighRdy, r11           ; OSPrioCur = OSPrioHighRdy;
    ld.b  0[r11]         , r12
    mov   #_OSPrioCur    , r11
    st.b  r12            , 0[r11]

    mov   #_OSTCBHighRdyPtr, r11         ; OSTCBCur = OSTCBHighRdyPtr;
    ld.w  0[r11]           , r12
    mov   #_OSTCBCurPtr    , r11
    st.w  r12              , 0[r11]

    ld.w  0[r12], sp                     ; SP = OSTCBHighRdyPtr->OSTCBStkPtr;

    OS_CTX_RESTORE sp                    ; Restore new Task's context

    eiret                                ; Return from interrupt starts new task

;PAGE
;********************************************************************************************************
;                                        OS_CPU_TickHandler
;
; Note(s) : 1) The pseudo-code for _OS_CPU_TickHandler() is:
;              a) Save processor registers;
;              b) Increment OSIntNestingCtr;
;              c) if (OSIntNestingCtr == 1) {
;                     OSTCBCurPtr->OSTCBStkPtr = SP;
;                 }
;              d) Call OSTimeTick();
;              e) Call OSIntExit();
;              f) Restore processosr Registers;
;
;           2) OS_CPU_TickHandler() must be registered in the proper vector address of timer that will be
;              used as the tick.
;
;           3) All the other ISRs must have the following implementation to secure proper register saving &
;              restoring when servicing an interrupt
;
;              MyISR
;                  OS_ISR_ENTER
;                  ISR Body here
;                  OS_ISR_EXIT
;********************************************************************************************************

_OS_CPU_TickHandler:
    OS_ISR_ENTER

    jarl _OSTimeTick, lp                 ; Call OSTimeTick();

    OS_ISR_EXIT

;$PAGE
;********************************************************************************************************
;                                     ASSEMBLY LANGUAGE PORT FILE END
;********************************************************************************************************
