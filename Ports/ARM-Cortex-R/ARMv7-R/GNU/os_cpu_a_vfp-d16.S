@********************************************************************************************************
@                                              uC/OS-III
@                                        The Real-Time Kernel
@
@                    Copyright 2009-2020 Silicon Laboratories Inc. www.silabs.com
@
@                                 SPDX-License-Identifier: APACHE-2.0
@
@               This software is subject to an open source license and is distributed by
@                Silicon Laboratories Inc. pursuant to the terms of the Apache License,
@                    Version 2.0 available at www.apache.org/licenses/LICENSE-2.0.
@
@********************************************************************************************************

@********************************************************************************************************
@
@                                             ARMv7-R Port
@
@ File      : os_cpu_a_vfp-d16.S
@ Version   : V3.08.01
@********************************************************************************************************
@ For       : ARMv7-R Cortex-R
@ Mode      : ARM or Thumb
@ Toolchain : GNU
@********************************************************************************************************
@ Note(s)   : (1) See Note #2 of os_cpu.h for important informations about this file.
@********************************************************************************************************

@********************************************************************************************************
@                                            FUNCTIONS
@********************************************************************************************************
                                                                @ .external references.
    .extern  OSRunning
    .extern  OSPrioCur
    .extern  OSPrioHighRdy
    .extern  OSTCBCurPtr
    .extern  OSTCBHighRdyPtr
    .extern  OSIntNestingCtr
    .extern  OSIntExit
    .extern  OSTaskSwHook
    .extern  OS_CPU_ExceptHndlr                                 @ Chip Support/BSP specific exception handler.

    .extern  OS_CPU_ExceptStkBase

                                                                @ Functions declared in this file.
    .global  OSStartHighRdy
    .global  OSCtxSw
    .global  OSIntCtxSw

                                                                @ Functions related to exception handling.
    .global  OS_CPU_ARM_ExceptUndefInstrHndlr
    .global  OS_CPU_ARM_ExceptSwiHndlr
    .global  OS_CPU_ARM_ExceptPrefetchAbortHndlr
    .global  OS_CPU_ARM_ExceptDataAbortHndlr
    .global  OS_CPU_ARM_ExceptIrqHndlr
    .global  OS_CPU_ARM_ExceptFiqHndlr

    .global  OS_CPU_ARM_DRegCntGet


@********************************************************************************************************
@                                               EQUATES
@********************************************************************************************************

    .equ     OS_CPU_ARM_CONTROL_INT_DIS,        0xC0                    @ Disable both FIQ and IRQ.
    .equ     OS_CPU_ARM_CONTROL_FIQ_DIS,        0x40                    @ Disable FIQ.
    .equ     OS_CPU_ARM_CONTROL_IRQ_DIS,        0x80                    @ Disable IRQ.
    .equ     OS_CPU_ARM_CONTROL_THUMB,          0x20                    @ Set THUMB mode.
    .equ     OS_CPU_ARM_CONTROL_ARM,            0x00                    @ Set ARM mode.

    .equ     OS_CPU_ARM_MODE_MASK,              0x1F
    .equ     OS_CPU_ARM_MODE_USR,               0x10
    .equ     OS_CPU_ARM_MODE_FIQ,               0x11
    .equ     OS_CPU_ARM_MODE_IRQ,               0x12
    .equ     OS_CPU_ARM_MODE_SVC,               0x13
    .equ     OS_CPU_ARM_MODE_ABT,               0x17
    .equ     OS_CPU_ARM_MODE_UND,               0x1B
    .equ     OS_CPU_ARM_MODE_SYS,               0x1F

    .equ     OS_CPU_ARM_EXCEPT_RESET,           0x00
    .equ     OS_CPU_ARM_EXCEPT_UNDEF_INSTR,     0x01
    .equ     OS_CPU_ARM_EXCEPT_SWI,             0x02
    .equ     OS_CPU_ARM_EXCEPT_PREFETCH_ABORT,  0x03
    .equ     OS_CPU_ARM_EXCEPT_DATA_ABORT,      0x04
    .equ     OS_CPU_ARM_EXCEPT_ADDR_ABORT,      0x05
    .equ     OS_CPU_ARM_EXCEPT_IRQ,             0x06
    .equ     OS_CPU_ARM_EXCEPT_FIQ,             0x07

    .equ     OS_CPU_ARM_FPEXC_EN,               0x40000000               @VFP enable bit.


