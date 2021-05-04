;********************************************************************************************************
;                                              uC/OS-III
;                                        The Real-Time Kernel
;
;                    Copyright 2009-2021 Silicon Laboratories Inc. www.silabs.com
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
;                                         Synopsys ARC EM6 Port
;
; File      : os_cpu_a.s
; Version   : V3.08.01
;********************************************************************************************************
; For       : Synopsys ARC EM6
; Mode      : Little-Endian, 32 registers, FPU, Code Density, Loop Counter, Stack Check
; Toolchain : MetaWare C/C++ Clang-based Compiler
;********************************************************************************************************

;********************************************************************************************************
;                                            PUBLIC FUNCTIONS
;********************************************************************************************************

    .global  OSCtxSw
    .global  OSIntCtxSw

    .global  OSStartHighRdy

    .global  OS_CPU_EM6_ExceptionHandler
    .global  OS_CPU_EM6_InterruptHandler
    .global  OS_CPU_EM6_TrapHandler


;********************************************************************************************************
;                                       EXTERNAL GLOBAL VARIABLES
;********************************************************************************************************

    .extern  OSPrioCur                                          ; Declared as CPU_INT08U     ,  8-bit wide.
    .extern  OSPrioHighRdy                                      ; Declared as CPU_INT08U     ,  8-bit wide.
    .extern  OSRunning                                          ; Declared as CPU_INT08U     ,  8-bit wide.
    .extern  OSTCBCurPtr                                        ; Declared as OS_TCB *       , 32-bit wide.
    .extern  OSTCBHighRdyPtr                                    ; Declared as OS_TCB *       , 32-bit wide.
    .extern  OSTaskSwHook                                       ; Declared as CPU_FNCT_VOID  , 32-bit wide.
    .extern  OS_CPU_ExceptionHandler                            ; Declared as ptr to function, 32-bit wide.
    .extern  OS_CPU_InterruptHandler                            ; Declared as ptr to function, 32-bit wide.
    .extern  OS_CPU_GetStackRegion                              ; Declared as ptr to function, 32-bit wide.


;********************************************************************************************************
;                                                EQUATES
;********************************************************************************************************

                                                                ; Auxiliary Registers
    .equ  OS_CPU_AR_LP_START,                   0x02
    .equ  OS_CPU_AR_LP_END,                     0x03
    .equ  OS_CPU_AR_STATUS32,                   0x0A
    .equ  OS_CPU_AR_JLI_BASE,                  0x290
    .equ  OS_CPU_AR_LDI_BASE,                  0x291
    .equ  OS_CPU_AR_EI_BASE,                   0x292
    .equ  OS_CPU_AR_ERET,                      0x400
    .equ  OS_CPU_AR_ERSTATUS,                  0x402
    .equ  OS_CPU_AR_ECR,                       0x403
    .equ  OS_CPU_AR_ICAUSE,                    0x40A
    .equ  OS_CPU_AR_USTACK_TOP,                0x260
    .equ  OS_CPU_AR_USTACK_BASE,               0x261
    .equ  OS_CPU_AR_KSTACK_TOP,                0x264
    .equ  OS_CPU_AR_KSTACK_BASE,               0x265

    .equ  OS_CPU_STATUS32_MASK,           0xFFF8603F

                                                                ; Context offsets, in bytes, in saved stack.
    .equ  OS_CPU_STK_OFFSET_END,                 164
    .equ  OS_CPU_STK_OFFSET_STATUS32,            160
    .equ  OS_CPU_STK_OFFSET_PC,                  156
    .equ  OS_CPU_STK_OFFSET_JLI_BASE,            152
    .equ  OS_CPU_STK_OFFSET_LDI_BASE,            148
    .equ  OS_CPU_STK_OFFSET_EI_BASE,             144
    .equ  OS_CPU_STK_OFFSET_LP_COUNT,            140
    .equ  OS_CPU_STK_OFFSET_LP_START,            136
    .equ  OS_CPU_STK_OFFSET_LP_END,              132
    .equ  OS_CPU_STK_OFFSET_R27,                 128
    .equ  OS_CPU_STK_OFFSET_R26,                 124
    .equ  OS_CPU_STK_OFFSET_R25,                 120
    .equ  OS_CPU_STK_OFFSET_R24,                 116
    .equ  OS_CPU_STK_OFFSET_R23,                 112
    .equ  OS_CPU_STK_OFFSET_R22,                 108
    .equ  OS_CPU_STK_OFFSET_R21,                 104
    .equ  OS_CPU_STK_OFFSET_R20,                 100
    .equ  OS_CPU_STK_OFFSET_R19,                  96
    .equ  OS_CPU_STK_OFFSET_R18,                  92
    .equ  OS_CPU_STK_OFFSET_R17,                  88
    .equ  OS_CPU_STK_OFFSET_R16,                  84
    .equ  OS_CPU_STK_OFFSET_R15,                  80
    .equ  OS_CPU_STK_OFFSET_R14,                  76
    .equ  OS_CPU_STK_OFFSET_R13,                  72
    .equ  OS_CPU_STK_OFFSET_R12,                  68
    .equ  OS_CPU_STK_OFFSET_R11,                  64
    .equ  OS_CPU_STK_OFFSET_R10,                  60
    .equ  OS_CPU_STK_OFFSET_R9,                   56
    .equ  OS_CPU_STK_OFFSET_R8,                   52
    .equ  OS_CPU_STK_OFFSET_R7,                   48
    .equ  OS_CPU_STK_OFFSET_R6,                   44
    .equ  OS_CPU_STK_OFFSET_R5,                   40
    .equ  OS_CPU_STK_OFFSET_R4,                   36
    .equ  OS_CPU_STK_OFFSET_R3,                   32
    .equ  OS_CPU_STK_OFFSET_R2,                   28
    .equ  OS_CPU_STK_OFFSET_R1,                   24
    .equ  OS_CPU_STK_OFFSET_R0,                   20
    .equ  OS_CPU_STK_OFFSET_BLINK,                16
    .equ  OS_CPU_STK_OFFSET_R30,                  12
    .equ  OS_CPU_STK_OFFSET_R29,                   8
    .equ  OS_CPU_STK_OFFSET_ACCH,                  4
    .equ  OS_CPU_STK_OFFSET_ACCL,                  0


