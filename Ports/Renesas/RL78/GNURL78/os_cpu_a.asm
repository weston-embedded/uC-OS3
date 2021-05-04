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
;                                           Renesas RL78 Port
;
; File      : os_cpu_a.asm
; Version   : V3.08.01
;********************************************************************************************************
; For       : Renesas RL78
; Toolchain : e2studios V 1.1.1.5 for w/ KPIT GNURL78 V1.1.0.x
;********************************************************************************************************

#include  "os_cpu_a.inc"

;********************************************************************************************************
;                                  PUBLIC AND EXTERNAL DECLARATIONS
;********************************************************************************************************

        .global  _OSStartHighRdy
        .global  _OSCtxSw
        .global  _OSIntCtxSw
        .global  _OSTickISR
        .global  _OSTaskSwHook
        .global  _OSTCBHighRdyPtr
        .global  _OSTCBCurPtr
        .global  _OSPrioCur
        .global  _OSPrioHighRdy
        .global  _OSIntEnter
        .global  _OSTimeTick
        .global  _OSIntExit
        .global  _OSIntNestingCtr


;********************************************************************************************************
;                                       CODE GENERATION DIRECTIVES
;********************************************************************************************************

    .text
    .section    .text



;********************************************************************************************************
;                                           MACRO DEFINITIONS
;********************************************************************************************************

;        ASEGN   RCODE:CODE, 0x007E
;        DW      OSCtxSw                                         ; Context Switch vector

;        RSEG    CODE                                            ; Program code

;********************************************************************************************************
;                                  START HIGHEST PRIORITY READY TASK
;
; Description: This function is called by OSStart() to start the highest priority task that is ready to run.
;
; Note       : OSStartHighRdy() MUST:
;                 a) Call OSTaskSwHook() then,
;                 b) Set OSRunning to TRUE,
;                 c) Switch to the highest priority task.
;
;              Registers are set as such:
;                   _______________________________________
;                   |  Functional Name  |  Absolute Name  |
;                   | 16-bit  |  8-bit  | 16-bit |  8-bit |
;                   ---------------------------------------
;                   |   HL    |    H    |   RP3  |   R7   |
;                   |         |    L    |        |   R6   |
;                   ---------------------------------------
;                   |   DE    |    D    |   RP2  |   R5   |
;                   |         |    E    |        |   R4   |
;                   ---------------------------------------
;                   |   BC    |    B    |   RP1  |   R3   |
;                   |         |    C    |        |   R2   |
;                   ---------------------------------------
;                   |   AX    |    A    |   RP0  |   R1   |
;                   |         |    X    |        |   R0   |
;                   ---------------------------------------
;
;
;********************************************************************************************************

_OSStartHighRdy:

        MOVW    BC, #_OSTaskSwHook                              ; Store location of OSTaskSWHook function
        CALL    BC                                              ; call OSTaskSwHook()

	    MOVW    HL, #_OSTCBHighRdyPtr                           ; HL = OSTCBHighRdyPtr
	    MOVW    AX, [HL + 0x0000]                               ; AX = OSTCBHighRdyPtr[0] (i.e. OSTCBStkPtr)
	    MOVW    HL, AX
	    MOVW    AX, [HL + 0x0000]                               ; AX = OSTCBHighRdy->OSTCBStkPtr value
	    MOVW    SP, AX                                          ; SP = OSTCBHighRdy->OSTCBStkPtr

        OS_CTX_RESTORE                                          ; restore all processor registers from new task's stack

        RETI                                                    ; return from interrupt



;********************************************************************************************************
;                                     TASK LEVEL CONTEXT SWITCH
;
; Description: This function is called by OS_Sched() to perform a task level context switch.
;
; Note       : OSCtxSw() MUST:
;                 a) Save the current task's registers onto the current task stack
;                 b) Save the SP into the current task's OS_TCB
;                 c) Call OSTaskSwHook()
;                 d) Copy OSPrioHighRdy to OSPrioCur
;                 e) Copy OSTCBHighRdyPtr to OSTCBCurPtr
;                 f) Load the SP with OSTCBHighRdyPtr->OSTCBStkPtr
;                 g) Restore all the registers from the high priority task stack
;                 h) Perform a return from interrupt
;********************************************************************************************************