@********************************************************************************************************
@                                     CODE GENERATION DIRECTIVES
@********************************************************************************************************

    .code 32


@********************************************************************************************************
@                                 FLOATING POINT REGISTER MACROS
@********************************************************************************************************

    .macro  OS_CPU_ARM_FP_REG_POP     rx
            POP     {\rx}
            VMSR    FPEXC, \rx                                  @    ... Pop new task's FPEXC
            FLDMIAD SP!, {D0-D15}                               @    ... Pop new task's General-Purpose floating point registers.
            POP     {\rx}
            VMSR    FPSCR, \rx                                  @    ... Pop new task's FPSCR.
    .endm

    .macro  OS_CPU_ARM_FP_REG_PUSH    rx
            VMRS    \rx, FPSCR                                  @     ... Save current FPSCR
            PUSH    {\rx}                                       @     ... Save general-purpose floating-point registers.
            FSTMDBD SP!, {D0-D15}
            VMRS    \rx, FPEXC                                  @     ... Save Floating point exception register.
            PUSH    {\rx}
    .endm


@********************************************************************************************************
@                                         START MULTITASKING
@                                      void OSStartHighRdy(void)
@
@ Note(s) : 1) OSStartHighRdy() MUST:
@              a) Call OSTaskSwHook() then,
@              b) Set OSRunning to OS_STATE_OS_RUNNING,
@              c) Switch to the highest priority task.
@********************************************************************************************************

    .type   OSStartHighRdy, %function
OSStartHighRdy:
                                                                @ Change to SVC mode.
    MSR     CPSR_c, #(OS_CPU_ARM_CONTROL_INT_DIS | OS_CPU_ARM_MODE_SVC)
    CLREX                                                       @ Clear exclusive monitor.

    BL      OSTaskSwHook                                        @ OSTaskSwHook();

                                                                @ SWITCH TO HIGHEST PRIORITY TASK:
    MOVW    R0, #:lower16:OSTCBHighRdyPtr                       @    Get highest priority task TCB address,
    MOVT    R0, #:upper16:OSTCBHighRdyPtr
    LDR     R0, [R0]                                            @    Get stack pointer,
    LDR     SP, [R0]                                            @    Switch to the new stack,

    OS_CPU_ARM_FP_REG_POP R0

    LDR     R0, [SP], #4                                        @    Pop new task's CPSR,
    MSR     SPSR_cxsf, R0

    LDMFD   SP!, {R0-R12, LR, PC}^                              @    Pop new task's context.


@********************************************************************************************************
@                       PERFORM A CONTEXT SWITCH (From task level) - OSCtxSw()
@
@ Note(s) : 1) OSCtxSw() is called in SVC mode with BOTH FIQ and IRQ interrupts DISABLED.
@
@           2) The pseudo-code for OSCtxSw() is:
@              a) Save the current task's context onto the current task's stack,
@              b) OSTCBCurPtr->StkPtr = SP;
@              c) OSTaskSwHook();
@              d) OSPrioCur           = OSPrioHighRdy;
@              e) OSTCBCurPtr         = OSTCBHighRdyPtr;
@              f) SP                  = OSTCBHighRdyPtr->StkPtr;
@              g) Restore the new task's context from the new task's stack,
@              h) Return to new task's code.
@
@           3) Upon entry:
@              OSTCBCurPtr      points to the OS_TCB of the task to suspend,
@              OSTCBHighRdyPtr  points to the OS_TCB of the task to resume.
@********************************************************************************************************

    .type   OSCtxSw, %function
