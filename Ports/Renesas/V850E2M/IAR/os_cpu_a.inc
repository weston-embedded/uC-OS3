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
;                                         ASSEMBLY LANGUAGE PORT
;                                          Renesas V850E2M Port
;
; File      : os_cpu_a.inc
; Version   : V3.08.02
;********************************************************************************************************
; For       : Renesas V850E2M
; Toolchain : IAR EWV850 v3.7x and 3.8x
;********************************************************************************************************


;********************************************************************************************************
;                                           PUBLIC FUNCTIONS
;********************************************************************************************************

    EXTERN    OSTCBCurPtr                                       ; Declared as 'OS_TCB *'
    EXTERN    OSIntNestingCtr                                   ; Declared as 'CPU_INT08U'
    EXTERN    OSIntExit                                         ; OSIntExit()

;********************************************************************************************************
;                                                EQUATES
;********************************************************************************************************

                                                                ; PROGRAM REGISTER STACK OFFSET
STK_OFFSET_R1     EQU   0
STK_OFFSET_R2     EQU   STK_OFFSET_R1    + 4
STK_OFFSET_R5     EQU   STK_OFFSET_R2    + 4
STK_OFFSET_R6     EQU   STK_OFFSET_R5    + 4
STK_OFFSET_R7     EQU   STK_OFFSET_R6    + 4
STK_OFFSET_R8     EQU   STK_OFFSET_R7    + 4
STK_OFFSET_R9     EQU   STK_OFFSET_R8    + 4
STK_OFFSET_R10    EQU   STK_OFFSET_R9    + 4
STK_OFFSET_R11    EQU   STK_OFFSET_R10   + 4
STK_OFFSET_R12    EQU   STK_OFFSET_R11   + 4
STK_OFFSET_R13    EQU   STK_OFFSET_R12   + 4
STK_OFFSET_R14    EQU   STK_OFFSET_R13   + 4
STK_OFFSET_R15    EQU   STK_OFFSET_R14   + 4
STK_OFFSET_R16    EQU   STK_OFFSET_R15   + 4
STK_OFFSET_R17    EQU   STK_OFFSET_R16   + 4
STK_OFFSET_R18    EQU   STK_OFFSET_R17   + 4
STK_OFFSET_R19    EQU   STK_OFFSET_R18   + 4
STK_OFFSET_R20    EQU   STK_OFFSET_R19   + 4
STK_OFFSET_R21    EQU   STK_OFFSET_R20   + 4
STK_OFFSET_R22    EQU   STK_OFFSET_R21   + 4
STK_OFFSET_R23    EQU   STK_OFFSET_R22   + 4
STK_OFFSET_R24    EQU   STK_OFFSET_R23   + 4
STK_OFFSET_R26    EQU   STK_OFFSET_R24   + 4
STK_OFFSET_R27    EQU   STK_OFFSET_R26   + 4
STK_OFFSET_R28    EQU   STK_OFFSET_R27   + 4
STK_OFFSET_R29    EQU   STK_OFFSET_R28   + 4
STK_OFFSET_R31    EQU   STK_OFFSET_R29   + 4
                                                                ; SYSTEM REGISTER STACK OFFSET
STK_OFFSET_EIPC   EQU   STK_OFFSET_R31   + 4
STK_OFFSET_EIPSW  EQU   STK_OFFSET_EIPC  + 4

STK_CTX_SIZE      EQU   STK_OFFSET_EIPSW + 4

;********************************************************************************************************
;                                      CODE GENERATION DIRECTIVES
;********************************************************************************************************

    RSEG CODE:CODE:NOROOT(2)

;********************************************************************************************************
;                                             OS_CTX_SAVE
;
; Description : This MACRO saves the CPU registers (i.e. CPU context) onto the current task's stack using
;               the same order as they were saved in OSTaskStkInit().
;
; Note(s)     : none
;********************************************************************************************************

OS_CTX_SAVE  MACRO  REG_SP                                      ; SAVE PROCESSOR REGISTER
   	addi  -STK_CTX_SIZE, REG_SP, REG_SP                         ; Adjust the Stack Pointer

	st.w   r1, STK_OFFSET_R1[REG_SP]
	st.w   r2, STK_OFFSET_R2[REG_SP]
   	st.w   r5, STK_OFFSET_R5[REG_SP]
	st.w   r6, STK_OFFSET_R6[REG_SP]
	st.w   r7, STK_OFFSET_R7[REG_SP]
	st.w   r8, STK_OFFSET_R8[REG_SP]
	st.w   r9, STK_OFFSET_R9[REG_SP]
	st.w  r10, STK_OFFSET_R10[REG_SP]
	st.w  r11, STK_OFFSET_R11[REG_SP]
	st.w  r12, STK_OFFSET_R12[REG_SP]
	st.w  r13, STK_OFFSET_R13[REG_SP]
	st.w  r14, STK_OFFSET_R14[REG_SP]
	st.w  r15, STK_OFFSET_R15[REG_SP]
	st.w  r16, STK_OFFSET_R16[REG_SP]
	st.w  r17, STK_OFFSET_R17[REG_SP]
	st.w  r18, STK_OFFSET_R18[REG_SP]
	st.w  r19, STK_OFFSET_R19[REG_SP]
	st.w  r20, STK_OFFSET_R20[REG_SP]
	st.w  r21, STK_OFFSET_R21[REG_SP]
	st.w  r22, STK_OFFSET_R22[REG_SP]
	st.w  r23, STK_OFFSET_R23[REG_SP]
	st.w  r24, STK_OFFSET_R24[REG_SP]
	st.w  r26, STK_OFFSET_R26[REG_SP]
	st.w  r27, STK_OFFSET_R27[REG_SP]
	st.w  r28, STK_OFFSET_R28[REG_SP]
	st.w  r29, STK_OFFSET_R29[REG_SP]
	st.w  r31, STK_OFFSET_R31[REG_SP]

	stsr  EIPC, r2
	st.w  r2, STK_OFFSET_EIPC[REG_SP]                           ; Restore task's EIPC

	stsr  EIPSW, r2
	st.w  r2, STK_OFFSET_EIPSW[REG_SP]                          ; Restore task's EIPSW
    ENDM

