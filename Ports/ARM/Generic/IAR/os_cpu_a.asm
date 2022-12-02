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
;                                           Generic ARM Port
;
; File      : os_cpu_a.asm
; Version   : V3.08.02
;********************************************************************************************************
; For       : ARM7 or ARM9
; Mode      : ARM  or Thumb
; Toolchain : IAR EWARM V5.xx and higher
;********************************************************************************************************

;********************************************************************************************************
;                                          PUBLIC FUNCTIONS
;********************************************************************************************************
                                                                ; External references.
    EXTERN  OSRunning
    EXTERN  OSPrioCur
    EXTERN  OSPrioHighRdy
    EXTERN  OSTCBCurPtr
    EXTERN  OSTCBHighRdyPtr
    EXTERN  OSIntNestingCtr
    EXTERN  OSIntExit
    EXTERN  OSTaskSwHook

    EXTERN  OS_CPU_ExceptStkBase
    EXTERN  OS_CPU_ExceptStkPtr

                                                                ; Functions declared in this file.
    PUBLIC  OSStartHighRdy
    PUBLIC  OSCtxSw
    PUBLIC  OSIntCtxSw

                                                                ; Functions related to exception handling.
    PUBLIC  OS_CPU_ARM_ExceptUndefInstrHndlr
    PUBLIC  OS_CPU_ARM_ExceptSwiHndlr
    PUBLIC  OS_CPU_ARM_ExceptPrefetchAbortHndlr
    PUBLIC  OS_CPU_ARM_ExceptDataAbortHndlr
    PUBLIC  OS_CPU_ARM_ExceptIrqHndlr
    PUBLIC  OS_CPU_ARM_ExceptFiqHndlr

    EXTERN  OS_CPU_ExceptHndlr                                  ; Chip Support specific exception handler.

;PAGE
;********************************************************************************************************
;                                               EQUATES
;********************************************************************************************************

OS_CPU_ARM_CONTROL_INT_DIS        EQU  0xC0                     ; Disable both FIQ and IRQ.
OS_CPU_ARM_CONTROL_FIQ_DIS        EQU  0x40                     ; Disable FIQ.
OS_CPU_ARM_CONTROL_IRQ_DIS        EQU  0x80                     ; Disable IRQ.
OS_CPU_ARM_CONTROL_THUMB          EQU  0x20                     ; Set THUMB mode.
OS_CPU_ARM_CONTROL_ARM            EQU  0x00                     ; Set ARM mode.

OS_CPU_ARM_MODE_MASK              EQU  0x1F
OS_CPU_ARM_MODE_USR               EQU  0x10
OS_CPU_ARM_MODE_FIQ               EQU  0x11
OS_CPU_ARM_MODE_IRQ               EQU  0x12
OS_CPU_ARM_MODE_SVC               EQU  0x13
OS_CPU_ARM_MODE_ABT               EQU  0x17
OS_CPU_ARM_MODE_UND               EQU  0x1B
OS_CPU_ARM_MODE_SYS               EQU  0x1F

OS_CPU_ARM_EXCEPT_RESET           EQU  0x00
OS_CPU_ARM_EXCEPT_UNDEF_INSTR     EQU  0x01
OS_CPU_ARM_EXCEPT_SWI             EQU  0x02
OS_CPU_ARM_EXCEPT_PREFETCH_ABORT  EQU  0x03
OS_CPU_ARM_EXCEPT_DATA_ABORT      EQU  0x04
OS_CPU_ARM_EXCEPT_IRQ             EQU  0x06
OS_CPU_ARM_EXCEPT_FIQ             EQU  0x07


;********************************************************************************************************
;                                     CODE GENERATION DIRECTIVES
;********************************************************************************************************

    RSEG CODE:CODE:NOROOT(2)
    CODE32


;PAGE
;********************************************************************************************************
;                                         START MULTITASKING
;                                      void OSStartHighRdy(void)
;
; Note(s) : 1) OSStartHighRdy() MUST:
;              a) Call OSTaskSwHook() then,
;              b) Set OSRunning to OS_STATE_OS_RUNNING,
;              c) Switch to the highest priority task.
;********************************************************************************************************

