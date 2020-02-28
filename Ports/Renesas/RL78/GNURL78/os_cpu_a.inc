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
; File      : os_cpu_a.inc
; Version   : V3.08.00
;********************************************************************************************************
; For       : Renesas RL78
; Toolchain : e2studios V 1.1.1.5 for w/ KPIT GNURL78 V1.1.0.x
;********************************************************************************************************

;********************************************************************************************************
;                                  PUBLIC AND EXTERNAL DECLARATIONS
;********************************************************************************************************

    .global     _OSIntNestingCtr                                  ; Declared as 'CPU_INT08U'
    .global     _OSTCBCurPtr                                      ; Declared as 'OS_TCB *'

    .global     _OSTimeTick                                       ; OSTimeTick()
    .global     _OSIntExit                                        ; OSIntExit()


;********************************************************************************************************
;                                      CODE GENERATION DIRECTIVES
;********************************************************************************************************

    .section        .text


;
;********************************************************************************************************
;                                             OS_CTX_SAVE
;
; Description : This MACRO saves the CPU registers (i.e. CPU context) onto the current task's stack using
;               the same order as they were saved in OSTaskStkInit().
;
; Note(s)     : None.
;********************************************************************************************************

.macro  OS_CTX_SAVE
         PUSH   RP0                                             ; Register R1 + R0
         PUSH   RP1                                             ; Register R3 + R2
         PUSH   RP2                                             ; Register R5 + R4
         PUSH   RP3                                             ; Register R7 + R6
         MOV    A, CS                                           ; Save the CS register value
         MOV    X, A
         MOV    A, ES                                           ; Save the ES register value
         PUSH   RP0                                             ; Save the ES:CS values in register AX
.endm


;
;********************************************************************************************************
;                                           OS_CTX_RESTORE
;
; Description : This MACRO restores the CPU registers (i.e. context) from the new task's stack in the
;               reverse order of OS_CTX_SAVE MACRO (see above)
;
; Note(s)     : None.
;********************************************************************************************************

.macro  OS_CTX_RESTORE
         POP    RP0                                             ; Get ES:CS registers from AX
         MOV    ES, A
         MOV    A,  X
         MOV    CS, A                                           ; Restore the CS and ES registers
         POP    RP3
         POP    RP2
         POP    RP1
         POP    RP0
.endm


;
;********************************************************************************************************
;                                            OS_ISR_ENTER
;
; Description : Interrupt service routine prologue for kernel-aware handler.
;               This macro implements the following code in assembly language:
;
;                  OS_ISR_ENTER
;                     OS_CTX_SAVE                               ; Call the macro: OS_CTX_SAVE
;                      OSIntNestingCtr++;
;                      if (OSIntNestingCtr == 1) {
;                          OSTCBCur->OSTCBStkPtr = SP;
;                      }
;
;               This MACRO is to be used by your assembly language based ISRs as follows:
;
;                  MyISR
;                      OS_ISR_ENTER
;                      ISR Body here
;                      OS_ISR_EXIT
;********************************************************************************************************

.macro OS_ISR_ENTER
        OS_CTX_SAVE                                             ; Save processor registers on the stack

        MOVW    RP0, #_OSIntNestingCtr
        INCW    RP0                                             ; increment OSIntNesting
        CMPW    RP0, #1                                         ; if (OSIntNesting != 1)
        BNZ     $branch                                         ; jump out to OSTickISR

                                                                ; else
                                                                ; {
        MOVW    RP2, #_OSTCBCurPtr                              ;     OSTCBCurPtr in RP2
        MOVW    RP0, [SP]
        MOVW   [RP2], RP0                                       ;     OSTCBCur->OSTCBStkPtr = SP
                                                                ; }
branch:
.endm

;
;********************************************************************************************************
;                                             OS_ISR_EXIT
;
; Description : Interrupt service routine epilog for kernel-aware handler.
;               This macro implements the following code in assembly language:
;
;                  OS_ISR_EXIT:
;                      OSIntExit();                             ; Call the C function: OSIntExit();
;                      POPALL                                   ; Call the macro: OS_CTX_RESTORE to restore the registers
;                      Return from interrupt                    ; CPU instruction to return from interrupt/exception
;********************************************************************************************************

.macro  OS_ISR_EXIT
        MOVW    RP0, #_OSIntExit
        CALL    RP0                                             ; call OSIntExit()

        OS_CTX_RESTORE                                          ; restore all processor registers from stack

        RETI                                                    ; return from interrupt
.endm

;
;********************************************************************************************************
;                                     ASSEMBLY LANGUAGE MACROS FILE END
;********************************************************************************************************