;********************************************************************************************************
;                                           OS_CTX_RESTORE
;
; Description : This MACRO restores the CPU registers (i.e. context) from the new task's stack in the
;               reverse order of OS_CTX_SAVE (see above)
;
; Note(s)     : none
;********************************************************************************************************

OS_CTX_RESTORE  MACRO  REG_SP                                   ; RESTORE PROCESSOR REGISTER
	ld.w  STK_OFFSET_R1[REG_SP] , r1
   	ld.w  STK_OFFSET_R5[REG_SP] , r5
	ld.w  STK_OFFSET_R6[REG_SP] , r6
	ld.w  STK_OFFSET_R7[REG_SP] , r7
	ld.w  STK_OFFSET_R8[REG_SP] , r8
	ld.w  STK_OFFSET_R9[REG_SP] , r9
	ld.w  STK_OFFSET_R10[REG_SP], r10
	ld.w  STK_OFFSET_R11[REG_SP], r11
	ld.w  STK_OFFSET_R12[REG_SP], r12
	ld.w  STK_OFFSET_R13[REG_SP], r13
	ld.w  STK_OFFSET_R14[REG_SP], r14
	ld.w  STK_OFFSET_R15[REG_SP], r15
	ld.w  STK_OFFSET_R16[REG_SP], r16
	ld.w  STK_OFFSET_R17[REG_SP], r17
	ld.w  STK_OFFSET_R18[REG_SP], r18
	ld.w  STK_OFFSET_R19[REG_SP], r19
	ld.w  STK_OFFSET_R20[REG_SP], r20
	ld.w  STK_OFFSET_R21[REG_SP], r21
	ld.w  STK_OFFSET_R22[REG_SP], r22
	ld.w  STK_OFFSET_R23[REG_SP], r23
	ld.w  STK_OFFSET_R24[REG_SP], r24
	ld.w  STK_OFFSET_R26[REG_SP], r26
	ld.w  STK_OFFSET_R27[REG_SP], r27
	ld.w  STK_OFFSET_R28[REG_SP], r28
	ld.w  STK_OFFSET_R29[REG_SP], r29
	ld.w  STK_OFFSET_R31[REG_SP], r31

	ld.w  STK_OFFSET_EIPSW[REG_SP], r2                          ; Restore task's EIPSW
	ldsr  r2, EIPSW

	ld.w  STK_OFFSET_EIPC[REG_SP], r2                           ; Restore task's EIPC
	ldsr  r2, EIPC

	ld.w  STK_OFFSET_R2[REG_SP] , r2

	addi STK_CTX_SIZE, REG_SP, REG_SP                           ; Adjust the Stack Pointer
    ENDM

;********************************************************************************************************
;                                            OS_ISR_ENTER
;
; Description : Interrupt service routine prologue for kernel-aware handler.
;               This macro implements the following code in assembly language:
;
;                  OS_ISR_ENTER
;                      OS_CTX_SAVE                   ; Call the macro: OS_CTX_SAVE
;                      OSIntNestingCtr++;
;                      if (OSIntNestingCtr == 1) {
;                          OSTCBCurPtr->StkPtr = SP;
;                      }
;
;               This MACRO is to be used by your assembly language based ISRs as follows:
;
;                  MyISR
;                      OS_ISR_ENTER
;                      ISR Body here
;                      OS_ISR_EXIT
;********************************************************************************************************

OS_ISR_ENTER  MACRO
    OS_CTX_SAVE  sp                                             ; Save processor registers on the stack

    mov OSIntNestingCtr, r11                                    ; OSIntNestingCtr++;
    ld.b 0[r11], r2
    add 0x1, r2
    st.b r2, 0[r11]

    cmp  0x1, r2
    bne  b                                                      ; if (OSIntNestingCtr == 1) {

    mov   OSTCBCurPtr, r11                                      ;     OSTCBCurPtr->StkPtr = SP;
    ld.w  0[r11]     , r11
    st.w  sp         , 0[r11]                                   ; }

b:
    ENDM

;********************************************************************************************************
;                                             OS_ISR_EXIT
;
; Description : Interrupt service routine epilog for kernel-aware handler.
;               This macro implements the following code in assembly language:
;
;                  OS_ISR_EXIT:
;                      OSIntExit();                  ; Call the C function: OSIntExit();
;                      OS_CTX_RESTORE                ; Call the macro: OS_CTX_RESTORE
;                      Return from interrupt         ; CPU instruction to return from interrupt/exception
;********************************************************************************************************

OS_ISR_EXIT  MACRO
    jarl OSIntExit, lp                                          ; Call 'OSIntExit()'

    OS_CTX_RESTORE  sp                                          ; Restore processor registers from stack
    eiret                                                       ; CPU instruction to return from Interrupt/exception

    ENDM

;$PAGE
;********************************************************************************************************
;                                     ASSEMBLY LANGUAGE MACROS FILE END
;********************************************************************************************************
