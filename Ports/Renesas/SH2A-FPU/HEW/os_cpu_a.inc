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
;                                           Renesas SH-2A-FPU
;                           Renesas SH SERIES C/C++ Compiler (V.9.00.03.006)
;
; File    : os_cpu_a.inc
; Version : V3.08.01
;********************************************************************************************************

    .IMPORT  _OSIntNestingCtr
    .IMPORT  _OSTCBCurPtr
    .IMPORT  _OSIntExit

    .SECTION    P, CODE

;********************************************************************************************************
;                                             OS_CTX_SAVE
;                                          Save CPU Context
;********************************************************************************************************

        .MACRO    OS_CTX_SAVE                 ; save processor registers on the stack
        movml.l   r15,   @-r15                ; push registers PR, R14, R13 ... R0
        sts.l     macl,  @-r15
        sts.l     mach,  @-r15
        stc.l     tbr,   r0
        mov.l     r0,    @-r15
        stc.l     gbr,   @-r15
        sts.l     fpul,  @-r15
        sts.l     fpscr, @-r15
        fmov.s    fr15,  @-r15
        fmov.s    fr14,  @-r15
        fmov.s    fr13,  @-r15
        fmov.s    fr12,  @-r15
        fmov.s    fr11,  @-r15
        fmov.s    fr10,  @-r15
        fmov.s    fr9,   @-r15
        fmov.s    fr8,   @-r15
        fmov.s    fr7,   @-r15
        fmov.s    fr6,   @-r15
        fmov.s    fr5,   @-r15
        fmov.s    fr4,   @-r15
        fmov.s    fr3,   @-r15
        fmov.s    fr2,   @-r15
        fmov.s    fr1,   @-r15
        fmov.s    fr0,   @-r15
        .ENDM

;********************************************************************************************************
;                                           OS_CTX_RESTORE
;                                         Restore CPU Context
;********************************************************************************************************

        .MACRO    OS_CTX_RESTORE              ; restore all processor registers from new task's stack
        fmov.s    @r15+, fr0
        fmov.s    @r15+, fr1
        fmov.s    @r15+, fr2
        fmov.s    @r15+, fr3
        fmov.s    @r15+, fr4
        fmov.s    @r15+, fr5
        fmov.s    @r15+, fr6
        fmov.s    @r15+, fr7
        fmov.s    @r15+, fr8
        fmov.s    @r15+, fr9
        fmov.s    @r15+, fr10
        fmov.s    @r15+, fr11
        fmov.s    @r15+, fr12
        fmov.s    @r15+, fr13
        fmov.s    @r15+, fr14
        fmov.s    @r15+, fr15
        lds.l     @r15+, fpscr
        lds.l     @r15+, fpul
        ldc.l     @r15+, gbr
        mov.l     @r15+, r0
        ldc       r0,    tbr
        lds.l     @r15+, mach
        lds.l     @r15+, macl
        movml.l   @r15+, r15                  ; restores R0, R1 ... R14, PR
        .ENDM


;********************************************************************************************************
;                                            OS_ISR_ENTER
;
; Description : Interrupt service routine prologue for kernel-aware handler.
;
; Note(s)     : 1) A kernel-aware handler should be written as follows:
;
;                      MyKernelAwareISR:
;                          OS_ISR_ENTER
;                          :               ; service ISR
;                          OS_ISR_EXIT
;********************************************************************************************************

        .MACRO    OS_ISR_ENTER

            OS_CTX_SAVE                     ; Save context & Notify uC/OS about ISR


            mov.l   #_OSIntNestingCtr, r1       ; Set OSIntNestingCtr == 1 (interrupt nesting not supported)
            mov.l   #1, r2
            mov.b   r2, @r1


            mov.l   #_OSTCBCurPtr, r1           ; Save current task's SP into its TCB
            mov.l   @r1, r0
            mov     r15, @r0

        .ENDM


;********************************************************************************************************
;                                             OS_ISR_EXIT
;
; Description : Interrupt service routine epilog for kernel-aware handler.
;
; Note(s)     : 1) A kernel-aware handler should be written as follows:
;
;                      MyKernelAwareISR:
;                          OS_ISR_ENTER
;                          :               ; service ISR
;                          OS_ISR_EXIT
;********************************************************************************************************

        .MACRO    OS_ISR_EXIT

            nop
            mov.l   #_OSIntExit, r0
            jsr     @r0                         ; call OSIntExit()
            nop

            OS_CTX_RESTORE                  ; Restore context

            rte                             ; return from interrupt
            nop

        .ENDM