;********************************************************************************************************
;                                                MACROS
;********************************************************************************************************

;********************************************************************************************************
;                                               FUNCTION
;
; Description : This macro declares a symbol of type function and aligns it to 4-bytes.
;
; Arguments   : fname   function to declare.
;
; Note(s)     : none.
;********************************************************************************************************

.macro  FUNCTION, fname
    .type   \&fname, @function
    .align  4
    \&fname:
.endm


;********************************************************************************************************
;                                      SAVE_CONTEXT_FROM_EXCEPTION
;
; Description : Macro used to save the context of a task at the task level as if it were saved
;               by an interrupt.
;
; Arguments   : none.
;
; Note(s)     : none.
;********************************************************************************************************

.macro  SAVE_CONTEXT_FROM_EXCEPTION
                                                                ; Push context as if saved from interrupt.
    ST    %r0, [%sp, -(OS_CPU_STK_OFFSET_END-OS_CPU_STK_OFFSET_R0)] ;   Save r0 first, used as a buffer.

    LR    %r0, [OS_CPU_AR_ERSTATUS]                             ;   STATUS32
    PUSH  %r0
    LR    %r0, [OS_CPU_AR_ERET]                                 ;   PC
    PUSH  %r0

    LR    %r0, [OS_CPU_AR_JLI_BASE]                             ;   JLI_BASE
    PUSH  %r0
    LR    %r0, [OS_CPU_AR_LDI_BASE]                             ;   LDI_BASE
    PUSH  %r0
    LR    %r0, [OS_CPU_AR_EI_BASE]                              ;   EI_BASE
    PUSH  %r0

    PUSH  %lp_count                                             ;   LP_COUNT
    LR    %r0, [OS_CPU_AR_LP_START]                             ;   LP_START
    PUSH  %r0
    LR    %r0, [OS_CPU_AR_LP_END]                               ;   LP_END
    PUSH  %r0

    PUSH  %r27                                                  ;   r27..r1
    PUSH  %r26
    PUSH  %r25
    PUSH  %r24
    PUSH  %r23
    PUSH  %r22
    PUSH  %r21
    PUSH  %r20
    PUSH  %r19
    PUSH  %r18
    PUSH  %r17
    PUSH  %r16
    PUSH  %r15
    PUSH  %r14
    PUSH  %r13
    PUSH  %r12
    PUSH  %r11
    PUSH  %r10
    PUSH  %r9
    PUSH  %r8
    PUSH  %r7
    PUSH  %r6
    PUSH  %r5
    PUSH  %r4
    PUSH  %r3
    PUSH  %r2
    PUSH  %r1

    SUB   %sp, %sp, 4                                           ;   skip r0

    PUSH  %r31                                                  ;   r31..r29
    PUSH  %r30
    PUSH  %r29

    PUSH  %acch                                                 ;   ACCH
    PUSH  %accl                                                 ;   ACCL