OSStartHighRdy
                                                                ; Change to SVC mode.
    MSR     CPSR_c, #(OS_CPU_ARM_CONTROL_INT_DIS | OS_CPU_ARM_MODE_SVC)

    LDR     R0, =OSTaskSwHook                                   ; OSTaskSwHook();
    MOV     LR, PC
    BX      R0

                                                                ; SWITCH TO HIGHEST PRIORITY TASK:
    LDR     R0, =OSTCBHighRdyPtr                                ;    Get highest priority task TCB address,
    LDR     R0, [R0]                                            ;    Get stack pointer,
    LDR     SP, [R0]                                            ;    Switch to the new stack,

    LDR     R0, [SP], #4                                        ;    Pop new task's CPSR,
    MSR     SPSR_cxsf, R0

    LDMFD   SP!, {R0-R12, LR, PC}^                              ;    Pop new task's context.


;PAGE
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

OSCtxSw
                                                                ; SAVE CURRENT TASK'S CONTEXT:
    STMFD   SP!, {LR}                                           ;     Push return address,
    STMFD   SP!, {LR}
    STMFD   SP!, {R0-R12}                                       ;     Push registers,
    MRS     R0, CPSR                                            ;     Push current CPSR,
    TST     LR, #1                                              ;     See if called from Thumb mode,
    ORRNE   R0, R0, #OS_CPU_ARM_CONTROL_THUMB                   ;     If yes, set the T-bit.
    STMFD   SP!, {R0}

    LDR     R0, =OSTCBCurPtr                                    ; OSTCBCurPtr->StkPtr = SP;
    LDR     R1, [R0]
    STR     SP, [R1]

    LDR     R0, =OSTaskSwHook                                   ; OSTaskSwHook();
    MOV     LR, PC
    BX      R0

    LDR     R0, =OSPrioCur                                      ; OSPrioCur   = OSPrioHighRdy;
    LDR     R1, =OSPrioHighRdy
    LDRB    R2, [R1]
    STRB    R2, [R0]

    LDR     R0, =OSTCBCurPtr                                    ; OSTCBCurPtr = OSTCBHighRdyPtr;
    LDR     R1, =OSTCBHighRdyPtr
    LDR     R2, [R1]
    STR     R2, [R0]

    LDR     SP, [R2]                                            ; SP = OSTCBHighRdyPtr->OSTCBStkPtr;

                                                                ; RESTORE NEW TASK'S CONTEXT:
    LDMFD   SP!, {R0}                                           ;    Pop new task's CPSR,
    MSR     SPSR_cxsf, R0

    LDMFD   SP!, {R0-R12, LR, PC}^                              ;    Pop new task's context.


;PAGE
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

OSIntCtxSw
    LDR     R0, =OSTaskSwHook                                   ; OSTaskSwHook();
    MOV     LR, PC
    BX      R0

    LDR     R0, =OSPrioCur                                      ; OSPrioCur = OSPrioHighRdy;
    LDR     R1, =OSPrioHighRdy
    LDRB    R2, [R1]
    STRB    R2, [R0]

    LDR     R0, =OSTCBCurPtr                                    ; OSTCBCurPtr = OSTCBHighRdyPtr;
    LDR     R1, =OSTCBHighRdyPtr
    LDR     R2, [R1]
    STR     R2, [R0]

    LDR     SP, [R2]                                            ; SP = OSTCBHighRdyPtr->OSTCBStkPtr;

                                                                ; RESTORE NEW TASK'S CONTEXT:
    LDMFD   SP!, {R0}                                           ;    Pop new task's CPSR,
    MSR     SPSR_cxsf, R0

    LDMFD   SP!, {R0-R12, LR, PC}^                              ;    Pop new task's context.


;PAGE
;********************************************************************************************************
;                               UNDEFINED INSTRUCTION EXCEPTION HANDLER
;
; Register Usage:  R0     Exception Type
;                  R1
;                  R2     Return PC
;********************************************************************************************************

