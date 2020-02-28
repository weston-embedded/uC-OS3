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
;                                           PIC24 MPLab Port
;
; File    : os_cpu_util_a.s
: Version : V3.08.00
;********************************************************************************************************

;
;********************************************************************************************************
;                                            MACRO OS_REGS_SAVE
;
; Description : This macro saves the current state of the CPU onto the current tasks stack
;
; Notes       : W15 is the CPU stack pointer. It should never be pushed from the stack during
;               a context save.
;********************************************************************************************************
;

.macro OS_REGS_SAVE                                                        ; Start of Macro
    push.d w0                                                           ; Push W0 and W1 on to the stack
    push.d w2                                                           ; Push W2 and W3 on to the stack
    push.d w4                                                           ; Push W4 and W5 on to the stack
    push.d w6                                                           ; Push W6 and W7 on to the stack
    push.d w8                                                           ; Push W8 and W9 on to the stack
    push.d w10                                                          ; Push W10 and W11 on to the stack
    push.d w12                                                          ; Push W12 and W13 on to the stack
    push w14                                                            ; Push W14 **ONLY** on to the stack

    push TBLPAG                                                         ; Push the Table Page Register on to the stack
    push PSVPAG                                                         ; Push the Program Space Visability Register on the stack
    push RCOUNT                                                         ; Push the Repeat Loop Counter Register on to the stack

    push SR                                                             ; Push the CPU Status Register on to the stack
    push CORCON                                                         ; Push the Core Control Register on to the stack
.endm                                                                   ; End of Macro

;
;********************************************************************************************************
;                                            MACRO OS_REGS_RESTORE
;
; Description : This macro restores the current state of the CPU from the current tasks stack
;
; Notes       : 1) W15 is the CPU stack pointer. It should never be popped from the stack during
;                  a context restore.
;               2) Registers are always popped in the reverse order from which they were pushed
;********************************************************************************************************
;

.macro OS_REGS_RESTORE                                                     ; Start of Macro
    pop CORCON                                                          ; Pull the Core Control Register from the stack
    pop SR                                                              ; Pull the CPU Status Register from the stack

    pop RCOUNT                                                          ; Pull the Repeat Loop Counter Register from the stack
    pop PSVPAG                                                          ; Pull the Program Space Visability Register on the stack
    pop TBLPAG                                                          ; Pull the Table Page Register from the stack

    pop w14                                                             ; Pull W14 **ONLY** from the stack
    pop.d w12                                                           ; Pull W12 and W13 from the stack
    pop.d w10                                                           ; Pull W10 and W11 from the stack
    pop.d w8                                                            ; Pull W8 and W9 from the stack
    pop.d w6                                                            ; Pull W6 and W7 from the stack
    pop.d w4                                                            ; Pull W4 and W5 from the stack
    pop.d w2                                                            ; Pull W2 and W3 from the stack
    pop.d w0                                                            ; Pull W0 and W1 from the stack
.endm                                                                   ; End of Macro
