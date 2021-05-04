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
;                                             TI C28x Port
;
; File      : os_cpu_a.asm
; Version   : V3.08.01
;********************************************************************************************************
; For       : TI C28x
; Mode      : C28 Object mode
; Toolchain : TI C/C++ Compiler
;********************************************************************************************************

;********************************************************************************************************
;                                             INCLUDE FILES
;********************************************************************************************************

    .include "os_cpu_i.asm"


;********************************************************************************************************
;                                           PUBLIC FUNCTIONS
;********************************************************************************************************

    .def   _OS_CPU_GetST0
    .def   _OS_CPU_GetST1
    .def   _OSStartHighRdy
    .def   _OS_CPU_RTOSINT_Handler
    .def   _OS_CPU_INT_Handler


;********************************************************************************************************
;                                       EXTERNAL GLOBAL VARIABLES
;********************************************************************************************************

    .ref   _OSTCBCurPtr                                         ; Declared as OS_TCB * , 32-bit long
    .ref   _OSTCBHighRdyPtr                                     ; Declared as OS_TCB * , 32-bit long
    .ref   _OSPrioCur                                           ; Declared as INT8U    , 16-bit long
    .ref   _OSPrioHighRdy                                       ; Declared as INT8U    , 16-bit long
    .ref   _OSTaskSwHook
    .ref   _OS_CPU_IntHandler


;********************************************************************************************************
;                                      CODE GENERATION DIRECTIVES
;********************************************************************************************************
                                                                ; Set text section and reset local labels.
    .text
    .newblock


;********************************************************************************************************
;                                            GET ST0 and ST1
;
; Description : Wrapper function to get ST0 and ST1 registers from a C function.
;
; Prototypes  : CPU_INT16U  OS_CPU_GetST0(void);
;               CPU_INT16U  OS_CPU_GetST1(void);
;********************************************************************************************************

    .asmfunc
_OS_CPU_GetST0:
    PUSH    ST0
    POP     AL
    LRETR
    .endasmfunc

    .asmfunc
_OS_CPU_GetST1:
    PUSH    ST1
    POP     AL
    LRETR
    .endasmfunc


;*********************************************************************************************************
;                                         START MULTITASKING
;
; Description : This function is called by OSStart() to start the highest priority task that was created
;               by your application before calling OSStart().
;
; Arguments   : none
;
; Note(s)     : 1) OSStartHighRdy() MUST:
;                      a) Call OSTaskSwHook().
;                      b) Restore context for OSTCBCurPtr.
;                      c) IRET into highest ready task.
;*********************************************************************************************************

    .asmfunc
_OSStartHighRdy:
                                                                ; Call OSTaskSwHook()
    LCR     #_OSTaskSwHook
                                                                ; Restore context.
    MOVL    XAR0, #_OSTCBCurPtr                                 ; Get the process's SP.
    MOVL    XAR1, *XAR0
    MOV     AL  , *AR1
    MOV    @SP  ,  AL
                                                                ; Restore registers.
    OS_CTX_RESTORE
                                                                ; IRET into task.
    IRET
    .endasmfunc
                                                                ; Catch start high failure.
OSStartHang:
    SB      OSStartHang, UNC


;********************************************************************************************************
;                                     GENERIC INTERRUPT HANDLER
;                                   void OS_CPU_INT_Handler(void)
;
; Note(s) : 1) Assembly wrapper for ISRs.
;
;           2) Saves task context before servicing the interrupt and maintains the value of IER across
;              interrupts.
;********************************************************************************************************

    .asmfunc
_OS_CPU_INT_Handler:

    OS_CTX_SAVE

    ASP

    LCR     #_OS_CPU_IntHandler

    NASP

    OS_CTX_RESTORE

    IRET
    .endasmfunc


;********************************************************************************************************
;                                       HANDLE RTOSINT INTERRUPT
;                                   void OS_CPU_RTOSINT_Handler(void)
;
; Note(s) : 1) The RTOSINT interrupt is used to perform a context switch. The C28x core saves the
;              ST0, T, ACC, P, AR0, AR1, ST1, DP, IER, DBGSTAT (shadow) registers and the Return
;              Address.
;              The remaining registers AR1H, AR0H, XAR2..XAR7, XT and RPC are saved by the handler.
;
;           2) The context switching RTOSINT handler pseudo-code is:
;              a) Save remaining registers on the process stack;
;              b) Save the process SP in its TCB, OSTCBCurPtr->OSTCBStkPtr = SP;
;              c) Call OSTaskSwHook();
;              d) Get current high priority, OSPrioCur = OSPrioHighRdy;
;              e) Get current ready thread TCB, OSTCBCurPtr = OSTCBHighRdyPtr;
;              f) Get new process SP from TCB, SP = OSTCBHighRdyPtr->OSTCBStkPtr;
;              g) Restore AR1H, AR0H, XAR2..XAR7, XT and RPC registers from the new process stack;
;              h) Overwrite the previously saved (at context switch out) IER register with current IER.
;              i) Perform exception return which will restore remaining context.
;
;           3) On entry into RTOSINT handler:
;              a) The following have been saved on the process stack (by processor):
;                 ST0, T, ACC, P, AR0, AR1, ST1, DP, IER, DBGSTAT (shadow) registers and the Return
;                 Address.
;              b) OSTCBCurPtr      points to the OS_TCB of the task to suspend
;                 OSTCBHighRdyPtr  points to the OS_TCB of the task to resume
;
;           4) This function MUST be placed in entry 16 (for RTOSINT) of the C28x interrupt table.
;********************************************************************************************************

    .asmfunc
_OS_CPU_RTOSINT_Handler:

                                                                ; Save registers.
    OS_CTX_SAVE
                                                                ; Save SP to current process.
    MOVL    XAR0, #_OSTCBCurPtr                                 ; Get the process's SP.
    MOVL    XAR1, *XAR0
    MOV     AL  , @SP
    MOV     *AR1,  AL

    ASP                                                         ; Align the stack pointer.
                                                                ; Call OSTaskSwHook.
    LCR     #_OSTaskSwHook
    NASP                                                        ; Restore alignement of the stack pointer.

                                                                ; OSPrioCur = OSPrioHighRdy
    MOVL    XAR0, #_OSPrioHighRdy
    MOVL    XAR1, #_OSPrioCur
    MOV     ACC, *XAR0
    MOV     *XAR1, ACC
                                                                ; OSTCBCurPtr = OSTCBHighRdyPtr
    MOVL    XAR0, #_OSTCBHighRdyPtr
    MOVL    XAR1, #_OSTCBCurPtr
    MOVL    ACC, *XAR0
    MOVL    *XAR1, ACC
                                                                ; Get SP from new process.
    MOVL    XAR0, *XAR1
    MOV     AL  , *AR0
    MOV     @SP , AL

    OS_CTX_RESTORE
                                                                ; Return from interrupt to restore remaining registers.
    IRET
    .endasmfunc


;********************************************************************************************************
;                                     OS-III ASSEMBLY PORT FILE END
;********************************************************************************************************

    .end