OS_CPU_ARM_ExceptUndefInstrHndlr
                                                                ; LR offset to return from this exception:  0.
    STMFD   SP!, {R0-R12, LR}                                   ; Push working registers.
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

OS_CPU_ARM_ExceptSwiHndlr
                                                                ; LR offset to return from this exception:  0.
    STMFD   SP!, {R0-R12, LR}                                   ; Push working registers.
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

OS_CPU_ARM_ExceptPrefetchAbortHndlr
    SUB     LR, LR, #4                                          ; LR offset to return from this exception: -4.
    STMFD   SP!, {R0-R12, LR}                                   ; Push working registers.
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

OS_CPU_ARM_ExceptDataAbortHndlr
    SUB     LR, LR, #8                                          ; LR offset to return from this exception: -8.
    STMFD   SP!, {R0-R12, LR}                                   ; Push working registers.
    MOV     R2, LR                                              ; Save link register.
    MOV     R0, #OS_CPU_ARM_EXCEPT_DATA_ABORT                   ; Set exception ID to OS_CPU_ARM_EXCEPT_DATA_ABORT.
    B            OS_CPU_ARM_ExceptHndlr                         ; Branch to global exception handler.


;PAGE
;********************************************************************************************************
;                                 INTERRUPT REQUEST EXCEPTION HANDLER
;
; Register Usage:  R0     Exception Type
;                  R1
;                  R2     Return PC
;********************************************************************************************************

OS_CPU_ARM_ExceptFiqHndlr
    SUB     LR, LR, #4                                          ; LR offset to return from this exception: -4.
    STMFD   SP!, {R0-R12, LR}                                   ; Push working registers.
    MOV     R2, LR                                              ; Save link register.
    MOV     R0, #OS_CPU_ARM_EXCEPT_FIQ                          ; Set exception ID to OS_CPU_ARM_EXCEPT_FIQ.
    B            OS_CPU_ARM_ExceptHndlr                         ; Branch to global exception handler.

;********************************************************************************************************
;********************************************************************************************************
;                                       GLOBAL EXCEPTION HANDLER
;********************************************************************************************************
;********************************************************************************************************

;********************************************************************************************************
;                                       GLOBAL EXCEPTION HANDLER
;
; Register Usage:  R0     Exception Type
;                  R1	  Exception's SPSR
;                  R2     Return PC
;                  R3     Old CPU mode
;
; Note(s)       : 1) An exception can occur in four different circumstances; in each of these, the
;                    SVC stack pointer will point to a different entity :
;
;                    a) CONDITION: An exception occurs before the OS has been fully initialized.
;                       SVC STACK: Should point to a stack initialized by the application's startup code.
;
;                    b) CONDITION: An exception interrupts a task.
;                       SVC STACK: Should point to task stack.
;
;                    c) CONDITION: An exception interrupts another exception, or an IRQ before it
;                                  switches to the exception stack.
;                       SVC STACK: Should point to location in an exception-mode stack.
;
;                    d) CONDITION: An exception interrupts an an IRQ after it switches to the exception
;                                  stack, 'OS_CPU_ExceptStk[]'.
;                       SVC STACK: Should point to location in an exception stack, 'OS_CPU_ExceptStk[]'.
;********************************************************************************************************