.endm


;********************************************************************************************************
;                                    RESTORE_CONTEXT_FROM_EXCEPTION
;
; Description : Macro used to restore the context of a task at the task level as if it were restored
;               by an interrupt.
;
; Arguments   : none.
;
; Note(s)     : none.
;********************************************************************************************************

.macro  RESTORE_CONTEXT_FROM_EXCEPTION
                                                                ; Load ERET
    LD    %r0, [%sp, OS_CPU_STK_OFFSET_PC]
    SR    %r0, [OS_CPU_AR_ERET]

                                                                ; Restore context manually because this is
                                                                ; a return from exception.
    LD    %r0, [%sp, OS_CPU_STK_OFFSET_JLI_BASE]                ;   JLI_BASE
    SR    %r0, [OS_CPU_AR_JLI_BASE]

    LD    %r0, [%sp, OS_CPU_STK_OFFSET_LDI_BASE]                ;   LDI_BASE
    SR    %r0, [OS_CPU_AR_LDI_BASE]

    LD    %r0, [%sp, OS_CPU_STK_OFFSET_EI_BASE]                 ;   EI_BASE
    SR    %r0, [OS_CPU_AR_EI_BASE]

    LD    %r0, [%sp, OS_CPU_STK_OFFSET_LP_COUNT]                ;   LP_COUNT
    MOV   %lp_count, %r0

    LD    %r0, [%sp, OS_CPU_STK_OFFSET_LP_START]                ;   LP_START
    SR    %r0, [OS_CPU_AR_LP_START]

    LD    %r0, [%sp, OS_CPU_STK_OFFSET_LP_END]                  ;   LP_END
    SR    %r0, [OS_CPU_AR_LP_END]

    LD    %r31, [%sp, OS_CPU_STK_OFFSET_BLINK]                  ;   r31..r29
    LD    %r30, [%sp, OS_CPU_STK_OFFSET_R30]
    LD    %r29, [%sp, OS_CPU_STK_OFFSET_R29]

    LD    %acch, [%sp, OS_CPU_STK_OFFSET_ACCH]                  ;   ACCH and ACCL
    LD    %accl, [%sp, OS_CPU_STK_OFFSET_ACCL]

    LD    %r27, [%sp, OS_CPU_STK_OFFSET_R27]                    ;   r27..r0
    LD    %r26, [%sp, OS_CPU_STK_OFFSET_R26]
    LD    %r25, [%sp, OS_CPU_STK_OFFSET_R25]
    LD    %r24, [%sp, OS_CPU_STK_OFFSET_R24]
    LD    %r23, [%sp, OS_CPU_STK_OFFSET_R23]
    LD    %r22, [%sp, OS_CPU_STK_OFFSET_R22]
    LD    %r21, [%sp, OS_CPU_STK_OFFSET_R21]
    LD    %r20, [%sp, OS_CPU_STK_OFFSET_R20]
    LD    %r19, [%sp, OS_CPU_STK_OFFSET_R19]
    LD    %r18, [%sp, OS_CPU_STK_OFFSET_R18]
    LD    %r17, [%sp, OS_CPU_STK_OFFSET_R17]
    LD    %r16, [%sp, OS_CPU_STK_OFFSET_R16]
    LD    %r15, [%sp, OS_CPU_STK_OFFSET_R15]
    LD    %r14, [%sp, OS_CPU_STK_OFFSET_R14]
    LD    %r13, [%sp, OS_CPU_STK_OFFSET_R13]
    LD    %r12, [%sp, OS_CPU_STK_OFFSET_R12]
    LD    %r11, [%sp, OS_CPU_STK_OFFSET_R11]
    LD    %r10, [%sp, OS_CPU_STK_OFFSET_R10]
    LD    %r9, [%sp, OS_CPU_STK_OFFSET_R9]
    LD    %r8, [%sp, OS_CPU_STK_OFFSET_R8]
    LD    %r7, [%sp, OS_CPU_STK_OFFSET_R7]
    LD    %r6, [%sp, OS_CPU_STK_OFFSET_R6]
    LD    %r5, [%sp, OS_CPU_STK_OFFSET_R5]
    LD    %r4, [%sp, OS_CPU_STK_OFFSET_R4]
    LD    %r3, [%sp, OS_CPU_STK_OFFSET_R3]
    LD    %r2, [%sp, OS_CPU_STK_OFFSET_R2]
    LD    %r1, [%sp, OS_CPU_STK_OFFSET_R1]
    LD    %r0, [%sp, OS_CPU_STK_OFFSET_R0]

                                                                ; Adjust stack pointer.
    ADD   %sp, %sp, OS_CPU_STK_OFFSET_END
