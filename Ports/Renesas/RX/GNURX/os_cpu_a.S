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
;                                       Renesas RX Specific Code
;
; File      : os_cpu_a.S
; Version   : V3.08.01
;********************************************************************************************************
; For       : Renesas RX
; Toolchain : HEW      with GNURX compiler
;             E2STUDIO with GNURX compiler
;********************************************************************************************************


;********************************************************************************************************
;                                           PUBLIC FUNCTIONS
;********************************************************************************************************

    .global     _OSTCBCurPtr                ; Declared as OS_TCB *               , 32-bit long
    .global     _OSTCBHighRdyPtr            ; Declared as OS_TCB *               , 32-bit long
    .global     _OSPrioCur                  ; Declared as INT8U                  ,  8-bit long
    .global     _OSPrioHighRdy              ; Declared as INT8U                  ,  8-bit long
    .global     _OSTaskSwHook

    .global     _OSStartHighRdy
    .global     _OSCtxSwISR
    .global     _OS_KA_IPL_Boundary

;********************************************************************************************************
;                                      CODE GENERATION DIRECTIVES
;********************************************************************************************************

    .text
	.section    .text


;*********************************************************************************************************
;                                         START MULTITASKING
;
; Description : This function is called by OSStart() to start the highest priority task that was created
;               by your application before calling OSStart().
;
; Arguments   : none
;
; Note(s)     : 1) The stack frame for the RX100 and RX200 families is assumed to look as follows:
;
;                                                                    LOW MEMORY
;                  OSTCBHighRdy->OSTCBStkPtr +  0  ---->  MACHI
;                                            +  4         MACMI
;                                            +  8         R1  (i.e. p_arg)     (32-bit contents of 'p_arg')
;                                            + 12         R2
;                                            + 16         R3
;                                            + 20         R4
;                                            + 24         R5
;                                            + 28         R6
;                                            + 32         R7
;                                            + 36         R8
;                                            + 40         R9
;                                            + 44         R10
;                                            + 48         R11
;                                            + 52         R12
;                                            + 56         R12
;                                            + 60         R14
;                                            + 64         R15
;                                            + 68         task                 (32-bit address of 'task')
;                                            + 72         PSW                  (Initial value of 0x0000)
;                                                                    HIGH MEMORY
;
;               2) The stack frame for the RX600 family is assumed to look as follows:
;
;                                                                    LOW MEMORY
;                  OSTCBHighRdy->OSTCBStkPtr +  0  ---->  MACHI
;                                            +  4         MACMI
;                                            +  8         FPSW
;                                            + 12         R1  (i.e. p_arg)     (32-bit contents of 'p_arg')
;                                            + 16         R2
;                                            + 20         R3
;                                            + 24         R4
;                                            + 28         R5
;                                            + 32         R6
;                                            + 36         R7
;                                            + 40         R8
;                                            + 44         R9
;                                            + 48         R10
;                                            + 52         R11
;                                            + 56         R12
;                                            + 60         R12
;                                            + 64         R14
;                                            + 68         R15
;                                            + 72         task                 (32-bit address of 'task')
;                                            + 76         PSW                  (Initial value of 0x0000)
;                                                                    HIGH MEMORY
;
;               3) OSStartHighRdy() MUST:
;                      a) Call OSTaskSwHook() then,
;                      b) Switch to the highest priority task.
;*********************************************************************************************************

_OSStartHighRdy:
    MOV.L   #_OSTaskSwHook, R5          ; Execute task switch hook
    JSR     R5

    SETPSW  U

    MOV.L   #_OSTCBHighRdyPtr, R5      ; SP = OSTCBHighRdyPtr->OSTCBStkPtr
    MOV.L   [R5], R2
    MOV.L   [R2], R0

                                       ; RESTORE CONTEXT OF NEW TASK
    POPM    R1-R2                      ; Pop Accumulator registers
    SHLL    #16, R2                    ; Correct for mid read/low write
    MVTACLO R2                         ; Write Accumulator
    MVTACHI R1

#if (defined(__RX_FPU_INSNS__) && (__RX_FPU_INSNS__==1))
    POPC    FPSW                       ; Restore FP status word
#endif

    POPM    R1-R15                     ; Restore registers
    RTE                                ; Restore PSW and PC


