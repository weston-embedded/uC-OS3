;********************************************************************************************************
;                                              uC/OS-III
;                                        The Real-Time Kernel
;
;                    Copyright 2009-2020 Silicon Laboratories Inc. www.silabs.com
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
;                                       ASSEMBLY LANGUAGE PORT
;
;                                      $$$$ Processor Name
;                                      $$$$ Compiler/Assembler Name
;
; Filename : os_cpu_a.asm
; Version  : $$$$ V3.08.00
;********************************************************************************************************

                                         ; Include macros from 'os_cpu_a.inc'
;$$$$    #include  "os_cpu_a.inc"

;********************************************************************************************************
;                                           PUBLIC FUNCTIONS
;********************************************************************************************************

;$$$$    PUBLIC   _OSStartHighRdy             ; Public functions
;$$$$    PUBLIC   _OSCtxSw
;$$$$    PUBLIC   _OSIntCtxSw

;$$$$    EXTERN   _OSTCBCurPtr                ; Declared as OS_TCB *               , 32-bit long
;$$$$    EXTERN   _OSTCBHighRdyPtr            ; Declared as OS_TCB *               , 32-bit long
;$$$$    EXTERN   _OSPrioCur                  ; Declared as INT8U                  ,  8-bit long
;$$$$    EXTERN   _OSPrioHighRdy              ; Declared as INT8U                  ,  8-bit long
;$$$$    EXTERN   _OSTaskSwHook

;********************************************************************************************************
;                                      CODE GENERATION DIRECTIVES
;********************************************************************************************************

;$$$$    section .text:CODE:ROOT

;*********************************************************************************************************
;                                         START MULTITASKING
;
; Description : This function is called by OSStart() to start the highest priority task that was created
;               by your application before calling OSStart().
;
; Arguments   : none
;
; Note(s)     : 1) The stack frame is assumed to look as follows:
;
;                  OSTCBHighRdy->OSTCBStkPtr +  0  ---->
;
;               2) OSStartHighRdy() MUST:
;                      a) Call OSTaskSwHook() then,
;                      b) Switch to the highest priority task.
;*********************************************************************************************************

_OSStartHighRdy:
;$$$$              ; Call OSTaskSwHook()
;$$$$              ; SP	= OSTCBHighRdyPtr->StkPtr
    OS_CTX_RESTORE
;$$$$              ; Execute a return from interrupt/exception instruction

;*********************************************************************************************************
;                                       TASK LEVEL CONTEXT SWITCH
;
; Description : This function is called when a task makes a higher priority task ready-to-run.
;               The pseudo code is:
;
;                   OS_CTX_SAVE
;                   OSTCBCurPtr->SP = SP;
;                   OSTaskSwHook();
;                   OSPrioCur       = OSPrioHighRdy;
;                   OSTCBCurPtr     = OSTCBHighRdyPtr;
;                   SP              = OSTCBCurPtr->StkPtr;
;                   OS_CTX_RESTORE
;                   Return from interrupt/exception
;*********************************************************************************************************

_OSCtxSw:
    OS_CTX_SAVE     ; Call context save macro

;$$$$               ; OSTCBCurPtr->SP = SP;

;$$$$               ; OSTaskSwHook();

;$$$$               ; OSPrioCur   = OSPrioHighRdy

;$$$$               ; OSTCBCurPtr = OSTCBHighRdyPtr

;$$$$               ; SP = OSTCBCur->OSTCBStkPtr

    OS_CTX_RESTORE	; Call context restore macro

;$$$$               ; Return from interrupt/exception

;*********************************************************************************************************
;                               PERFORM A CONTEXT SWITCH (From an ISR)
;
; Description : This function is called when an ISR makes a higher priority task ready-to-run.
;*********************************************************************************************************

_OSIntCtxSw:
;$$$$               ; Call OSTaskSwHook();

;$$$$               ; OSPrioCur   = OSPrioHighRdy

;$$$$               ; OSTCBCurPtr = OSTCBHighRdyPtr

;$$$$               ; SP = OSTCBCur->OSTCBStkPtr

    OS_CTX_RESTORE	; Call context restore macro

;$$$$               ; Return from interrupt/exception


    END