.endm


;********************************************************************************************************
;                                    SAVE_CONTEXT_FROM_INTERRUPT
;
; Description : Macro used to save the remaining context of a task when saved by an interrupt.
;
; Arguments   : none.
;
; Note(s)     : none.
;********************************************************************************************************

.macro  SAVE_CONTEXT_FROM_INTERRUPT
                                                                ; Save manual part of context
    PUSH  %blink
    PUSH  %r30
    PUSH  %r29
    PUSH  %acch
    PUSH  %accl
.endm


;********************************************************************************************************
;                                    RESTORE_CONTEXT_FROM_INTERRUPT
;
; Description : Macro used to restore the remaining context of a task when restored by an interrupt.
;
; Arguments   : none.
;
; Note(s)     : none.
;********************************************************************************************************

.macro  RESTORE_CONTEXT_FROM_INTERRUPT
                                                                ; Restore manual part of context.
    POP   %accl
    POP   %acch
    POP   %r29
    POP   %r30
    POP   %blink
.endm


;********************************************************************************************************
;                                       CODE GENERATION DIRECTIVES
;********************************************************************************************************

   .text


;********************************************************************************************************
;                                           START MULTITASKING
;                                       void OSStartHighRdy(void)
;
; Description : This function is called by OSStart() to start the highest priority task that was created
;               by your application before calling OSStart().
;
; Arguments   : none
;
; Note(s)     : 1) OSStartHighRdy() MUST:
;                      a) Call OSTaskSwHook().
;                      b) Restore context for OSTCBCurPtr.
;                      c) RTIE into highest ready task.
;********************************************************************************************************

FUNCTION OSStartHighRdy
                                                                ; Call OSTaskSwHook
    BL    OSTaskSwHook

                                                                ; Set Stack region
    BL    OS_CPU_GetStackRegion
    SR    %r1, [OS_CPU_AR_KSTACK_TOP]
    SR    %r0, [OS_CPU_AR_KSTACK_BASE]

                                                                ; Get stack pointer from OSTCBCurPtr
    MOV   %r0, OSTCBCurPtr
    LD    %r0, [%r0]
    LD    %sp, [%r0]

                                                                ; Load ERSTATUS
    LR    %r0, [OS_CPU_AR_STATUS32]                             ;   Merge Global and Local state.
    AND   %r0, %r0, OS_CPU_STATUS32_MASK
    LD    %r1, [%sp, OS_CPU_STK_OFFSET_STATUS32]
    OR    %r1, %r0, %r1
    SR    %r1, [OS_CPU_AR_ERSTATUS]

                                                                ; Restore context.
    RESTORE_CONTEXT_FROM_EXCEPTION
    RTIE


;********************************************************************************************************
;                         PERFORM A CONTEXT SWITCH (From task level) - OSCtxSw()
;
; Note(s) : 1) OSCtxSw() is called when OS wants to perform a task context switch.  This function
;              triggers the TRAP exception which is where the real work is done.
;********************************************************************************************************

FUNCTION OSCtxSw
    TRAP_S  0
    J_S     [%blink]


;********************************************************************************************************
;                     PERFORM A CONTEXT SWITCH (From interrupt level) - OSIntCtxSw()
;
; Note(s) : 1) OSIntCtxSw() is called by OSIntExit() when it determines a context switch is needed as
;              the result of an interrupt.
;********************************************************************************************************

