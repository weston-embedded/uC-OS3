#********************************************************************************************************
#                                              uC/OS-III
#                                        The Real-Time Kernel
#
#                    Copyright 2009-2020 Silicon Laboratories Inc. www.silabs.com
#
#                                 SPDX-License-Identifier: APACHE-2.0
#
#               This software is subject to an open source license and is distributed by
#                Silicon Laboratories Inc. pursuant to the terms of the Apache License,
#                    Version 2.0 available at www.apache.org/licenses/LICENSE-2.0.
#
#********************************************************************************************************

#********************************************************************************************************
#
#                                       ASSEMBLY LANGUAGE MACROS
#
#                                           MPC57xx VLE Port
#                                             GNU Toolchain
#
# File    : os_cpu_a.inc
# Version : V3.08.01
#********************************************************************************************************

#********************************************************************************************************
#*                                              DEFINES
#********************************************************************************************************

    .equ VRSAVE,     256
    .equ SPEFSCR,    512


#********************************************************************************************************
#*                                   CONTEXT STACK FRAME DEFINITION
#*
#* Description: This stack frame is used to save and restore context.
#********************************************************************************************************
                                                                ### TOP OF STACK FRAME ###
    .equ XR1,        0x0                                        # Backchain
    .equ XLR,        0x4                                        # Link Register

    .equ XMSR,       0x8
    .equ XSRR0,      0xC
    .equ XSRR1,     0x10
    .equ XCTR,      0x14
    .equ XXER,      0x18
    .equ XSPEFSCR,  0x1C

    .equ XCR,       0x20                                        # Condition Register

                                                                ### GENERAL-PURPOSE REGISTERS ###
    .equ XR0,       0x24
    .equ XR2,       0x28
    .equ XR3,       0x2C
    .equ XR4,       0x30
    .equ XR5,       0x34
    .equ XR6,       0x38
    .equ XR7,       0x3C
    .equ XR8,       0x40
    .equ XR9,       0x44
    .equ XR10,      0x48
    .equ XR11,      0x4C
    .equ XR12,      0x50
    .equ XR13,      0x54
    .equ XR14,      0x58
    .equ XR15,      0x5C
    .equ XR16,      0x60
    .equ XR17,      0x64
    .equ XR18,      0x68
    .equ XR19,      0x6C
    .equ XR20,      0x70
    .equ XR21,      0x74
    .equ XR22,      0x78
    .equ XR23,      0x7C
    .equ XR24,      0x80
    .equ XR25,      0x84
    .equ XR26,      0x88
    .equ XR27,      0x8C
    .equ XR28,      0x90
    .equ XR29,      0x94
    .equ XR30,      0x98
    .equ XR31,      0x9C
                                                                ### BOTTOM OF STACK FRAME ###

    .equ STACK_FRAME_SIZE, 0xA0


#********************************************************************************************************
#*                                         PROLOGUE DEFINITION
#********************************************************************************************************
prologue:       .macro
        mtspr   VRSAVE,r11
        e_li    r11,-STACK_FRAME_SIZE
        stwux   r1,r1,r11
        mfspr   r11,VRSAVE

        e_stw   r0,XR0(r1)
        e_stw   r2,XR2(r1)
        e_stw   r3,XR3(r1)
        e_stw   r4,XR4(r1)
        e_stw   r5,XR5(r1)
        e_stw   r6,XR6(r1)
        e_stw   r7,XR7(r1)
        e_stw   r8,XR8(r1)
        e_stw   r9,XR9(r1)
        e_stw   r10,XR10(r1)
        e_stw   r11,XR11(r1)
        e_stw   r12,XR12(r1)
        e_stw   r13,XR13(r1)
        e_stw   r14,XR14(r1)
        e_stw   r15,XR15(r1)
        e_stw   r16,XR16(r1)
        e_stw   r17,XR17(r1)
        e_stw   r18,XR18(r1)
        e_stw   r19,XR19(r1)
        e_stw   r20,XR20(r1)
        e_stw   r21,XR21(r1)
        e_stw   r22,XR22(r1)
        e_stw   r23,XR23(r1)
        e_stw   r24,XR24(r1)
        e_stw   r25,XR25(r1)
        e_stw   r26,XR26(r1)
        e_stw   r27,XR27(r1)
        e_stw   r28,XR28(r1)
        e_stw   r29,XR29(r1)
        e_stw   r30,XR30(r1)
        e_stw   r31,XR31(r1)

        mfmsr   r0
        e_stw   r0,XMSR(r1)

        mfspr   r0,SPEFSCR
        e_stw   r0,XSPEFSCR(r1)

        mfspr   r0,SRR0
        e_stw   r0,XSRR0(r1)

        mfspr   r0,SRR1
        e_stw   r0,XSRR1(r1)

        mflr    r0
        e_stw   r0,XLR(r1)

        mfctr   r0
        e_stw   r0,XCTR(r1)

        mfxer   r0
        e_stw   r0,XXER(r1)

        mfcr    r0
        e_stw   r0,XCR(r1)

        mfmsr   r0
    .endm


#********************************************************************************************************
#*                                         EPILOGUE DEFINITION
#********************************************************************************************************
epilogue:       .macro
        e_lwz   r2,XR2(r1)
        e_lwz   r3,XR3(r1)
        e_lwz   r4,XR4(r1)
        e_lwz   r5,XR5(r1)
        e_lwz   r6,XR6(r1)
        e_lwz   r7,XR7(r1)
        e_lwz   r8,XR8(r1)
        e_lwz   r9,XR9(r1)
        e_lwz   r10,XR10(r1)
        e_lwz   r11,XR11(r1)
        e_lwz   r12,XR12(r1)
        e_lwz   r13,XR13(r1)
        e_lwz   r14,XR14(r1)
        e_lwz   r15,XR15(r1)
        e_lwz   r16,XR16(r1)
        e_lwz   r17,XR17(r1)
        e_lwz   r18,XR18(r1)
        e_lwz   r19,XR19(r1)
        e_lwz   r20,XR20(r1)
        e_lwz   r21,XR21(r1)
        e_lwz   r22,XR22(r1)
        e_lwz   r23,XR23(r1)
        e_lwz   r24,XR24(r1)
        e_lwz   r25,XR25(r1)
        e_lwz   r26,XR26(r1)
        e_lwz   r27,XR27(r1)
        e_lwz   r28,XR28(r1)
        e_lwz   r29,XR29(r1)
        e_lwz   r30,XR30(r1)
        e_lwz   r31,XR31(r1)

        e_lwz   r0,XCR(r1)
        mtcrf   0xff,r0
        e_lwz   r0,XXER(r1)
        mtxer   r0
        e_lwz   r0,XCTR(r1)
        mtctr   r0
        e_lwz   r0,XLR(r1)
        mtlr    r0

        e_lwz   r0,XSRR1(r1)
        mtspr   SRR1,r0
        e_lwz   r0,XSRR0(r1)
        mtspr   SRR0,r0
        e_lwz   r0,XMSR(r1)
        mtmsr   r0

        e_lwz   r0,XSPEFSCR(r1)
        mtspr   SPEFSCR,r0

        e_lwz   r0,XR0(r1)

        e_lwz   r1,XR1(r1)
    .endm
