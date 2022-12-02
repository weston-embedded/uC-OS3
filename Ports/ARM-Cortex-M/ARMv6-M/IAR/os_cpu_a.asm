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
;                                             ARMv6-M Port
;
; File      : os_cpu_a.asm
; Version   : V3.08.02
;********************************************************************************************************
; For       : ARMv6-M Cortex-M0 or Cortex-M0+
; Mode      : Thumb2
; Toolchain : IAR EWARM
;********************************************************************************************************
; Note(s)   : (1) This port supports the ARM Cortex-M0 and Cortex-M0+ architectures.
;********************************************************************************************************
;

;********************************************************************************************************
;                                          PUBLIC FUNCTIONS
;********************************************************************************************************

                                                                ; External references.
    EXTERN  OSPrioCur
    EXTERN  OSPrioHighRdy
    EXTERN  OSTCBCurPtr
    EXTERN  OSTCBHighRdyPtr
    EXTERN  OSIntExit
    EXTERN  OSTaskSwHook
    EXTERN  OS_CPU_ExceptStkBase


    PUBLIC  OSStartHighRdy                                      ; Functions declared in this file
    PUBLIC  OSCtxSw
    PUBLIC  OSIntCtxSw
    PUBLIC  OS_CPU_PendSVHandler

;********************************************************************************************************
;                                                EQUATES
;********************************************************************************************************

NVIC_INT_CTRL   EQU     0xE000ED04                              ; Interrupt control state register.
NVIC_SYSPRI14   EQU     0xE000ED20                              ; System priority register (priority 14).
NVIC_PENDSV_PRI EQU     0x00FF0000                              ; PendSV priority value (lowest).
NVIC_PENDSVSET  EQU     0x10000000                              ; Value to trigger PendSV exception.

;********************************************************************************************************
;                                      CODE GENERATION DIRECTIVES
;********************************************************************************************************

    RSEG CODE:CODE:NOROOT(2)
    THUMB


;********************************************************************************************************
;                                         START MULTITASKING
;                                      void OSStartHighRdy(void)
;
; Note(s) : 1) This function triggers a PendSV exception (essentially, causes a context switch) to cause
;              the first task to start.
;
;           2) OSStartHighRdy() MUST:
;              a) Setup PendSV exception priority to lowest;
;              b) Set initial PSP to 0, to tell context switcher this is first run;
;              c) Set the main stack to OS_CPU_ExceptStkBase
;              d) Trigger PendSV exception;
;              e) Enable interrupts (tasks will run with interrupts enabled).
;********************************************************************************************************

OSStartHighRdy
    CPSID   I                                                   ; Prevent interruption during context switch
    LDR     R0, =NVIC_SYSPRI14                                  ; Set the PendSV exception priority
    LDR     R1, =NVIC_PENDSV_PRI
    STR     R1, [R0]

    MOVS    R0, #0                                              ; Set the PSP to 0 for initial context switch call
    MSR     PSP, R0
    BL      OSTaskSwHook

    LDR     R0, =OS_CPU_ExceptStkBase                           ; Initialize the MSP to the OS_CPU_ExceptStkBase
    LDR     R1, [R0]
    MSR     MSP, R1

    LDR     R0, =NVIC_INT_CTRL                                  ; Trigger the PendSV exception (causes context switch)
    LDR     R1, =NVIC_PENDSVSET
    STR     R1, [R0]

    CPSIE   I                                                   ; Enable interrupts at processor level

OSStartHang
    B       OSStartHang                                         ; Should never get here


;********************************************************************************************************
;                       PERFORM A CONTEXT SWITCH (From task level) - OSCtxSw()
;
; Note(s) : 1) OSCtxSw() is called when OS wants to perform a task context switch.  This function
;              triggers the PendSV exception which is where the real work is done.
;********************************************************************************************************

OSCtxSw
    LDR     R0, =NVIC_INT_CTRL                                  ; Trigger the PendSV exception (causes context switch)
    LDR     R1, =NVIC_PENDSVSET
    STR     R1, [R0]
    BX      LR


;********************************************************************************************************
;                   PERFORM A CONTEXT SWITCH (From interrupt level) - OSIntCtxSw()
;
; Note(s) : 1) OSIntCtxSw() is called by OSIntExit() when it determines a context switch is needed as
;              the result of an interrupt.  This function simply triggers a PendSV exception which will
;              be handled when there are no more interrupts active and interrupts are enabled.
;********************************************************************************************************

OSIntCtxSw
    LDR     R0, =NVIC_INT_CTRL                                  ; Trigger the PendSV exception (causes context switch)
    LDR     R1, =NVIC_PENDSVSET
    STR     R1, [R0]
    BX      LR