FUNCTION OSIntCtxSw
                                                                ; Call OSTaskSwHook
    BL    OSTaskSwHook
                                                                ; Set Stack region
    BL    OS_CPU_GetStackRegion
    SR    %r1, [OS_CPU_AR_KSTACK_TOP]
    SR    %r0, [OS_CPU_AR_KSTACK_BASE]

                                                                ; OSPrioCur = OSPrioHighRdy;
    MOV   %r0, OSPrioCur
    MOV   %r1, OSPrioHighRdy
    LDB   %r1, [%r1]
    STB   %r1, [%r0]

                                                                ; OSTCBCurPtr = OSTCBHighRdyPtr;
    MOV   %r0, OSTCBCurPtr
    MOV   %r1, OSTCBHighRdyPtr
    LD    %r1, [%r1]
    ST    %r1, [%r0]
                                                                ; Get SP from OSTCBHighRdyPtr->StkPtr.
    LD    %sp, [%r1]

                                                                ; Merge Global and Local state.
    LR    %r0, [OS_CPU_AR_STATUS32]
    AND   %r0, %r0, OS_CPU_STATUS32_MASK
    LD    %r1, [%sp, OS_CPU_STK_OFFSET_STATUS32]
    OR    %r1, %r0, %r1
    ST    %r1, [%sp, OS_CPU_STK_OFFSET_STATUS32]

                                                                ; Restore manual part of context.
    RESTORE_CONTEXT_FROM_INTERRUPT
                                                                ; Restore atomatic part of context.
    RTIE


;********************************************************************************************************
;                                       GENERIC EXCEPTION HANDLER
;                                void OS_CPU_EM6_ExceptionHandler(void)
;
; Note(s) : 1) This is the global exception handler. It handles all exceptions an EM6 core may have.
;
;           2) The global exception handler calls the BSP defined 'OS_CPU_ExceptionHandler()' function to
;              actually handle the exception.
;
;           3) This handler should fill all Vector Table entries for exceptions except for entry 0 (reset)
;              and 9 (Trap).
;********************************************************************************************************

FUNCTION OS_CPU_EM6_ExceptionHandler
                                                                ; Change to Exception Stack.
    MOV   %r13, %sp
    LR    %sp, [OS_CPU_AR_USTACK_BASE]
                                                                ; Call Handler.
    LR    %r0, [OS_CPU_AR_ECR]
    BL    OS_CPU_ExceptionHandler
                                                                ; Restore Stack.
    MOV   %sp, %r13
    RTIE


;********************************************************************************************************
;                                       GENERIC INTERRUPT HANDLER
;                                void OS_CPU_EM6_InterruptHandler(void)
;
; Note(s) : 1) This is the global interrupt handler. It handles all kernel-aware interrupts an EM6 core
;              can observe.
;
;           2) The global interrupt handler calls the BSP defined 'OS_CPU_InterruptHandler()' function to
;              actually handle the interrupt.
;
;           3) This handler should fill all Vector Table entries for interrupts.
;********************************************************************************************************

FUNCTION OS_CPU_EM6_InterruptHandler
                                                                ; Save manual part of context
    SAVE_CONTEXT_FROM_INTERRUPT
                                                                ; OSRunning?
    MOV     %r0, OSRunning
    LDB     %r0, [%r0]
    BRNE_S  %r0, 0, OS_CPU_EM6_InterruptHandler_Running

;********************************************************************************************************
;                               GENERIC INTERRUPT HANDLER: OS NOT RUNNING
;
; Note(s) : 1) If the OS is not running, the pseudo-code is:
;              a) Call OS_CPU_InterruptHandler();
;              b) Restore context and return to interrupted code.
;********************************************************************************************************

OS_CPU_EM6_InterruptHandler_NotRunning:
                                                                ; Call OS_CPU_InterruptHandler();
    LR      %r0, [OS_CPU_AR_ICAUSE]
    BL      OS_CPU_InterruptHandler
                                                                ; Restore manual part of context.
    RESTORE_CONTEXT_FROM_INTERRUPT
                                                                ; Restore atomatic part of context.
    RTIE

;********************************************************************************************************
;                                 GENERIC INTERRUPT HANDLER: OS RUNNING
;
; Note(s) : 1) If the OS is running, the pseudo-code is:
;              a) OSTCBCurPtr->StkPtr = SP;
;              b) Call OSIntEnter();
;              c) Call OS_CPU_InterruptHandler();
;              d) Call OSIntExit();
;              e) (if OSIntExit() returns): Restore context and return to interrupted task.
;********************************************************************************************************

