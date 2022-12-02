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
;                                             ARMv7-R Port
;
; File      : os_cpu_a_vfp-none.asm
; Version   : V3.08.02
;********************************************************************************************************
; For       : ARMv7-R Cortex-R
; Mode      : ARM or Thumb
; Toolchain : TI TMS470 COMPILER
;********************************************************************************************************
; Note(s)   : (1) See Note #2 of os_cpu.h for important informations about this file.
;********************************************************************************************************


;********************************************************************************************************
;                                          PUBLIC FUNCTIONS
;********************************************************************************************************
                                                                ; External references.
    .global  OSRunning
    .global  OSPrioCur
    .global  OSPrioHighRdy
    .global  OSTCBCurPtr

    .global  OSIntNestingCtr
    .global  OSIntExit
    .global  OSTaskSwHook
    .global  OS_CPU_ExceptHndlr
    .global  OSTCBHighRdyPtr
    .global  OS_CPU_ExceptStkBase

                                                                ; Functions declared in this file.
    .global  OSStartHighRdy
    .global  OSCtxSw
    .global  OSIntCtxSw

                                                                ; Functions related to exception handling.
    .global  OS_CPU_ARM_ExceptUndefInstrHndlr
    .global  OS_CPU_ARM_ExceptSwiHndlr
    .global  OS_CPU_ARM_ExceptPrefetchAbortHndlr
    .global  OS_CPU_ARM_ExceptDataAbortHndlr
    .global  OS_CPU_ARM_ExceptAddrAbortHndlr
    .global  OS_CPU_ARM_ExceptIrqHndlr
    .global  OS_CPU_ARM_ExceptFiqHndlr

    .global  OS_CPU_ARM_DRegCntGet


OSRunningAddr            .word  OSRunning
OSPrioCurAddr            .word  OSPrioCur
OSPrioHighRdyAddr        .word  OSPrioHighRdy
OSTCBCurPtrAddr          .word  OSTCBCurPtr

OSTCBHighRdyPtrAddr      .word  OSTCBHighRdyPtr

OSIntNestingCtrAddr      .word  OSIntNestingCtr
OS_CPU_ExceptStkBaseAddr .word  OS_CPU_ExceptStkBase


;********************************************************************************************************
;                                               EQUATES
;********************************************************************************************************

OS_CPU_ARM_CONTROL_INT_DIS        .equ  0xC0                     ; Disable both FIQ and IRQ.
OS_CPU_ARM_CONTROL_FIQ_DIS        .equ  0x40                     ; Disable FIQ.
OS_CPU_ARM_CONTROL_IRQ_DIS        .equ  0x80                     ; Disable IRQ.
OS_CPU_ARM_CONTROL_THUMB          .equ  0x20                     ; Set THUMB mode.
OS_CPU_ARM_CONTROL_ARM            .equ  0x00                     ; Set ARM mode.

OS_CPU_ARM_MODE_MASK              .equ  0x1F
OS_CPU_ARM_MODE_USR               .equ  0x10
OS_CPU_ARM_MODE_FIQ               .equ  0x11
OS_CPU_ARM_MODE_IRQ               .equ  0x12
OS_CPU_ARM_MODE_SVC               .equ  0x13
OS_CPU_ARM_MODE_ABT               .equ  0x17
OS_CPU_ARM_MODE_UND               .equ  0x1B
OS_CPU_ARM_MODE_SYS               .equ  0x1F

OS_CPU_ARM_EXCEPT_RESET           .equ  0x00
OS_CPU_ARM_EXCEPT_UNDEF_INSTR     .equ  0x01
OS_CPU_ARM_EXCEPT_SWI             .equ  0x02
OS_CPU_ARM_EXCEPT_PREFETCH_ABORT  .equ  0x03
OS_CPU_ARM_EXCEPT_DATA_ABORT      .equ  0x04
OS_CPU_ARM_EXCEPT_ADDR_ABORT      .equ  0x05
OS_CPU_ARM_EXCEPT_IRQ             .equ  0x06
OS_CPU_ARM_EXCEPT_FIQ             .equ  0x07