OSCtxSw:
                                                                @ SAVE CURRENT TASK'S CONTEXT:
    STMFD   SP!, {LR}                                           @     Push return address,
    STMFD   SP!, {LR}
    STMFD   SP!, {R0-R12}                                       @     Push registers,
    MRS     R0, CPSR                                            @     Push current CPSR,
    TST     LR, #1                                              @     See if called from Thumb mode,
    ORRNE   R0, R0, #OS_CPU_ARM_CONTROL_THUMB                   @     If yes, set the T-bit.
    STMFD   SP!, {R0}

    OS_CPU_ARM_FP_REG_PUSH  R0

    CLREX                                                       @ Clear exclusive monitor.

    MOVW    R0, #:lower16:OSTCBCurPtr                           @ OSTCBCurPtr->StkPtr = SP;
    MOVT    R0, #:upper16:OSTCBCurPtr
    LDR     R1, [R0]
    STR     SP, [R1]

    BL      OSTaskSwHook                                        @ OSTaskSwHook();

    MOVW    R0, #:lower16:OSPrioCur                             @ OSPrioCur   = OSPrioHighRdy;
    MOVT    R0, #:upper16:OSPrioCur
    MOVW    R1, #:lower16:OSPrioHighRdy
    MOVT    R1, #:upper16:OSPrioHighRdy
    LDRB    R2, [R1]
    STRB    R2, [R0]

    MOVW    R0, #:lower16:OSTCBCurPtr                           @ OSTCBCurPtr = OSTCBHighRdyPtr;
    MOVT    R0, #:upper16:OSTCBCurPtr
    MOVW    R1, #:lower16:OSTCBHighRdyPtr
    MOVT    R1, #:upper16:OSTCBHighRdyPtr

    LDR     R2, [R1]
    STR     R2, [R0]

    LDR     SP, [R2]                                            @ SP = OSTCBHighRdyPtr->OSTCBStkPtr;

    OS_CPU_ARM_FP_REG_POP R0
                                                                @ RESTORE NEW TASK'S CONTEXT:
    LDMFD   SP!, {R0}                                           @    Pop new task's CPSR,
    MSR     SPSR_cxsf, R0

    LDMFD   SP!, {R0-R12, LR, PC}^                              @    Pop new task's context.


@********************************************************************************************************
@                   PERFORM A CONTEXT SWITCH (From interrupt level) - OSIntCtxSw()
@
@ Note(s) : 1) OSIntCtxSw() is called in SVC mode with BOTH FIQ and IRQ interrupts DISABLED.
@
@           2) The pseudo-code for OSCtxSw() is:
@              a) OSTaskSwHook();
@              b) OSPrioCur   = OSPrioHighRdy;
@              c) OSTCBCurPtr = OSTCBHighRdyPtr;
@              d) SP          = OSTCBHighRdyPtr->OSTCBStkPtr;
@              e) Restore the new task's context from the new task's stack,
@              f) Return to new task's code.
@
@           3) Upon entry:
@              OSTCBCurPtr      points to the OS_TCB of the task to suspend,
@              OSTCBHighRdyPtr  points to the OS_TCB of the task to resume.
@********************************************************************************************************

    .type   OSIntCtxSw, %function
OSIntCtxSw:
    BL      OSTaskSwHook                                        @ OSTaskSwHook();

    MOVW    R0, #:lower16:OSPrioCur                             @ OSPrioCur = OSPrioHighRdy;
    MOVT    R0, #:upper16:OSPrioCur
    MOVW    R1, #:lower16:OSPrioHighRdy
    MOVT    R1, #:upper16:OSPrioHighRdy
    LDRB    R2, [R1]
    STRB    R2, [R0]

    MOVW    R0, #:lower16:OSTCBCurPtr                           @ OSTCBCurPtr = OSTCBHighRdyPtr;
    MOVT    R0, #:upper16:OSTCBCurPtr
    MOVW    R1, #:lower16:OSTCBHighRdyPtr
    MOVT    R1, #:upper16:OSTCBHighRdyPtr
    LDR     R2, [R1]
    STR     R2, [R0]

    LDR     SP, [R2]                                            @ SP = OSTCBHighRdyPtr->OSTCBStkPtr;

    OS_CPU_ARM_FP_REG_POP R0
                                                                @ RESTORE NEW TASK'S CONTEXT:
    LDMFD   SP!, {R0}                                           @    Pop new task's CPSR,
    MSR     SPSR_cxsf, R0

    LDMFD   SP!, {R0-R12, LR, PC}^                              @    Pop new task's context.


@********************************************************************************************************
@                               UNDEFINED INSTRUCTION EXCEPTION HANDLER
@
@ Register Usage:  R0     Exception Type
@                  R1
@                  R2     Return PC
@********************************************************************************************************

OS_CPU_ARM_ExceptUndefInstrHndlr:
                                                                @ LR offset to return from this exception:  0.
    STMFD   SP!, {R0-R3}                                        @ Push working registers.
    MOV     R2, LR                                              @ Save link register.
    MOV     R0, #OS_CPU_ARM_EXCEPT_UNDEF_INSTR                  @ Set exception ID to OS_CPU_ARM_EXCEPT_UNDEF_INSTR.
    B            OS_CPU_ARM_ExceptHndlr                         @ Branch to global exception handler.


