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
; Toolchain : IAR EWRL78 v1.2x and up
;********************************************************************************************************



;********************************************************************************************************
;                                  PUBLIC AND EXTERNAL DECLARATIONS
;********************************************************************************************************

    EXTERN     OSIntNestingCtr                                  ; Declared as 'CPU_INT08U'
    EXTERN     OSTCBCurPtr                                      ; Declared as 'OS_TCB *'

    EXTERN     OSTimeTick                                       ; OSTimeTick()
    EXTERN     OSIntExit                                        ; OSIntExit()


;********************************************************************************************************
;                                      CODE GENERATION DIRECTIVES
;********************************************************************************************************

         RSEG CODE:CODE:NOROOT(1)


;********************************************************************************************************
;                                             PUSHALL
;
; Description : This MACRO saves the CPU registers (i.e. CPU context) onto the current task's stack using
;               the same order as they were saved in OSTaskStkInit().
;
; Note(s)     : None.
;********************************************************************************************************

PUSHALL  MACRO
         PUSH   RP0                                             ; Register R1 + R0
         PUSH   RP1                                             ; Register R3 + R2
         PUSH   RP2                                             ; Register R5 + R4
         PUSH   RP3                                             ; Register R7 + R6
         MOV    A, CS                                           ; Save the CS register value
         MOV    X, A
         MOV    A, ES                                           ; Save the ES register value
         PUSH   RP0                                             ; Save the ES:CS values in register AX
         ENDM


;********************************************************************************************************
;                                           POPALL
;
; Description : This MACRO restores the CPU registers (i.e. context) from the new task's stack in the
;               reverse order of PUSHALL MACRO (see above)
;
; Note(s)     : None.
;********************************************************************************************************

POPALL   MACRO
         POP    RP0                                             ; Get ES:CS registers from AX
         MOV    ES, A
         MOV    A,  X
         MOV    CS, A                                           ; Restore the CS and ES registers
         POP    RP3
         POP    RP2
         POP    RP1
         POP    RP0
         ENDM


;********************************************************************************************************
;                                            OS_ISR_ENTER
;
; Description : Interrupt service routine prologue for kernel-aware handler.
;               This macro implements the following code in assembly language:
;
;                  OS_ISR_ENTER
;                     PUSHALL                                   ; Call the macro: PUSHALL
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

OS_ISR_ENTER   MACRO
               PUSHALL                                          ; Save processor registers on the stack
               LOCAL   branch

               INC     OSIntNestingCtr                          ; increment OSIntNesting
               CMP     OSIntNestingCtr, #1                      ; if (OSIntNestingCtr != 1)
               BNZ     branch                                   ; jump out to OSTickISR

                                                                ; else
                                                                ; {
               MOVW    RP2, OSTCBCurPtr                         ;     OSTCBCurPtr in RP2
               MOVW    RP0, SP
               MOVW    [RP2], RP0                               ;     OSTCBCur->OSTCBStkPtr = SP
                                                                ; }
branch:
               ENDM

;********************************************************************************************************
;                                             OS_ISR_EXIT
;
; Description : Interrupt service routine epilog for kernel-aware handler.
;               This macro implements the following code in assembly language:
;
;                  OS_ISR_EXIT:
;                      OSIntExit();                             ; Call the C function: OSIntExit();
;                      POPALL                                   ; Call the macro: POPALL to restore the registers
;                      Return from interrupt                    ; CPU instruction to return from interrupt/exception
;********************************************************************************************************

OS_ISR_EXIT   MACRO
              CALL    OSIntExit                                 ; call OSIntExit()

              POPALL                                            ; restore all processor registers from stack

              RETI                                              ; return from interrupt
              ENDM

;$PAGE
;********************************************************************************************************
;                                     ASSEMBLY LANGUAGE MACROS FILE END
;********************************************************************************************************