OS_CPU_ARM_FPEXC_EN               .equ  0x40000000


;********************************************************************************************************
;                                     CODE GENERATION DIRECTIVES
;********************************************************************************************************

     .text
     .state32


;********************************************************************************************************
;                                         START MULTITASKING
;                                      void OSStartHighRdy(void)
;
; Note(s) : 1) OSStartHighRdy() MUST:
;              a) Call OSTaskSwHook() then,
;              b) Set OSRunning to OS_STATE_OS_RUNNING,
;              c) Switch to the highest priority task.
;********************************************************************************************************

OSStartHighRdy:
                                                                ; Change to SVC mode.
    MSR     CPSR_c, #(OS_CPU_ARM_CONTROL_INT_DIS | OS_CPU_ARM_MODE_SVC)
    CLREX                                                       ; Clear exclusive monitor.

    BL      OSTaskSwHook                                        ; OSTaskSwHook();

                                                                ; SWITCH TO HIGHEST PRIORITY TASK:
    LDR     R0, OSTCBHighRdyPtrAddr                             ;    Get highest priority task TCB address,
    LDR     R0, [R0]                                            ;    Get stack pointer,
    LDR     SP, [R0]                                            ;    Switch to the new stack,

    LDR     R0, [SP], #4                                        ;    Pop new task's CPSR,
    MSR     SPSR_cxsf, R0

    LDMFD   SP!, {R0-R12, LR, PC}^                              ;    Pop new task's context.


;********************************************************************************************************
;                       PERFORM A CONTEXT SWITCH (From task level) - OSCtxSw()
;
; Note(s) : 1) OSCtxSw() is called in SVC mode with BOTH FIQ and IRQ interrupts DISABLED.
;
;           2) The pseudo-code for OSCtxSw() is:
;              a) Save the current task's context onto the current task's stack,
;              b) OSTCBCurPtr->StkPtr = SP;
;              c) OSTaskSwHook();
;              d) OSPrioCur           = OSPrioHighRdy;
;              e) OSTCBCurPtr         = OSTCBHighRdyPtr;
;              f) SP                  = OSTCBHighRdyPtr->StkPtr;
;              g) Restore the new task's context from the new task's stack,
;              h) Return to new task's code.
;
;           3) Upon entry:
;              OSTCBCurPtr      points to the OS_TCB of the task to suspend,
;              OSTCBHighRdyPtr  points to the OS_TCB of the task to resume.
;********************************************************************************************************

OSCtxSw:
                                                                ; SAVE CURRENT TASK'S CONTEXT:
    STMFD   SP!, {LR}                                           ;     Push return address,
    STMFD   SP!, {LR}
    STMFD   SP!, {R0-R12}                                       ;     Push registers,
    MRS     R0, CPSR                                            ;     Push current CPSR,
    TST     LR, #1                                              ;     See if called from Thumb mode,
    ORRNE   R0, R0, #OS_CPU_ARM_CONTROL_THUMB                   ;     If yes, set the T-bit.
    STMFD   SP!, {R0}

    CLREX                                                       ; Clear exclusive monitor.

    LDR     R0, OSTCBCurPtrAddr                                 ; OSTCBCurPtr->StkPtr = SP;
    LDR     R1, [R0]
    STR     SP, [R1]

    BL      OSTaskSwHook                                        ; OSTaskSwHook();

    LDR     R0, OSPrioCurAddr                                   ; OSPrioCur   = OSPrioHighRdy;
    LDR     R1, OSPrioHighRdyAddr
    LDRB    R2, [R1]
    STRB    R2, [R0]

    LDR     R0, OSTCBCurPtrAddr                                 ; OSTCBCurPtr = OSTCBHighRdyPtr;
    LDR     R1, OSTCBHighRdyPtrAddr
    LDR     R2, [R1]
    STR     R2, [R0]

    LDR     SP, [R2]                                            ; SP = OSTCBHighRdyPtr->OSTCBStkPtr;

                                                                ; RESTORE NEW TASK'S CONTEXT:
    LDMFD   SP!, {R0}                                           ;    Pop new task's CPSR,
    MSR     SPSR_cxsf, R0

    LDMFD   SP!, {R0-R12, LR, PC}^                              ;    Pop new task's context.