;*********************************************************************************************************
;                                    PENDED CONTEXT SWITCH ISR
;
; Description : This ISR Performs all context switching within the OS.
;
; Arguments   : none
;
; Note(s)     : 1) Upon entry...
;                  OSTCBCur       Points to the OS_TCB of the task to suspend
;                  OSTCBHighRdy   Contains the pointer from the last scheduling
;
;
;               2) The interrupt stack frame looks as follows.  This was caused by the
;                  execution of SWINT event (the registers for the task to suspend need to be
;                  saved):
;
;                                                                         LOW MEMORY
;                                      ISP +  0  ---->  32-bit PC of task
;                                          +  4         32-bit PSW
;                                          +  8
;                                                                         HIGH MEMORY
;
;
;               3) The stack frame of the task to resume (for RX100 and RX200 families) looks as follows:
;
;                                                                         LOW MEMORY
;                  OSTCBHighRdyPtr->StkPtr +  0  ---->  MACHI
;                                          +  4         MACLO
;                                          +  8         R1 (p_arg)
;                                          + 12         R2
;                                          + 16         R3
;                                          + 20         R4
;                                          + 24         R5
;                                          + 28         R6
;                                          + 32         R7
;                                          + 36         R8
;                                          + 40         R9
;                                          + 44         R10
;                                          + 48         R11
;                                          + 52         R12
;                                          + 56         R12
;                                          + 60         R14
;                                          + 64         R15
;                                          + 68         task        (32-bit address of 'task')
;                                          + 72         PSW
;                                                                         HIGH MEMORY
;
;
;               4) The stack frame of the task to resume (for the RX600 family) looks as follows:
;
;                                                                         LOW MEMORY
;                  OSTCBHighRdyPtr->StkPtr +  0  ---->  MACHI
;                                          +  4         MACLO
;                                          +  8         FPSW
;                                          + 12         R1 (p_arg)
;                                          + 16         R2
;                                          + 20         R3
;                                          + 24         R4
;                                          + 28         R5
;                                          + 32         R6
;                                          + 36         R7
;                                          + 40         R8
;                                          + 44         R9
;                                          + 48         R10
;                                          + 52         R11
;                                          + 56         R12
;                                          + 60         R12
;                                          + 64         R14
;                                          + 68         R15
;                                          + 72         task        (32-bit address of 'task')
;                                          + 76         PSW
;                                                                         HIGH MEMORY
;*********************************************************************************************************


_OSCtxSwISR:
    SETPSW  I                          ; Re-enable higher priority interrupts.

                                       ; Transfer Interrupt Stack Contents to the current user stack
    PUSH.L  R15                        ; Save R15 to interrupt stack as we'll use for temporary register.
    MVFC    USP, R15                   ; Read the user stack pointer
    SUB     #12, R15                   ; Create space on user stack
    MVTC    R15, USP                   ; Update USP

                                       ; SAVE CONTEXT OF TASK BEING SWITCHED OUT
                                       ; Copy the data across from interrupt stack
    MOV.L   8[R0], 8[R15]              ; PSW
    MOV.L   4[R0], 4[R15]              ; PC
    MOV.L   0[R0], 0[R15]              ; R15
    ADD     #12, R0                    ; correct ISP for data we are done with using (RTE will be from USP)
    SETPSW  U                          ; Switch to user stack for pushing rest of registers
    PUSHM   R1-R14                     ; Save remaining registers

#if (defined(__RX_FPU_INSNS__) && (__RX_FPU_INSNS__==1))
    PUSHC   FPSW                       ; Save FPSW
#endif

    MVFACHI R1                         ; Save special registers
    MVFACMI R2
    PUSHM   R1-R2

                                       ; Context is saved, registers can be used, handle OS requirements
    MOV.L   #_OS_KA_IPL_Boundary, R2   ; Set IPL to Kernel Awareness boundary
    MOV.L   [R2], R1
    MVFC    PSW,  R2
    AND     #-0F000001H, R2
    OR      R1,   R2
    MVTC    R2,   PSW

    MOV.L   #_OSTCBCurPtr, R6          ; Save current task's SP into its TCB
    MOV.L   [R6], R2
    MOV.L   R0, [R2]

    MOV.L   #_OSTaskSwHook, R5         ; Execute task switch hook
    JSR     R5

    MOV.L   #_OSPrioCur, R3            ; OSPrioCur = OSPrioHighRdy
    MOV.L   #_OSPrioHighRdy, R2
    MOV.B   [R2], [R3]

    MOV.L   #_OSTCBHighRdyPtr, R2      ; OSTCBCurPtr = OSTCBHighRdyPtr
    MOV.L   [R2], [R6]
    MOV.L   [R6], R2
    MOV.L   [R2], R0                   ; SP = OSTCBCurPtr->StkPtr

    MVTIPL  #1                         ; Restore IPL to level 1

                                       ; RESTORE CONTEXT OF NEW TASK
    POPM    R1-R2                      ; Load ACC registers from stack
    SHLL    #16, R2                    ; Correct for reading mid/writing low
    MVTACHI R1                         ; Restore special registers
    MVTACLO R2

#if (defined(__RX_FPU_INSNS__) && (__RX_FPU_INSNS__==1))
        POPC    FPSW                       ; Restore FP status word
#endif

    POPM    R1-R15
    RTE

    .END
