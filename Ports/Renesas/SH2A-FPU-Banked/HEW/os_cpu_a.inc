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
;                                           Renesas SH2A-FPU
;                           Renesas SH SERIES C/C++ Compiler (V.9.00.03.006)
;
; File    : os_cpu_a.inc
; Version : V3.08.02
;********************************************************************************************************


;********************************************************************************************************
;*                                            OS_CTX_SAVE
;*                                     Save CPU and FPU Context
;********************************************************************************************************
        .MACRO    OS_CTX_SAVE
        FMOV.S    FR0,   @-R15                ; Save FPU registers
        FMOV.S    FR1,   @-R15
        FMOV.S    FR2,   @-R15
        FMOV.S    FR3,   @-R15
        FMOV.S    FR4,   @-R15
        FMOV.S    FR5,   @-R15
        FMOV.S    FR6,   @-R15
        FMOV.S    FR7,   @-R15
        FMOV.S    FR8,   @-R15
        FMOV.S    FR9,   @-R15
        FMOV.S    FR10,  @-R15
        FMOV.S    FR11,  @-R15
        STS.L     FPUL,  @-R15
        STS.L     FPSCR, @-R15
        FMOV.S    FR12,  @-R15
        FMOV.S    FR13,  @-R15
        FMOV.S    FR14,  @-R15
        FMOV.S    FR15,  @-R15

        MOV.L     R0,    @-R15				  ; Save R0
		STC.L     TBR,   R0                   ; Save TBR
        MOV.L     R0,    @-R15

        STC.L     GBR,   @-R15                ; Save GBR
        STS.L     PR,    @-R15                ; Save PR

        STS.L     MACL,  @-R15                ; Save DSP registers
        STS.L     MACH,  @-R15

        MOVML.L   R14,   @-R15                ; Save Integer registers
        .ENDM

;********************************************************************************************************
;*                                          OS_CTX_SAVE_ISR
;*                                          Save ISR Context
;********************************************************************************************************
        .MACRO    OS_CTX_SAVE_ISR
        FMOV.S    FR0,   @-R15                ; Save FPU registers
        FMOV.S    FR1,   @-R15
        FMOV.S    FR2,   @-R15
        FMOV.S    FR3,   @-R15
        FMOV.S    FR4,   @-R15
        FMOV.S    FR5,   @-R15
        FMOV.S    FR6,   @-R15
        FMOV.S    FR7,   @-R15
        FMOV.S    FR8,   @-R15
        FMOV.S    FR9,   @-R15
        FMOV.S    FR10,  @-R15
        FMOV.S    FR11,  @-R15
        STS.L     FPUL,  @-R15
        STS.L     FPSCR, @-R15
        FMOV.S    FR12,  @-R15
        FMOV.S    FR13,  @-R15
        FMOV.S    FR14,  @-R15
        FMOV.S    FR15,  @-R15

        MOV.L     R0,    @-R15				  ; Save R0
        STC.L     TBR,   R0                   ; Save TBR
        MOV.L     R0,    @-R15
        .ENDM

;********************************************************************************************************
;*                                       OS_C_ISR_SAVE
;*                                 Save partial FPU and TBR
;********************************************************************************************************
        .MACRO    OS_C_ISR_SAVE
        FMOV.S    FR12,  @-R15                ; Save FR12 to FR15
        FMOV.S    FR13,  @-R15
        FMOV.S    FR14,  @-R15
        FMOV.S    FR15,  @-R15

        MOV.L     R0,    @-R15				  ; Save R0
		STC.L     TBR,   R0                   ; Save TBR
        MOV.L     R0,    @-R15
        .ENDM

;********************************************************************************************************
;*                                             OS_CTX_RESTORE
;*                                        Restore CPU and FPU Context
;********************************************************************************************************

        .MACRO    OS_CTX_RESTORE
        MOVML.L   @R15+, R14                  ; Restore Integer registers

        LDS.L     @R15+, MACH                 ; Restore DSP registers
        LDS.L     @R15+, MACL

        LDS.L     @R15+, PR                   ; Restore PR
        LDC.L     @R15+, GBR                  ; Restore GBR

        MOV.L     @R15+, R0                   ; Restore TBR
        LDC       R0,    TBR
        MOV.L     @R15+, R0                   ; Restore R0

        FMOV.S    @R15+, FR15                 ; Restore FPU registers
        FMOV.S    @R15+, FR14
        FMOV.S    @R15+, FR13
        FMOV.S    @R15+, FR12
        LDS.L     @R15+, FPSCR
        LDS.L     @R15+, FPUL
        FMOV.S    @R15+, FR11
        FMOV.S    @R15+, FR10
        FMOV.S    @R15+, FR9
        FMOV.S    @R15+, FR8
        FMOV.S    @R15+, FR7
        FMOV.S    @R15+, FR6
        FMOV.S    @R15+, FR5
        FMOV.S    @R15+, FR4
        FMOV.S    @R15+, FR3
        FMOV.S    @R15+, FR2
        FMOV.S    @R15+, FR1
        FMOV.S    @R15+, FR0
        .ENDM

;********************************************************************************************************
;*                                             OS_CTX_RESTORE_ISR
;*                                          Restore Context from ISR
;********************************************************************************************************

        .MACRO    OS_CTX_RESTORE_ISR
        MOV.L     @R15+, R0                   ; Restore TBR
        LDC       R0,    TBR
        MOV.L     @R15+, R0                   ; Restore R0

        FMOV.S    @R15+, FR15                 ; Restore FPU registers
        FMOV.S    @R15+, FR14
        FMOV.S    @R15+, FR13
        FMOV.S    @R15+, FR12
        LDS.L     @R15+, FPSCR
        LDS.L     @R15+, FPUL
        FMOV.S    @R15+, FR11
        FMOV.S    @R15+, FR10
        FMOV.S    @R15+, FR9
        FMOV.S    @R15+, FR8
        FMOV.S    @R15+, FR7
        FMOV.S    @R15+, FR6
        FMOV.S    @R15+, FR5
        FMOV.S    @R15+, FR4
        FMOV.S    @R15+, FR3
        FMOV.S    @R15+, FR2
        FMOV.S    @R15+, FR1
        FMOV.S    @R15+, FR0
        .ENDM

;********************************************************************************************************
;*                                     OS_C_ISR_RESTORE
;*                                Restore partial FPU and TBR
;********************************************************************************************************
        .MACRO    OS_C_ISR_RESTORE
        MOV.L     @R15+, R0                   ; Restore TBR
        LDC       R0,    TBR
        MOV.L     @R15+, R0                   ; Restore R0

        FMOV.S    @R15+, FR15                 ; Restore FPU registers
        FMOV.S    @R15+, FR14
        FMOV.S    @R15+, FR13
        FMOV.S    @R15+, FR12
        .ENDM