@********************************************************************************************************
@                                SOFTWARE INTERRUPT EXCEPTION HANDLER
@
@ Register Usage:  R0     Exception Type
@                  R1
@                  R2     Return PC
@********************************************************************************************************

OS_CPU_ARM_ExceptSwiHndlr:
                                                                @ LR offset to return from this exception:  0.
    STMFD   SP!, {R0-R3}                                        @ Push working registers.
    MOV     R2, LR                                              @ Save link register.
    MOV     R0, #OS_CPU_ARM_EXCEPT_SWI                          @ Set exception ID to OS_CPU_ARM_EXCEPT_SWI.
    B            OS_CPU_ARM_ExceptHndlr                         @ Branch to global exception handler.


@********************************************************************************************************
@                                  PREFETCH ABORT EXCEPTION HANDLER
@
@ Register Usage:  R0     Exception Type
@                  R1
@                  R2     Return PC
@********************************************************************************************************

OS_CPU_ARM_ExceptPrefetchAbortHndlr:
    SUB     LR, LR, #4                                          @ LR offset to return from this exception: -4.
    STMFD   SP!, {R0-R3}                                        @ Push working registers.
    MOV     R2, LR                                              @ Save link register.
    MOV     R0, #OS_CPU_ARM_EXCEPT_PREFETCH_ABORT               @ Set exception ID to OS_CPU_ARM_EXCEPT_PREFETCH_ABORT.
    B            OS_CPU_ARM_ExceptHndlr                         @ Branch to global exception handler.


@********************************************************************************************************
@                                    DATA ABORT EXCEPTION HANDLER
@
@ Register Usage:  R0     Exception Type
@                  R1
@                  R2     Return PC
@********************************************************************************************************

OS_CPU_ARM_ExceptDataAbortHndlr:
    SUB     LR, LR, #8                                          @ LR offset to return from this exception: -8.
    STMFD   SP!, {R0-R3}                                        @ Push working registers.
    MOV     R2, LR                                              @ Save link register.
    MOV     R0, #OS_CPU_ARM_EXCEPT_DATA_ABORT                   @ Set exception ID to OS_CPU_ARM_EXCEPT_DATA_ABORT.
    B            OS_CPU_ARM_ExceptHndlr                         @ Branch to global exception handler.


@********************************************************************************************************
@                                   ADDRESS ABORT EXCEPTION HANDLER
@
@ Register Usage:  R0     Exception Type
@                  R1
@                  R2     Return PC
@********************************************************************************************************

OS_CPU_ARM_ExceptAddrAbortHndlr:
    SUB     LR, LR, #8                                          @ LR offset to return from this exception: -8.
    STMFD   SP!, {R0-R3}                                        @ Push working registers.
    MOV     R2, LR                                              @ Save link register.
    MOV     R0, #OS_CPU_ARM_EXCEPT_ADDR_ABORT                   @ Set exception ID to OS_CPU_ARM_EXCEPT_ADDR_ABORT.
    B            OS_CPU_ARM_ExceptHndlr                         @ Branch to global exception handler.


@********************************************************************************************************
@                                 INTERRUPT REQUEST EXCEPTION HANDLER
@
@ Register Usage:  R0     Exception Type
@                  R1
@                  R2     Return PC
@********************************************************************************************************

OS_CPU_ARM_ExceptIrqHndlr:
    SUB     LR, LR, #4                                          @ LR offset to return from this exception: -4.
    STMFD   SP!, {R0-R3}                                        @ Push working registers.
    MOV     R2, LR                                              @ Save link register.
    MOV     R0, #OS_CPU_ARM_EXCEPT_IRQ                          @ Set exception ID to OS_CPU_ARM_EXCEPT_IRQ.
    B            OS_CPU_ARM_ExceptHndlr                         @ Branch to global exception handler.


@********************************************************************************************************
@                              FAST INTERRUPT REQUEST EXCEPTION HANDLER
@
@ Register Usage:  R0     Exception Type
@                  R1
@                  R2     Return PC
@********************************************************************************************************

OS_CPU_ARM_ExceptFiqHndlr:
    SUB     LR, LR, #4                                          @ LR offset to return from this exception: -4.
    STMFD   SP!, {R0-R3}                                        @ Push working registers.
    MOV     R2, LR                                              @ Save link register.
    MOV     R0, #OS_CPU_ARM_EXCEPT_FIQ                          @ Set exception ID to OS_CPU_ARM_EXCEPT_FIQ.
    B            OS_CPU_ARM_ExceptHndlr                         @ Branch to global exception handler.