;********************************************************************************************************
;                   PERFORM A CONTEXT SWITCH (From interrupt level) - OSIntCtxSw()
;
; Note(s) : 1) OSIntCtxSw() is called in SVC mode with BOTH FIQ and IRQ interrupts DISABLED.
;
;           2) The pseudo-code for OSCtxSw() is:
;              a) OSTaskSwHook();
;              b) OSPrioCur   = OSPrioHighRdy;
;              c) OSTCBCurPtr = OSTCBHighRdyPtr;
;              d) SP          = OSTCBHighRdyPtr->OSTCBStkPtr;
;              e) Restore the new task's context from the new task's stack,
;              f) Return to new task's code.
;
;           3) Upon entry:
;              OSTCBCurPtr      points to the OS_TCB of the task to suspend,
;              OSTCBHighRdyPtr  points to the OS_TCB of the task to resume.
;********************************************************************************************************

OSIntCtxSw:
    BL      OSTaskSwHook

    LDR     R0, OSPrioCurAddr                                   ; OSPrioCur = OSPrioHighRdy;
    LDR     R1, OSPrioHighRdyAddr
    LDRB    R2, [R1]
    STRB    R2, [R0]

    LDR     R0, OSTCBCurPtrAddr                                 ; OSTCBCurPtr = OSTCBHighRdyPtr;
    LDR     R1, OSTCBHighRdyPtrAddr
    LDR     R2, [R1]
    STR     R2, [R0]

    LDR     SP, [R2]                                            ; SP = OSTCBHighRdyPtr->OSTCBStkPtr;

                                                                ; RESTORE NEW TASK'S CONTEXT:
    LDMFD   SP!, {R0}                                           ;    Pop new task's CPSR,
    MSR     SPSR_cxsf, R0

    LDMFD   SP!, {R0-R12, LR, PC}^                              ;    Pop new task's context.


;********************************************************************************************************
;                               UNDEFINED INSTRUCTION EXCEPTION HANDLER
;
; Register Usage:  R0     Exception Type
;                  R1
;                  R2     Return PC
;********************************************************************************************************

OS_CPU_ARM_ExceptUndefInstrHndlr:
                                                                ; LR offset to return from this exception:  0.
    STMFD   SP!, {R0-R3}                                        ; Push working registers.
    MOV     R2, LR                                              ; Save link register.
    MOV     R0, #OS_CPU_ARM_EXCEPT_UNDEF_INSTR                  ; Set exception ID to OS_CPU_ARM_EXCEPT_UNDEF_INSTR.
    B            OS_CPU_ARM_ExceptHndlr                         ; Branch to global exception handler.


;********************************************************************************************************
;                                SOFTWARE INTERRUPT EXCEPTION HANDLER
;
; Register Usage:  R0     Exception Type
;                  R1
;                  R2     Return PC
;********************************************************************************************************

OS_CPU_ARM_ExceptSwiHndlr:
                                                                ; LR offset to return from this exception:  0.
    STMFD   SP!, {R0-R3}                                        ; Push working registers.
    MOV     R2, LR                                              ; Save link register.
    MOV     R0, #OS_CPU_ARM_EXCEPT_SWI                          ; Set exception ID to OS_CPU_ARM_EXCEPT_SWI.
    B            OS_CPU_ARM_ExceptHndlr                         ; Branch to global exception handler.