OS_CPU_ARM_ExceptHndlr
    MRS     R1, SPSR                                            ; Save CPSR (i.e. exception's SPSR).

                                                                ; DETERMINE IF WE INTERRUPTED A TASK/IRQ OR ANOTHER LOWER PRIORITY EXCEPTION:
                                                                ;   SPSR.Mode = SVC                :  task or IRQ handled in SVC mode,
                                                                ;   SPSR.Mode = FIQ, IRQ, ABT, UND :  other exceptions,
                                                                ;   SPSR.Mode = USR                : *unsupported state*.
    AND     R3, R1, #OS_CPU_ARM_MODE_MASK
    CMP     R3,     #OS_CPU_ARM_MODE_SVC
    BNE     OS_CPU_ARM_ExceptHndlr_BrkExcept

;********************************************************************************************************
;                                  EXCEPTION HANDLER: TASK INTERRUPTED
;
; Register Usage:  R0     Exception Type
;                  R1	  Exception's SPSR
;                  R2     Return PC
;                  R3	  Exception's CPSR
;                  R4     Exception's SP
;********************************************************************************************************

OS_CPU_ARM_ExceptHndlr_BrkTask
    MRS     R3, CPSR                                            ; Save exception's CPSR.
    MOV     R4, SP                                              ; Save exception's stack pointer.

                                                                ; Change to SVC mode & disable interruptions.
    MSR     CPSR_c, #(OS_CPU_ARM_CONTROL_INT_DIS | OS_CPU_ARM_MODE_SVC)

                                                                ; SAVE TASK'S CONTEXT ONTO TASK'S STACK:
    STMFD   SP!, {R2}                                           ;   Push task's PC,
    STMFD   SP!, {LR}                                           ;   Push task's LR,
    STMFD   SP!, {R5-R12}                                       ;   Push task's R12-R5,
    LDMFD   R4!, {R5-R9}                                        ;   Move task's R4-R0 from exception stack to task's stack.
    STMFD   SP!, {R5-R9}
    STMFD   SP!, {R1}                                           ;   Push task's CPSR (i.e. exception SPSR).

                                                                ; if (OSRunning == 1)
    LDR     R1, =OSRunning
    LDRB    R1, [R1]
    CMP     R1, #1
    BNE     OS_CPU_ARM_ExceptHndlr_BrkTask_1

                                                                ; HANDLE NESTING COUNTER:
    LDR     R1, =OSIntNestingCtr                                ;   OSIntNestingCtr++;
    LDRB    R2, [R1]
    ADD     R2, R2, #1
    STRB    R2, [R1]

    CMP     R2, #1                                              ;   if (OSIntNestingCtr > 1)
    BNE     OS_CPU_ARM_ExceptHndlr_BrkIRQ                       ;       IRQ has been interrupted.

    LDR     R1, =OSTCBCurPtr                                    ;   OSTCBCurPtr->StkPtr = SP;
    LDR     R2, [R1]
    STR     SP, [R2]

OS_CPU_ARM_ExceptHndlr_BrkTask_1
    MSR     CPSR_cxsf, R3                                       ; RESTORE INTERRUPTED MODE.

                                                                ; EXECUTE EXCEPTION HANDLER:
    LDR     R1, =OS_CPU_ExceptHndlr                             ; OS_CPU_ExceptHndlr(except_type = R0);
    MOV     LR, PC
    BX      R1

                                                                ; Adjust exception stack pointer.  This is needed because
                                                                ; exception stack is not used when restoring task context.
    ADD     SP, SP, #(14 * 4)

                                                                ; Change to SVC mode & disable interruptions.
    MSR     CPSR_c, #(OS_CPU_ARM_CONTROL_INT_DIS | OS_CPU_ARM_MODE_SVC)

                                                                ; Call OSIntExit().  This call MAY never return if a ready
                                                                ; task with higher priority than the interrupted one is
                                                                ; found.
    LDR     R0, =OSIntExit
    MOV     LR, PC
    BX      R0

                                                                ; RESTORE NEW TASK'S CONTEXT:
    LDMFD   SP!, {R0}                                           ;    Pop new task's CPSR,
    MSR     SPSR_cxsf, R0

    LDMFD   SP!, {R0-R12, LR, PC}^                              ;    Pop new task's context.

;********************************************************************************************************
;                               EXCEPTION HANDLER: EXCEPTION INTERRUPTED
;
; Register Usage:  R0     Exception Type
;                  R1
;                  R2
;                  R3
;********************************************************************************************************

OS_CPU_ARM_ExceptHndlr_BrkExcept
    STMFD   SP!, {R1}                                           ; Push exception's SPSR.

    MRS     R3, CPSR                                            ; Push exception's CPSR.
                                                                ; Change to SVC mode & disable interruptions.
    MSR     CPSR_c, #(OS_CPU_ARM_CONTROL_INT_DIS | OS_CPU_ARM_MODE_SVC)

    STMFD   SP!, {R3}

                                                                ; HANDLE NESTING COUNTER:
    LDR     R2, =OSIntNestingCtr                                ;   OSIntNestingCtr++;
    LDRB    R4, [R2]
    ADD     R4, R4, #1
    STRB    R4, [R2]

    MSR     CPSR_cxsf, R3                                       ; RESTORE INTERRUPTED MODE.

                                                                ; EXECUTE EXCEPTION HANDLER:
    LDR     R2, =OS_CPU_ExceptHndlr                             ; OS_CPU_ExceptHndlr(except_type = R0);
    MOV     LR, PC
    BX      R2

                                                                ; Change to SVC mode & disable interruptions.
    MSR     CPSR_c, #(OS_CPU_ARM_CONTROL_INT_DIS | OS_CPU_ARM_MODE_SVC)

                                                                ; HANDLE NESTING COUNTER:
    LDR     R2, =OSIntNestingCtr                                ;   OSIntNestingCtr--;
    LDRB    R4, [R2]
    SUB     R4, R4, #1
    STRB    R4, [R2]

    LDMFD   SP!, {R3}
    MSR     CPSR_cxsf, R3                                       ; RESTORE INTERRUPTED MODE.

                                                                ; RESTORE INTERRUPTED EXCEPTIONS' CONTEXT:
    LDMFD   SP!, {R0}                                           ;    Pop exception's CPSR,
    MSR     SPSR_cxsf, R0

    LDMFD   SP!, {R0-R12, PC}^                                  ;   Pull working registers and return from exception.

;********************************************************************************************************
;                                   EXCEPTION HANDLER: IRQ INTERRUPTED
;
; Register Usage:  R0     Exception Type
;                  R1
;                  R2
;                  R3
;********************************************************************************************************

OS_CPU_ARM_ExceptHndlr_BrkIRQ
    MSR     CPSR_cxsf, R3                                       ; RESTORE INTERRUPTED MODE.

                                                                ; EXECUTE EXCEPTION HANDLER:
    LDR     R1, =OS_CPU_ExceptHndlr                             ; OS_CPU_ExceptHndlr(except_type = R0);
    MOV     LR, PC
    BX      R1

                                                                ; Adjust exception stack pointer.  This is needed because
                                                                ; exception stack is not used when restoring IRQ context.
    ADD     SP, SP, #(14 * 4)

                                                                ; Change to SVC mode & disable interruptions.
    MSR     CPSR_c, #(OS_CPU_ARM_CONTROL_INT_DIS | OS_CPU_ARM_MODE_SVC)

                                                                ; HANDLE NESTING COUNTER:
    LDR     R2, =OSIntNestingCtr                                ;   OSIntNestingCtr--;
    LDRB    R4, [R2]
    SUB     R4, R4, #1
    STRB    R4, [R2]

                                                                ; RESTORE IRQ'S CONTEXT:
    LDMFD   SP!, {R0}                                           ;    Pop IRQ's CPSR,
    MSR     SPSR_cxsf, R0

    LDMFD   SP!, {R0-R12, LR, PC}^                              ;    Pop IRQ's context.

;********************************************************************************************************
;********************************************************************************************************
;                                              IRQ HANDLER
;********************************************************************************************************
;********************************************************************************************************

;********************************************************************************************************
;                                  INTERRUPT REQUEST EXCEPTION HANDLER
;
; Register Usage:  R0     Exception Type
;                  R1     Exception's SPSR
;                  R2     Return PC
;                  R3     Exception's SP
;
; Note(s)       : 1) An IRQ can occur in three different circumstances; in each of these, the
;                    SVC stack pointer will point to a different entity :
;
;                    a) CONDITION: An exception occurs before the OS has been fully initialized.
;                       SVC STACK: Should point to a stack initialized by the application's startup code.
;                       STK USAGE: Interrupted context -- SVC stack.
;                                  Exception           -- SVC stack.
;                                  Nested exceptions   -- SVC stack.
;
;                    b) CONDITION: An IRQ interrupts a task.
;                       SVC STACK: Should point to task stack.
;                       STK USAGE: Interrupted context -- Task stack.
;                                  Exception           -- Exception stack 'OS_CPU_ExceptStk[]'.
;                                  Nested exceptions   -- Exception stack 'OS_CPU_ExceptStk[]'.
;
;                    c) CONDITION: An IRQ interrupts another IRQ.
;                       SVC STACK: Should point to location in exception stack, 'OS_CPU_ExceptStk[]'.
;                       STK USAGE: Interrupted context -- Exception stack 'OS_CPU_ExceptStk[]'.
;                                  Exception           -- Exception stack 'OS_CPU_ExceptStk[]'.
;                                  Nested exceptions   -- Exception stack 'OS_CPU_ExceptStk[]'.
;********************************************************************************************************