;********************************************************************************************************
;                                       HANDLE PendSV EXCEPTION
;                                   void OS_CPU_PendSVHandler(void)
;
; Note(s) : 1) PendSV is used to cause a context switch.  This is a recommended method for performing
;              context switches with Cortex-M0.  This is because the Cortex-M0 auto-saves half of the
;              processor context on any exception, and restores same on return from exception.  So only
;              saving of R4-R11, LR is required and fixing up the stack pointers. Using the PendSV exception
;              this way means that context saving and restoring is identical whether it is initiated from
;              a thread or occurs due to an interrupt or exception.
;
;           2) Pseudo-code is:
;              a) Get the process SP, if 0 then skip (goto d) the saving part (first context switch);
;              b) Adjust SP to make space for R4-R11 and LR register
;              c) Save the process SP in its TCB, OSTCBCurPtr->StkPtr = SP;
;              d) Save remaining regs R4-R11, LR on process stack;
;              e) Call OSTaskSwHook();
;              f) Get current high priority, OSPrioCur = OSPrioHighRdy;
;              g) Get current ready thread TCB, OSTCBCurPtr = OSTCBHighRdyPtr;
;              h) Get new process SP from TCB, SP = OSTCBHighRdyPtr->StkPtr;
;              i) Adjust SP to the High Registers(R8-R11) & LR location
;              j) Restore R8-R11, and LR from new process stack;
;              k) Load PSP with new process SP
;              m) Restore remaining R4-R7 from new process stack
;              n) Perform exception return which will restore remaining context.
;
;           3) On entry into PendSV handler:
;              a) The following have been saved on the process stack (by processor):
;                 xPSR, PC, LR, R12, R0-R3
;              b) Processor mode is switched to Handler mode (from Thread mode)
;              c) Stack is Main stack (switched from Process stack)
;              d) OSTCBCurPtr      points to the OS_TCB of the task to suspend
;                 OSTCBHighRdyPtr  points to the OS_TCB of the task to resume
;
;           4) Since PendSV is set to lowest priority in the system (by OSStartHighRdy() above), we
;              know that it will only be run when no other exception or interrupt is active, and
;              therefore safe to assume that context being switched out was using the process stack (PSP).
;********************************************************************************************************

OS_CPU_PendSVHandler
    CPSID   I                                                   ; Prevent interruption during context switch
    MRS     R0, PSP                                             ; PSP is process stack pointer

    CMP     R0, #0
    BEQ     OS_CPU_PendSVHandler_nosave                         ; equivalent code to CBZ from M3 arch to M0 arch
                                                                ; Except that it does not change the condition code flags

    SUBS    R0, R0, #0x24                                       ; Adjust SP to make space for Low, High & LR registers
    LDR     R1, =OSTCBCurPtr                                    ; OSTCBCurPtr->StkPtr = SP;
    LDR     R1, [R1]
    STR     R0, [R1]                                            ; R0 is SP of process being switched out

    STMIA   R0!, {R4-R7}                                        ; Store R4-R7(Low Registers) on process stack
    MOV     R4, R8                                              ; Move R8-R11 values to R4-R7 registers.
    MOV     R5, R9
    MOV     R6, R10
    MOV     R7, R11
    STMIA   R0!, {R4-R7}                                        ; Store R8-R11(High Registers) on process stack
    MOV     R3, R14                                             ; R3 is LR of process being switched out
    STMIA   R0!, {R3}                                           ; Store LR (EXC_RETURN) on process stack.

                                                                ; At this point, entire context of process has been saved
OS_CPU_PendSVHandler_nosave
    BL      OSTaskSwHook                                        ; OSTaskSwHook();

    LDR     R0, =OSPrioCur                                      ; OSPrioCur   = OSPrioHighRdy;
    LDR     R1, =OSPrioHighRdy
    LDRB    R2, [R1]
    STRB    R2, [R0]

    LDR     R0, =OSTCBCurPtr                                    ; OSTCBCurPtr = OSTCBHighRdyPtr;
    LDR     R1, =OSTCBHighRdyPtr
    LDR     R2, [R1]
    STR     R2, [R0]

    LDR     R0, [R2]                                            ; R0 is new process SP; SP = OSTCBHighRdy->OSTCBStkPtr;

    ADDS    R0, #0x10                                           ; Adjust SP to High Registers(R8-R11) location
    LDMFD   R0!, {R4-R7}                                        ; Load R8-R11 values from new process stack into R4-R7
    MOV     R8,  R4                                             ; Restore R8-R11 by moving their values from R4-R7
    MOV     R9,  R5
    MOV     R10, R6
    MOV     R11, R7

    LDMFD   R0!, {R3}                                           ; R3 has the new process LR; R3 = LR (EXEC_RETURN)
    MOV     R14, R3                                             ; Restore LR (EXEC_RETURN)

    MSR     PSP, R0                                             ; Load PSP with new process SP

    SUBS    R0, #0x24                                           ; Adjust SP to Low registers(R4-R7) location
    LDMFD   R0!, {R4-R7}                                        ; Restore remaining R4-R7 from new process stack

    CPSIE   I
    BX      LR                                                  ; Exception return will restore remaining context

    END