@********************************************************************************************************
@                                      GLOBAL EXCEPTION HANDLER
@
@ Register Usage:  R0     Exception Type
@                  R1     Exception's SPSR
@                  R2     Return PC
@                  R3     Exception's SP
@
@ Note(s)       : 1) An exception can occur in three different circumstances; in each of these, the
@                    SVC stack pointer will point to a different entity :
@
@                    a) CONDITION: An exception occurs before the OS has been fully initialized.
@                       SVC STACK: Should point to a stack initialized by the application's startup code.
@                       STK USAGE: Interrupted context -- SVC stack.
@                                  Exception           -- SVC stack.
@                                  Nested exceptions   -- SVC stack.
@
@                    b) CONDITION: An exception interrupts a task.
@                       SVC STACK: Should point to task stack.
@                       STK USAGE: Interrupted context -- Task stack.
@                                  Exception           -- Exception stack 'OS_CPU_ExceptStk[]'.
@                                  Nested exceptions   -- Exception stack 'OS_CPU_ExceptStk[]'.
@
@                    c) CONDITION: An exception interrupts another exception.
@                       SVC STACK: Should point to location in exception stack, 'OS_CPU_ExceptStk[]'.
@                       STK USAGE: Interrupted context -- Exception stack 'OS_CPU_ExceptStk[]'.
@                                  Exception           -- Exception stack 'OS_CPU_ExceptStk[]'.
@                                  Nested exceptions   -- Exception stack 'OS_CPU_ExceptStk[]'.
@********************************************************************************************************