OS_CPU_ARM_ExceptIrqHndlr
    SUB     LR, LR, #4                                          ; LR offset to return from this exception: -4.
    STMFD   SP!, {R0-R3}                                        ; Push working registers.

    MOV     R0, #OS_CPU_ARM_EXCEPT_IRQ                          ; Set exception ID to OS_CPU_ARM_EXCEPT_IRQ.
    MRS     R1, SPSR                                            ; Save CPSR (i.e. exception's SPSR).
    MOV     R2, LR                                              ; Save link register.
    MOV     R3, SP                                              ; Save exception's stack pointer.

                                                                ; Change to SVC mode & disable interruptions.
    MSR     CPSR_c, #(OS_CPU_ARM_CONTROL_INT_DIS | OS_CPU_ARM_MODE_SVC)

                                                                ; SAVE CONTEXT ONTO SVC STACK:
    STMFD   SP!, {R2}                                           ;   Push task's PC,
    STMFD   SP!, {LR}                                           ;   Push task's LR,
    STMFD   SP!, {R4-R12}                                       ;   Push task's R12-R4,
    LDMFD   R3!, {R5-R8}                                        ;   Move task's R3-R0 from exception stack to task's stack.
    STMFD   SP!, {R5-R8}
    STMFD   SP!, {R1}                                           ;   Push task's CPSR (i.e. exception SPSR).

                                                                ; if (OSRunning == 1)
    LDR     R3, =OSRunning
    LDRB    R4, [R3]
    CMP     R4, #1
    BNE     OS_CPU_ARM_IRQHndlr_BreakNothing

                                                                ; HANDLE NESTING COUNTER:
    LDR     R3, =OSIntNestingCtr                                ;   OSIntNestingCtr++;
    LDRB    R4, [R3]
    ADD     R4, R4, #1
    STRB    R4, [R3]

    CMP     R4, #1                                              ; if (OSIntNestingCtr == 1)
    BNE     OS_CPU_ARM_IRQHndlr_BreakIRQ


;PAGE
;********************************************************************************************************
;                                      IRQ HANDLER: TASK INTERRUPTED
;
; Register Usage:  R0     Exception Type
;                  R1
;                  R2
;                  R3
;********************************************************************************************************

OS_CPU_ARM_IRQHndlr_BreakTask
    LDR     R3, =OSTCBCurPtr                                    ; OSTCBCurPtr->StkPtr = SP;
    LDR     R4, [R3]
    STR     SP, [R4]

    LDR     R3, =OS_CPU_ExceptStkBase                           ; Switch to exception stack.
    LDR     SP, [R3]

                                                                ; EXECUTE EXCEPTION HANDLER:
    LDR     R1, =OS_CPU_ExceptHndlr                             ; OS_CPU_ExceptHndlr(except_type = R0)
    MOV     LR, PC
    BX      R1

                                                                ; Change to IRQ mode & disable interruptions.
    MSR     CPSR_c, #(OS_CPU_ARM_CONTROL_INT_DIS | OS_CPU_ARM_MODE_IRQ)

                                                                ; Adjust exception stack pointer.  This is needed because
                                                                ; exception stack is not used when restoring task context.
    ADD     SP, SP, #(4 * 4)


                                                                ; Change to SVC mode & disable interruptions.
    MSR     CPSR_c, #(OS_CPU_ARM_CONTROL_INT_DIS | OS_CPU_ARM_MODE_SVC)

                                                                ; Call OSIntExit().  This call MAY never return if a ready
                                                                ; task with higher priority than the interrupted one is
                                                                ; found.
    LDR     R0, =OSIntExit
    MOV     LR, PC
    BX      R0

    LDR     R3, =OSTCBCurPtr                                    ; SP = OSTCBCurPtr->StkPtr;
    LDR     R4,  [R3]
    LDR     SP,  [R4]
                                                                ; RESTORE NEW TASK'S CONTEXT:
    LDMFD   SP!, {R0}                                           ;    Pop new task's CPSR,
    MSR     SPSR_cxsf, R0

    LDMFD   SP!, {R0-R12, LR, PC}^                              ;    Pop new task's context.


;PAGE
;********************************************************************************************************
;                                      IRQ HANDLER: IRQ INTERRUPTED
;
; Register Usage:  R0     Exception Type
;                  R1
;                  R2
;                  R3
;********************************************************************************************************

OS_CPU_ARM_IRQHndlr_BreakIRQ
    LDR     R3, =OS_CPU_ExceptStkPtr                            ; OS_CPU_ExceptStkPtr = SP;
    STR     SP, [R3]

                                                                ; EXECUTE EXCEPTION HANDLER:
    LDR     R3, =OS_CPU_ExceptHndlr                             ; OS_CPU_ExceptHndlr(except_type = R0)
    MOV     LR, PC
    BX      R3

                                                                ; Change to IRQ mode & disable interruptions.
    MSR     CPSR_c, #(OS_CPU_ARM_CONTROL_INT_DIS | OS_CPU_ARM_MODE_IRQ)

                                                                ; Adjust exception stack pointer.  This is needed because
                                                                ; exception stack is not used when restoring task context.
    ADD     SP, SP, #(4 * 4)

                                                                ; Change to SVC mode & disable interruptions.
    MSR     CPSR_c, #(OS_CPU_ARM_CONTROL_INT_DIS | OS_CPU_ARM_MODE_SVC)

                                                                ; HANDLE NESTING COUNTER:
    LDR     R3, =OSIntNestingCtr                                ;   OSIntNestingCtr--;
    LDRB    R4, [R3]
    SUB     R4, R4, #1
    STRB    R4, [R3]

                                                                ; RESTORE OLD CONTEXT:
    LDMFD   SP!, {R0}                                           ;    Pop old CPSR,
    MSR     SPSR_cxsf, R0

    LDMFD   SP!, {R0-R12, LR, PC}^                              ;   Pull working registers and return from exception.


;PAGE
;********************************************************************************************************
;                                   IRQ HANDLER: 'NOTHING' INTERRUPTED
;
; Register Usage:  R0     Exception Type
;                  R1
;                  R2
;                  R3
;********************************************************************************************************

OS_CPU_ARM_IRQHndlr_BreakNothing
                                                                ; EXECUTE EXCEPTION HANDLER:
    LDR     R3, =OS_CPU_ExceptHndlr                             ; OS_CPU_ExceptHndlr(except_type = R0)
    MOV     LR, PC
    BX      R3

                                                                ; Change to IRQ mode & disable interruptions.
    MSR     CPSR_c, #(OS_CPU_ARM_CONTROL_INT_DIS | OS_CPU_ARM_MODE_IRQ)

                                                                ; Adjust exception stack pointer.  This is needed because
                                                                ; exception stack is not used when restoring task context.
    ADD     SP, SP, #(4 * 4)

                                                                ; Change to SVC mode & disable interruptions.
    MSR     CPSR_c, #(OS_CPU_ARM_CONTROL_INT_DIS | OS_CPU_ARM_MODE_SVC)

                                                                ; RESTORE OLD CONTEXT:
    LDMFD   SP!, {R0}                                           ;   Pop old CPSR,
    MSR     SPSR_cxsf, R0

    LDMFD   SP!, {R0-R12, LR, PC}^                              ;   Pull working registers and return from exception.

    END