OS_CPU_EM6_InterruptHandler_Running:
                                                                ; Save SP in OSTCBCurPtr->StkPtr.
    MOV     %r0, OSTCBCurPtr
    LD      %r0, [%r0]
    ST      %sp, [%r0]
                                                                ; Change to ISR Stack.
    MOV     %r13, %sp
    LR      %sp, [OS_CPU_AR_USTACK_BASE]
                                                                ; Set stack region.
    LR      %r14, [OS_CPU_AR_USTACK_TOP]
    AEX     %r14, [OS_CPU_AR_KSTACK_TOP]
    LR      %r15, [OS_CPU_AR_USTACK_BASE]
    AEX     %r15, [OS_CPU_AR_KSTACK_BASE]

                                                                ; Call OSIntEnter();
    BL      OSIntEnter
                                                                ; Call OS_CPU_InterruptHandler();
    LR      %r0, [OS_CPU_AR_ICAUSE]
    BL      OS_CPU_InterruptHandler
                                                                ; Call OSIntExit();
    BL      OSIntExit
                                                                ; Restore previous stack.
    MOV     %sp, %r13
                                                                ; Restore Stack region
    SR      %r14, [OS_CPU_AR_KSTACK_TOP]
    SR      %r15, [OS_CPU_AR_KSTACK_BASE]
                                                                ; Restore manual part of context.
    RESTORE_CONTEXT_FROM_INTERRUPT
                                                                ; Restore atomatic part of context.
    RTIE

;********************************************************************************************************
;                                         HANDLE Trap EXCEPTION
;                                   void OS_CPU_EM6_TrapHandler(void)
;
; Note(s) : 1) Trap is used to cause a context switch.
;
;           2) Pseudo-code is:
;              a) Save registers as if saved by interrupt;
;              c) Save the SP (r28): OSTCBCurPtr->StkPtr = SP;
;              d) Call OSTaskSwHook();
;              e) Set current high priority   , OSPrioCur   = OSPrioHighRdy;
;              f) Get current ready thread TCB, OSTCBCurPtr = OSTCBHighRdyPtr;
;              g) Get new SP (r28): SP = OSTCBHighRdyPtr->StkPtr;
;              h) Restore context.
;
;           3) On entry into Trap handler:
;              a) The processor is in Kernel+Exception state.
;              b) OSTCBCurPtr      points to the OS_TCB of the task to suspend
;                 OSTCBHighRdyPtr  points to the OS_TCB of the task to resume
;
;           4) This function MUST be placed in entry 9 (Trap) of the Interruot Vector Table.
;********************************************************************************************************

FUNCTION OS_CPU_EM6_TrapHandler
                                                                ; Save current task's context.
    SAVE_CONTEXT_FROM_EXCEPTION

                                                                ; Save SP in OSTCBCurPtr->StkPtr.
    MOV   %r0, OSTCBCurPtr
    LD    %r0, [%r0]
    ST    %sp, [%r0]

                                                                ; Call OSTaskSwHook
    BL    OSTaskSwHook

                                                                ; Set Stack region
    BL    OS_CPU_GetStackRegion
    SR    %r1, [OS_CPU_AR_KSTACK_TOP]
    SR    %r0, [OS_CPU_AR_KSTACK_BASE]

                                                                ; OSPrioCur = OSPrioHighRdy;
    MOV   %r0, OSPrioCur
    MOV   %r1, OSPrioHighRdy
    LDB   %r1, [%r1]
    STB   %r1, [%r0]

                                                                ; OSTCBCurPtr = OSTCBHighRdyPtr;
    MOV   %r0, OSTCBCurPtr
    MOV   %r1, OSTCBHighRdyPtr
    LD    %r1, [%r1]
    ST    %r1, [%r0]

                                                                ; Get SP from OSTCBHighRdyPtr->StkPtr.
    LD    %sp, [%r1]

                                                                ; Load ERSTATUS
    LR    %r0, [OS_CPU_AR_ERSTATUS]                             ;   Merge Global and Local state.
    AND   %r0, %r0, OS_CPU_STATUS32_MASK
    LD    %r1, [%sp, OS_CPU_STK_OFFSET_STATUS32]
    OR    %r1, %r0, %r1
    SR    %r1, [OS_CPU_AR_ERSTATUS]
                                                                ; Restore context.
    RESTORE_CONTEXT_FROM_EXCEPTION
    RTIE


;********************************************************************************************************
;                                       CODE GENERATION DIRECTIVES
;********************************************************************************************************

.end