;********************************************************************************************************
;                                  PREFETCH ABORT EXCEPTION HANDLER
;
; Register Usage:  R0     Exception Type
;                  R1
;                  R2     Return PC
;********************************************************************************************************

OS_CPU_ARM_ExceptPrefetchAbortHndlr:
    SUB     LR, LR, #4                                          ; LR offset to return from this exception: -4.
    STMFD   SP!, {R0-R3}                                        ; Push working registers.
    MOV     R2, LR                                              ; Save link register.
    MOV     R0, #OS_CPU_ARM_EXCEPT_PREFETCH_ABORT               ; Set exception ID to OS_CPU_ARM_EXCEPT_PREFETCH_ABORT.
    B            OS_CPU_ARM_ExceptHndlr                         ; Branch to global exception handler.


;********************************************************************************************************
;                                    DATA ABORT EXCEPTION HANDLER
;
; Register Usage:  R0     Exception Type
;                  R1
;                  R2     Return PC
;********************************************************************************************************

OS_CPU_ARM_ExceptDataAbortHndlr:
    SUB     LR, LR, #8                                          ; LR offset to return from this exception: -8.
    STMFD   SP!, {R0-R3}                                        ; Push working registers.
    MOV     R2, LR                                              ; Save link register.
    MOV     R0, #OS_CPU_ARM_EXCEPT_DATA_ABORT                   ; Set exception ID to OS_CPU_ARM_EXCEPT_DATA_ABORT.
    B            OS_CPU_ARM_ExceptHndlr                         ; Branch to global exception handler.


;********************************************************************************************************
;                                   ADDRESS ABORT EXCEPTION HANDLER
;
; Register Usage:  R0     Exception Type
;                  R1
;                  R2     Return PC
;********************************************************************************************************

OS_CPU_ARM_ExceptAddrAbortHndlr:
    SUB     LR, LR, #8                                          ; LR offset to return from this exception: -8.
    STMFD   SP!, {R0-R3}                                        ; Push working registers.
    MOV     R2, LR                                              ; Save link register.
    MOV     R0, #OS_CPU_ARM_EXCEPT_ADDR_ABORT                   ; Set exception ID to OS_CPU_ARM_EXCEPT_ADDR_ABORT.
    B            OS_CPU_ARM_ExceptHndlr                         ; Branch to global exception handler.


;********************************************************************************************************
;                                 INTERRUPT REQUEST EXCEPTION HANDLER
;
; Register Usage:  R0     Exception Type
;                  R1
;                  R2     Return PC
;********************************************************************************************************

OS_CPU_ARM_ExceptIrqHndlr:
    SUB     LR, LR, #4                                          ; LR offset to return from this exception: -4.
    STMFD   SP!, {R0-R3}                                        ; Push working registers.
    MOV     R2, LR                                              ; Save link register.
    MOV     R0, #OS_CPU_ARM_EXCEPT_IRQ                          ; Set exception ID to OS_CPU_ARM_EXCEPT_IRQ.
    B            OS_CPU_ARM_ExceptHndlr                         ; Branch to global exception handler.


;********************************************************************************************************
;                              FAST INTERRUPT REQUEST EXCEPTION HANDLER
;
; Register Usage:  R0     Exception Type
;                  R1
;                  R2     Return PC
;********************************************************************************************************

OS_CPU_ARM_ExceptFiqHndlr:
    SUB     LR, LR, #4                                          ; LR offset to return from this exception: -4.
    STMFD   SP!, {R0-R3}                                        ; Push working registers.
    MOV     R2, LR                                              ; Save link register.
    MOV     R0, #OS_CPU_ARM_EXCEPT_FIQ                          ; Set exception ID to OS_CPU_ARM_EXCEPT_FIQ.
    B            OS_CPU_ARM_ExceptHndlr                         ; Branch to global exception handler.