_OSCtxSw:

        OS_CTX_SAVE                                             ; save processor registers on the stack

                                                                ; Save OSTCBCurPtr->StkPtr as current SP value
        MOVW    DE, #_OSTCBCurPtr                               ; Get OSTCBCurPtr
        MOVW    AX, [DE + 0x0000]								; Store OSTCBCurPtr address in AX
        MOVW	DE, AX
        MOVW    AX, SP											; AX->OSTCBCurPtr (AX pointed to OSTCBCur)
        MOVW   [DE + 0x0000], AX                                ; OSTCBCurPtr->OSTCBStkPtr = SP

        MOVW    BC, #_OSTaskSwHook
        CALL    BC                                              ; call OSTaskSwHook()

																; Set OSTCBCurPtr = OSTCBHighRdyPtr
        MOVW    HL, #_OSTCBHighRdyPtr                           ; HL = OSTCBHighRdyPtr
        MOVW    AX, [HL + 0x0000]                               ; AX = OSTCBHighRdyPtr[0] (i.e. OSTCBStkPtr)
        MOVW    HL, #_OSTCBCurPtr                               ; HL = OSTCBCurPtr
        MOVW   [HL + 0x0000], AX                                ; OSTCBCurPtr = OSTCBHighRdyPtr[0] (sets equal to one another)

																; Set OSPrioCur to the next ready priority
        MOVW    DE, #_OSPrioCur                                 ; DE = OSPrioCur
        MOVW    HL, #_OSPrioHighRdy                             ; HL = OSPrioHighRdy
        MOV     A, [HL + 0x0000]							    ; AX->OSPrioHighRdy
        MOV    [DE + 0x0000], A                                 ; OSPrioCur = OSPrioHighRdy

        MOVW    HL, #_OSTCBHighRdyPtr                           ; HL = OSTCBHighRdy
        MOVW    AX, [HL + 0x0000]                               ; AX = OSTCBHighRdy[0] (i.e. OSTCBStkPtr)
        MOVW    HL, AX
        MOVW    AX, [HL + 0x0000]                               ; AX = OSTCBHighRdy->OSTCBStkPtr value
        MOVW    SP, AX                                          ; SP = OSTCBHighRdy->OSTCBStkPtr

        OS_CTX_RESTORE                                          ; restore all processor registers from new task's stack

        RETI                                                    ; return from interrupt

;********************************************************************************************************
;                                       ISR LEVEL CONTEXT SWITCH
;
; Description: This function is called by OSIntExit() to perform an ISR level context switch.
;
; Note       : OSIntCtxSw() MUST:
;                 a) Call OSTaskSwHook()
;                 b) Copy OSPrioHighRdy to OSPrioCur
;                 c) Copy OSTCBHighRdyPtr to OSTCBCurPtr
;                 d) Load the SP with OSTCBHighRdyPtr->OSTCBStkPtr
;                 e) Restore all the registers from the high priority task stack
;                 f) Perform a return from interrupt
;********************************************************************************************************

_OSIntCtxSw:
        MOVW    AX, #_OSTaskSwHook
        CALL    AX                                              ; call OSTaskSwHook

                                                                ; Set OSTCBCur = OSTCBHighRdy
        MOVW    HL, #_OSTCBHighRdyPtr                           ; HL = OSTCBHighRdyPtr
        MOVW    AX, [HL + 0x0000]                               ; AX = OSTCBHighRdyPtr[0] (i.e. OSTCBStkPtr)
        MOVW    HL, #_OSTCBCurPtr                               ; HL = OSTCBCurPtr
        MOVW   [HL + 0x0000], AX                                ; OSTCBCur = OSTCBHighRdy[0] (sets equal to one another)

        MOVW    DE, #_OSPrioCur                                 ; DE = OSPrioCur
        MOVW    HL, #_OSPrioHighRdy                             ; HL = OSPrioHighRdy
        MOVW    AX, [HL + 0x0000]                               ; AX->OSPrioHighRdy
        MOVW   [DE + 0x0000], AX                                ; OSPrioCur = OSPrioHighRdy

        MOVW    HL, #_OSTCBHighRdyPtr                           ; HL = OSTCBHighRdyPtr
        MOVW    AX, [HL + 0x0000]                               ; AX = OSTCBHighRdyPtr[0] (i.e. OSTCBStkPtr)
        MOVW    HL, AX
        MOVW    AX, [HL + 0x0000]                               ; AX = OSTCBHighRdyPtr->OSTCBStkPtr value
        MOVW    SP, AX                                          ; SP = OSTCBHighRdyPtr->OSTCBStkPtr

        OS_CTX_RESTORE                                          ; Restore all processor registers from new task's stack

        RETI                                                    ; Return from interrupt

;********************************************************************************************************
;                                             OSTickISR()
;********************************************************************************************************


_OSTickISR:

        OS_ISR_ENTER

        MOVW    RP0, #_OSTimeTick
        CALL    RP0                                             ; call OSTimeTick()

        OS_ISR_EXIT


        .END