OS_CPU_ARM_ExceptHndlr:
    MRS     R1, SPSR                                            @ Save CPSR (i.e. exception's SPSR).
    MOV     R3, SP                                              @ Save exception's stack pointer.

                                                                @ Adjust exception stack pointer.  This is needed because
                                                                @ exception stack is not used when restoring task context.
    ADD     SP, SP, #(4 * 4)

                                                                @ Change to SVC mode & disable interruptions.
    MSR     CPSR_c, #(OS_CPU_ARM_CONTROL_INT_DIS | OS_CPU_ARM_MODE_SVC)
    CLREX                                                       @ Clear exclusive monitor.

    STMFD   SP!, {R2}                                           @   Push task's PC,
    STMFD   SP!, {LR}                                           @   Push task's LR,
    STMFD   SP!, {R4-R12}                                       @   Push task's R12-R4,
    LDMFD   R3!, {R5-R8}                                        @   Move task's R3-R0 from exception stack to task's stack.
    STMFD   SP!, {R5-R8}
    STMFD   SP!, {R1}                                           @   Push task's CPSR (i.e. exception SPSR).

    OS_CPU_ARM_FP_REG_PUSH   R1

    MOVW    R3, #:lower16:OSRunning                             @ if (OSRunning == 1)
    MOVT    R3, #:upper16:OSRunning
    LDRB    R4, [R3]
    CMP     R4, #1
    BNE     OS_CPU_ARM_ExceptHndlr_BreakNothing

                                                                @ HANDLE NESTING COUNTER:
    MOVW    R3, #:lower16:OSIntNestingCtr                       @   OSIntNestingCtr++;
    MOVT    R3, #:upper16:OSIntNestingCtr
    LDRB    R4, [R3]
    ADD     R4, R4, #1
    STRB    R4, [R3]

    CMP     R4, #1                                              @ if (OSIntNestingCtr == 1)
    BNE     OS_CPU_ARM_ExceptHndlr_BreakExcept


@********************************************************************************************************
@                                 EXCEPTION HANDLER: TASK INTERRUPTED
@
@ Register Usage:  R0     Exception Type
@                  R1
@                  R2
@                  R3
@********************************************************************************************************

OS_CPU_ARM_ExceptHndlr_BreakTask:
    MOVW    R3, #:lower16:OSTCBCurPtr                           @ OSTCBCurPtr->StkPtr = SP;
    MOVT    R3, #:upper16:OSTCBCurPtr
    LDR     R4, [R3]
    STR     SP, [R4]


    MOVW    R3, #:lower16:OS_CPU_ExceptStkBase                  @ Switch to exception stack.
    MOVT    R3, #:upper16:OS_CPU_ExceptStkBase
    LDR     SP, [R3]

                                                                @ EXECUTE EXCEPTION HANDLER:
    BL      OS_CPU_ExceptHndlr                                  @ OS_CPU_ExceptHndlr(except_type = R0)

                                                                @ Change to SVC mode & disable interruptions.
    MSR     CPSR_c, #(OS_CPU_ARM_CONTROL_INT_DIS | OS_CPU_ARM_MODE_SVC)

                                                                @ Call OSIntExit().  This call MAY never return if a ready
                                                                @ task with higher priority than the interrupted one is
                                                                @ found.
    BL      OSIntExit

    MOVW    R3, #:lower16:OSTCBCurPtr                           @ SP = OSTCBCurPtr->StkPtr;
    MOVT    R3, #:upper16:OSTCBCurPtr
    LDR     R4, [R3]
    LDR     SP, [R4]

    OS_CPU_ARM_FP_REG_POP R0
                                                                @ RESTORE NEW TASK'S CONTEXT:
    LDMFD   SP!, {R0}                                           @    Pop new task's CPSR,
    MSR     SPSR_cxsf, R0

    LDMFD   SP!, {R0-R12, LR, PC}^                              @    Pop new task's context.


@********************************************************************************************************
@                              EXCEPTION HANDLER: EXCEPTION INTERRUPTED
@
@ Register Usage:  R0     Exception Type
@                  R1
@                  R2
@                  R3
@********************************************************************************************************

OS_CPU_ARM_ExceptHndlr_BreakExcept:

    MOV     R1, SP
    AND     R1, R1, #4
    SUB     SP, SP, R1
    STMFD   SP!, {R1, LR}
                                                                @ EXECUTE EXCEPTION HANDLER:
    BL      OS_CPU_ExceptHndlr                                  @ OS_CPU_ExceptHndlr(except_type = R0)

    LDMIA   SP!, {R1, LR}
    ADD     SP, SP, R1

                                                                @ Change to SVC mode & disable interruptions.
    MSR     CPSR_c, #(OS_CPU_ARM_CONTROL_INT_DIS | OS_CPU_ARM_MODE_SVC)

                                                                @ HANDLE NESTING COUNTER:
    MOVW    R3, #:lower16:OSIntNestingCtr                       @   OSIntNestingCtr--;
    MOVT    R3, #:upper16:OSIntNestingCtr
    LDRB    R4, [R3]
    SUB     R4, R4, #1
    STRB    R4, [R3]

    OS_CPU_ARM_FP_REG_POP R0
                                                                @ RESTORE OLD CONTEXT:
    LDMFD   SP!, {R0}                                           @    Pop old CPSR,
    MSR     SPSR_cxsf, R0

    LDMFD   SP!, {R0-R12, LR, PC}^                              @   Pull working registers and return from exception.


@********************************************************************************************************
@                              EXCEPTION HANDLER: 'NOTHING' INTERRUPTED
@
@ Register Usage:  R0     Exception Type
@                  R1
@                  R2
@                  R3
@********************************************************************************************************

OS_CPU_ARM_ExceptHndlr_BreakNothing:

    MOV     R1, SP
    AND     R1, R1, #4
    SUB     SP, SP, R1
    STMFD   SP!, {R1, LR}

                                                                @ EXECUTE EXCEPTION HANDLER:
    BL      OS_CPU_ExceptHndlr                                  @ OS_CPU_ExceptHndlr(except_type = R0)

    LDMIA   SP!, {R1, LR}
    ADD     SP, SP, R1

                                                                @ Change to SVC mode & disable interruptions.
    MSR     CPSR_c, #(OS_CPU_ARM_CONTROL_INT_DIS | OS_CPU_ARM_MODE_SVC)

    OS_CPU_ARM_FP_REG_POP R0
                                                                @ RESTORE OLD CONTEXT:
    LDMFD   SP!, {R0}                                           @   Pop old CPSR,
    MSR     SPSR_cxsf, R0

    LDMFD   SP!, {R0-R12, LR, PC}^                              @   Pull working registers and return from exception.


@********************************************************************************************************
@                              VFP/NEON REGISTER COUNT
@
@ Register Usage:  R0     Double Register Count
@********************************************************************************************************

    .type   OS_CPU_ARM_DRegCntGet, %function
OS_CPU_ARM_DRegCntGet:
    MOV     R0, #16
    BX      LR