;********************************************************************************************************
;                                      GLOBAL EXCEPTION HANDLER
;
; Register Usage:  R0     Exception Type
;                  R1     Exception's SPSR
;                  R2     Return PC
;                  R3     Exception's SP
;
; Note(s)       : 1) An exception can occur in three different circumstances; in each of these, the
;                    SVC stack pointer will point to a different entity :
;
;                    a) CONDITION: An exception occurs before the OS has been fully initialized.
;                       SVC STACK: Should point to a stack initialized by the application's startup code.
;                       STK USAGE: Interrupted context -- SVC stack.
;                                  Exception           -- SVC stack.
;                                  Nested exceptions   -- SVC stack.
;
;                    b) CONDITION: An exception interrupts a task.
;                       SVC STACK: Should point to task stack.
;                       STK USAGE: Interrupted context -- Task stack.
;                                  Exception           -- Exception stack 'OS_CPU_ExceptStk[]'.
;                                  Nested exceptions   -- Exception stack 'OS_CPU_ExceptStk[]'.
;
;                    c) CONDITION: An exception interrupts another exception.
;                       SVC STACK: Should point to location in exception stack, 'OS_CPU_ExceptStk[]'.
;                       STK USAGE: Interrupted context -- Exception stack 'OS_CPU_ExceptStk[]'.
;                                  Exception           -- Exception stack 'OS_CPU_ExceptStk[]'.
;                                  Nested exceptions   -- Exception stack 'OS_CPU_ExceptStk[]'.
;********************************************************************************************************

OS_CPU_ARM_ExceptHndlr:
    MRS     R1, SPSR                                            ; Save CPSR (i.e. exception's SPSR).
    MOV     R3, SP                                              ; Save exception's stack pointer.

                                                                ; Adjust exception stack pointer.  This is needed because
                                                                ; exception stack is not used when restoring task context.
    ADD     SP, SP, #(4 * 4)

                                                                ; Change to SVC mode & disable interruptions.
    MSR     CPSR_c, #(OS_CPU_ARM_CONTROL_INT_DIS | OS_CPU_ARM_MODE_SVC)
    CLREX                                                       ; Clear exclusive monitor.

    STMFD   SP!, {R2}                                           ;   Push task's PC,
    STMFD   SP!, {LR}                                           ;   Push task's LR,
    STMFD   SP!, {R4-R12}                                       ;   Push task's R12-R4,
    LDMFD   R3!, {R5-R8}                                        ;   Move task's R3-R0 from exception stack to task's stack.
    STMFD   SP!, {R5-R8}
    STMFD   SP!, {R1}                                           ;   Push task's CPSR (i.e. exception SPSR).

                                                                ; if (OSRunning == 1)
    LDR     R3, OSRunningAddr
    LDRB    R4, [R3]
    CMP     R4, #1
    BNE     OS_CPU_ARM_ExceptHndlr_BreakNothing

                                                                ; HANDLE NESTING COUNTER:
    LDR     R3, OSIntNestingCtrAddr                             ;   OSIntNestingCtr++;
    LDRB    R4, [R3]
    ADD     R4, R4, #1
    STRB    R4, [R3]

    CMP     R4, #1                                              ; if (OSIntNestingCtr == 1)
    BNE     OS_CPU_ARM_ExceptHndlr_BreakExcept


;********************************************************************************************************
;                                 EXCEPTION HANDLER: TASK INTERRUPTED
;
; Register Usage:  R0     Exception Type
;                  R1
;                  R2
;                  R3
;********************************************************************************************************

OS_CPU_ARM_ExceptHndlr_BreakTask:
    LDR     R3, OSTCBCurPtrAddr                                 ; OSTCBCurPtr->StkPtr = SP;
    LDR     R4, [R3]
    STR     SP, [R4]

    LDR     R3, OS_CPU_ExceptStkBaseAddr                        ; Switch to exception stack.
    LDR     SP, [R3]

                                                                ; EXECUTE EXCEPTION HANDLER:
    BL      OS_CPU_ExceptHndlr                                  ; OS_CPU_ExceptHndlr(except_type = R0)

                                                                ; Change to SVC mode & disable interruptions.
    MSR     CPSR_c, #(OS_CPU_ARM_CONTROL_INT_DIS | OS_CPU_ARM_MODE_SVC)

                                                                ; Call OSIntExit().  This call MAY never return if a ready
                                                                ; task with higher priority than the interrupted one is
                                                                ; found.
    BL      OSIntExit

    LDR     R3, OSTCBCurPtrAddr                                 ; SP = OSTCBCurPtr->StkPtr;
    LDR     R4, [R3]
    LDR     SP, [R4]

                                                                ; RESTORE NEW TASK'S CONTEXT:
    LDMFD   SP!, {R0}                                           ;    Pop new task's CPSR,
    MSR     SPSR_cxsf, R0

    LDMFD   SP!, {R0-R12, LR, PC}^                              ;    Pop new task's context.


;********************************************************************************************************
;                              EXCEPTION HANDLER: EXCEPTION INTERRUPTED
;
; Register Usage:  R0     Exception Type
;                  R1
;                  R2
;                  R3
;********************************************************************************************************

OS_CPU_ARM_ExceptHndlr_BreakExcept:

    MOV     R1, SP
    AND     R1, R1, #4
    SUB     SP, SP, R1
    STMFD   SP!, {R1, LR}
                                                                ; EXECUTE EXCEPTION HANDLER:
    BL      OS_CPU_ExceptHndlr                                  ; OS_CPU_ExceptHndlr(except_type = R0)

    LDMIA   SP!, {R1, LR}
    ADD     SP, SP, R1

                                                                ; Change to SVC mode & disable interruptions.
    MSR     CPSR_c, #(OS_CPU_ARM_CONTROL_INT_DIS | OS_CPU_ARM_MODE_SVC)

                                                                ; HANDLE NESTING COUNTER:
    LDR     R3, OSIntNestingCtrAddr                             ;   OSIntNestingCtr--;
    LDRB    R4, [R3]
    SUB     R4, R4, #1
    STRB    R4, [R3]

                                                                ; RESTORE OLD CONTEXT:
    LDMFD   SP!, {R0}                                           ;    Pop old CPSR,
    MSR     SPSR_cxsf, R0

    LDMFD   SP!, {R0-R12, LR, PC}^                              ;   Pull working registers and return from exception.


;********************************************************************************************************
;                              EXCEPTION HANDLER: 'NOTHING' INTERRUPTED
;
; Register Usage:  R0     Exception Type
;                  R1
;                  R2
;                  R3
;********************************************************************************************************

OS_CPU_ARM_ExceptHndlr_BreakNothing:

    MOV     R1, SP
    AND     R1, R1, #4
    SUB     SP, SP, R1
    STMFD   SP!, {R1, LR}

                                                                ; EXECUTE EXCEPTION HANDLER:
    BL      OS_CPU_ExceptHndlr                                  ; OS_CPU_ExceptHndlr(except_type = R0)

    LDMIA   SP!, {R1, LR}
    ADD     SP, SP, R1

                                                                ; Change to SVC mode & disable interruptions.
    MSR     CPSR_c, #(OS_CPU_ARM_CONTROL_INT_DIS | OS_CPU_ARM_MODE_SVC)

                                                                ; RESTORE OLD CONTEXT:
    LDMFD   SP!, {R0}                                           ;   Pop old CPSR,
    MSR     SPSR_cxsf, R0

    LDMFD   SP!, {R0-R12, LR, PC}^                              ;   Pull working registers and return from exception.


;********************************************************************************************************
;                              VFP/NEON REGISTER COUNT
;
; Register Usage:  R0     Double Register Count
;********************************************************************************************************

OS_CPU_ARM_DRegCntGet:
    MOV     R0, #0
    BX      LR

    .end
