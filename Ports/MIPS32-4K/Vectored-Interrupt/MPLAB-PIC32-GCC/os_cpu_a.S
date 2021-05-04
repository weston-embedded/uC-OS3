/*
*********************************************************************************************************
*                                              uC/OS-III
*                                        The Real-Time Kernel
*
*                    Copyright 2009-2021 Silicon Laboratories Inc. www.silabs.com
*
*                                 SPDX-License-Identifier: APACHE-2.0
*
*               This software is subject to an open source license and is distributed by
*                Silicon Laboratories Inc. pursuant to the terms of the Apache License,
*                    Version 2.0 available at www.apache.org/licenses/LICENSE-2.0.
*
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                               MIPS32 4K
*
*                                                 MPLAB
*
* File    : os_cpu_a.S
* Version : V3.08.01
*********************************************************************************************************
*/

    .section .text,code
    .set noreorder
    .set nomips16
    .set noat


/*
*********************************************************************************************************
*                                          PUBLIC FUNCTIONS
*********************************************************************************************************
*/

    .global  OSStartHighRdy
    .global  OSIntCtxSw
    .global  OSCtxSw
    .global  CoreTimerIntHandler


/*
*********************************************************************************************************
*                           CONSTANTS USED TO ACCESS TASK CONTEXT STACK
*********************************************************************************************************
*/

.equ    STK_OFFSET_SR,      4
.equ    STK_OFFSET_EPC,     STK_OFFSET_SR    + 4
.equ    STK_OFFSET_LO,      STK_OFFSET_EPC   + 4
.equ    STK_OFFSET_HI,      STK_OFFSET_LO    + 4
.equ    STK_OFFSET_GPR1,    STK_OFFSET_HI    + 4
.equ    STK_OFFSET_GPR2,    STK_OFFSET_GPR1  + 4
.equ    STK_OFFSET_GPR3,    STK_OFFSET_GPR2  + 4
.equ    STK_OFFSET_GPR4,    STK_OFFSET_GPR3  + 4
.equ    STK_OFFSET_GPR5,    STK_OFFSET_GPR4  + 4
.equ    STK_OFFSET_GPR6,    STK_OFFSET_GPR5  + 4
.equ    STK_OFFSET_GPR7,    STK_OFFSET_GPR6  + 4
.equ    STK_OFFSET_GPR8,    STK_OFFSET_GPR7  + 4
.equ    STK_OFFSET_GPR9,    STK_OFFSET_GPR8  + 4
.equ    STK_OFFSET_GPR10,   STK_OFFSET_GPR9  + 4
.equ    STK_OFFSET_GPR11,   STK_OFFSET_GPR10 + 4
.equ    STK_OFFSET_GPR12,   STK_OFFSET_GPR11 + 4
.equ    STK_OFFSET_GPR13,   STK_OFFSET_GPR12 + 4
.equ    STK_OFFSET_GPR14,   STK_OFFSET_GPR13 + 4
.equ    STK_OFFSET_GPR15,   STK_OFFSET_GPR14 + 4
.equ    STK_OFFSET_GPR16,   STK_OFFSET_GPR15 + 4
.equ    STK_OFFSET_GPR17,   STK_OFFSET_GPR16 + 4
.equ    STK_OFFSET_GPR18,   STK_OFFSET_GPR17 + 4
.equ    STK_OFFSET_GPR19,   STK_OFFSET_GPR18 + 4
.equ    STK_OFFSET_GPR20,   STK_OFFSET_GPR19 + 4
.equ    STK_OFFSET_GPR21,   STK_OFFSET_GPR20 + 4
.equ    STK_OFFSET_GPR22,   STK_OFFSET_GPR21 + 4
.equ    STK_OFFSET_GPR23,   STK_OFFSET_GPR22 + 4
.equ    STK_OFFSET_GPR24,   STK_OFFSET_GPR23 + 4
.equ    STK_OFFSET_GPR25,   STK_OFFSET_GPR24 + 4
.equ    STK_OFFSET_GPR26,   STK_OFFSET_GPR25 + 4
.equ    STK_OFFSET_GPR27,   STK_OFFSET_GPR26 + 4
.equ    STK_OFFSET_GPR28,   STK_OFFSET_GPR27 + 4
.equ    STK_OFFSET_GPR30,   STK_OFFSET_GPR28 + 4
.equ    STK_OFFSET_GPR31,   STK_OFFSET_GPR30 + 4
.equ    STK_CTX_SIZE,       STK_OFFSET_GPR31 + 4


/*
*********************************************************************************************************
*                                           OSStartHighRdy()
*
* Description: Starts the highest priority task that is available to run.  OSStartHighRdy() MUST:
*
*              a) Call OSTaskSwHook()
*              b) Set OSRunning to TRUE
*              c) Switch to the highest priority task.
*
*              The stack frame of the task to resume is assumed to look as follows:
*
*              OSTCBHighRdy->OSTCBStkPtr + 0x00    Free Entry                    (LOW Memory)
*                                        + 0x04    Status Register
*                                        + 0x08    EPC
*                                        + 0x0C    Special Purpose LO Register
*                                        + 0x10    Special Purpose HI Register
*                                        + 0x14    GPR[1]
*                                        + 0x18    GPR[2]
*                                        + 0x1C    GPR[3]
*                                        + 0x20    GPR[4]
*                                               |
*                                               |
*                                              \ /
*                                               V
*                                        + 0x80    GPR[28]
*                                        + 0x84    GPR[30]
*                                        + 0x88    GPR[31]                       (HIGH Memory)
*
* Note(s): 1) OSTaskStkInit(), which is responsible for initializing each task's stack, sets bit 0 of the
*             entry corresponding to the Status register.  Thus, interrupts will be enabled when each
*             task first runs.
*********************************************************************************************************
*/

    .ent OSStartHighRdy
OSStartHighRdy:

    la    $8,  OSTaskSwHook                    /* Call OSTaskSwHook()                                  */
    jalr  $8
    nop

    la    $8,  OSTCBHighRdyPtr                 /* Update the current TCB                               */
    lw    $9,  0($8)
    lw    $29, 0($9)                           /* Load the new task's stack pointer                    */

    lw    $8,  STK_OFFSET_SR($29)              /* Restore the Status register                          */
    mtc0  $8,  $12, 0
    ehb

    lw    $8,  STK_OFFSET_EPC($29)             /* Restore the EPC                                      */
    mtc0  $8,  $14, 0
    ehb

    lw    $8,  STK_OFFSET_LO($29)              /* Restore the contents of the LO and HI registers      */
    lw    $9,  STK_OFFSET_HI($29)
    mtlo  $8
    mthi  $9

    lw    $31, STK_OFFSET_GPR31($29)           /* Restore the General Purpose Registers                */
    lw    $30, STK_OFFSET_GPR30($29)
    lw    $28, STK_OFFSET_GPR28($29)
    lw    $27, STK_OFFSET_GPR27($29)
    lw    $26, STK_OFFSET_GPR26($29)
    lw    $25, STK_OFFSET_GPR25($29)
    lw    $24, STK_OFFSET_GPR24($29)
    lw    $23, STK_OFFSET_GPR23($29)
    lw    $22, STK_OFFSET_GPR22($29)
    lw    $21, STK_OFFSET_GPR21($29)
    lw    $20, STK_OFFSET_GPR20($29)
    lw    $19, STK_OFFSET_GPR19($29)
    lw    $18, STK_OFFSET_GPR18($29)
    lw    $17, STK_OFFSET_GPR17($29)
    lw    $16, STK_OFFSET_GPR16($29)
    lw    $15, STK_OFFSET_GPR15($29)
    lw    $14, STK_OFFSET_GPR14($29)
    lw    $13, STK_OFFSET_GPR13($29)
    lw    $12, STK_OFFSET_GPR12($29)
    lw    $11, STK_OFFSET_GPR11($29)
    lw    $10, STK_OFFSET_GPR10($29)
    lw    $9,  STK_OFFSET_GPR9($29)
    lw    $8,  STK_OFFSET_GPR8($29)
    lw    $7,  STK_OFFSET_GPR7($29)
    lw    $6,  STK_OFFSET_GPR6($29)
    lw    $5,  STK_OFFSET_GPR5($29)
    lw    $4,  STK_OFFSET_GPR4($29)
    lw    $3,  STK_OFFSET_GPR3($29)
    lw    $2,  STK_OFFSET_GPR2($29)
    lw    $1,  STK_OFFSET_GPR1($29)

    addi  $29, $29, STK_CTX_SIZE               /* Adjust the stack pointer                             */
    eret

    .end OSStartHighRdy


/*
*********************************************************************************************************
*                                             OSIntCtxSw()
*
* Description: This function is used to perform a context switch following an ISR.
*
*              OSIntCtxSw() implements the following pseudo-code:
*
*                  OSTaskSwHook();
*                  OSPrioCur = OSPrioHighRdy;
*                  OSTCBCur  = OSTCBHighRdy;
*                  SP        = OSTCBHighRdy->OSTCBStkPtr;
*                  Restore the Status register and the EPC to their prior states;
*                  Restore the LO and HI registers;
*                  Restore each of the general purpose registers;
*                  Adjust the stack pointer;
*                  Execute an eret instruction to begin executing the new task;
*
*              Upon entry, the registers of the task being suspended have already been saved onto that
*              task's stack and the SP for the task has been saved in its OS_TCB by the ISR.
*
*              The stack frame of the task to resume is assumed to look as follows:
*
*              OSTCBHighRdy->OSTCBStkPtr + 0x00    Free Entry                    (LOW Memory)
*                                        + 0x04    Status Register
*                                        + 0x08    EPC
*                                        + 0x0C    Special Purpose LO Register
*                                        + 0x10    Special Purpose HI Register
*                                        + 0x14    GPR[1]
*                                        + 0x18    GPR[2]
*                                        + 0x1C    GPR[3]
*                                        + 0x20    GPR[4]
*                                               |
*                                               |
*                                              \ /
*                                               V
*                                        + 0x80    GPR[28]
*                                        + 0x84    GPR[30]
*                                        + 0x88    GPR[31]                       (HIGH Memory)
*********************************************************************************************************
*/

    .ent OSIntCtxSw
OSIntCtxSw:

    la    $8,  OSTaskSwHook                    /* Call OSTaskSwHook()                                  */
    jalr  $8
    nop

    la    $8,  OSPrioHighRdy                   /* Update the current priority                          */
    lbu   $9,  0($8)
    la    $10, OSPrioCur
    sb    $9,  0($10)

    la    $8,  OSTCBHighRdyPtr                 /* Update the current TCB                               */
    lw    $9,  0($8)
    la    $10, OSTCBCurPtr
    sw    $9,  0($10)

    lw    $29, 0($9)                           /* Load the new task's stack pointer                    */

    lw    $8,  STK_OFFSET_SR($29)              /* Restore the Status register                          */
    mtc0  $8,  $12, 0
    ehb

    lw    $8,  STK_OFFSET_EPC($29)             /* Restore the EPC                                      */
    mtc0  $8,  $14, 0
    ehb

    lw    $8,  STK_OFFSET_LO($29)              /* Restore the contents of the LO and HI registers      */
    lw    $9,  STK_OFFSET_HI($29)
    mtlo  $8
    mthi  $9

    lw    $31, STK_OFFSET_GPR31($29)           /* Restore the General Purpose Registers                */
    lw    $30, STK_OFFSET_GPR30($29)
    lw    $28, STK_OFFSET_GPR28($29)
    lw    $27, STK_OFFSET_GPR27($29)
    lw    $26, STK_OFFSET_GPR26($29)
    lw    $25, STK_OFFSET_GPR25($29)
    lw    $24, STK_OFFSET_GPR24($29)
    lw    $23, STK_OFFSET_GPR23($29)
    lw    $22, STK_OFFSET_GPR22($29)
    lw    $21, STK_OFFSET_GPR21($29)
    lw    $20, STK_OFFSET_GPR20($29)
    lw    $19, STK_OFFSET_GPR19($29)
    lw    $18, STK_OFFSET_GPR18($29)
    lw    $17, STK_OFFSET_GPR17($29)
    lw    $16, STK_OFFSET_GPR16($29)
    lw    $15, STK_OFFSET_GPR15($29)
    lw    $14, STK_OFFSET_GPR14($29)
    lw    $13, STK_OFFSET_GPR13($29)
    lw    $12, STK_OFFSET_GPR12($29)
    lw    $11, STK_OFFSET_GPR11($29)
    lw    $10, STK_OFFSET_GPR10($29)
    lw    $9,  STK_OFFSET_GPR9($29)
    lw    $8,  STK_OFFSET_GPR8($29)
    lw    $7,  STK_OFFSET_GPR7($29)
    lw    $6,  STK_OFFSET_GPR6($29)
    lw    $5,  STK_OFFSET_GPR5($29)
    lw    $4,  STK_OFFSET_GPR4($29)
    lw    $3,  STK_OFFSET_GPR3($29)
    lw    $2,  STK_OFFSET_GPR2($29)
    lw    $1,  STK_OFFSET_GPR1($29)

    addi  $29, $29, STK_CTX_SIZE               /* Adjust the stack pointer                             */

    eret                                       /* Resume execution in new task                         */

    .end OSIntCtxSw


/*
*********************************************************************************************************
*                                               OSCtxSw
*
* Description: Performs a Context switch from a task.  This function is ALWAYS called with interrupts
*              DISABLED.
*
*              OSCtxSw() implements the following pseudo-code:
*
*                  Adjust the stack pointer;
*                  Save each of the general purpose registers;
*                  Save the LO and HI registers;
*                  Save the EPC and the Status register;
*                  Verify that a syscall exception (corresponding to OS_TASK_SW) occurred;
*                  OSTCBCur->OSTCBStkPtr = SP;
*                  OSTaskSwHook();
*                  OSPrioCur = OSPrioHighRdy;
*                  OSTCBCur  = OSTCBHighRdy;
*                  SP        = OSTCBHighRdy->OSTCBStkPtr;
*                  Restore the Status register and the EPC to their prior states;
*                  Restore the LO and HI registers;
*                  Restore each of the general purpose registers;
*                  Adjust the stack pointer;
*                  Execute an eret instruction to begin executing the new task;
*
*              The stack frame of the task to suspend will look as follows when OSCtxSw() is done:
*
*              OSTCBHighRdy->OSTCBStkPtr + 0x00    Free Entry                    (LOW Memory)
*                                        + 0x04    Status Register
*                                        + 0x08    EPC
*                                        + 0x0C    Special Purpose LO Register
*                                        + 0x10    Special Purpose HI Register
*                                        + 0x14    GPR[1]
*                                        + 0x18    GPR[2]
*                                        + 0x1C    GPR[3]
*                                        + 0x20    GPR[4]
*                                               |
*                                               |
*                                              \ /
*                                               V
*                                        + 0x80    GPR[28]
*                                        + 0x84    GPR[30]
*                                        + 0x88    GPR[31]                       (HIGH Memory)
*
*              The stack frame of the task to resume looks as follows:
*
*              OSTCBHighRdy->OSTCBStkPtr + 0x00    Free Entry                    (LOW Memory)
*                                        + 0x04    Status Register
*                                        + 0x08    EPC
*                                        + 0x0C    Special Purpose LO Register
*                                        + 0x10    Special Purpose HI Register
*                                        + 0x14    GPR[1]
*                                        + 0x18    GPR[2]
*                                        + 0x1C    GPR[3]
*                                        + 0x20    GPR[4]
*                                               |
*                                               |
*                                              \ /
*                                               V
*                                        + 0x80    GPR[28]
*                                        + 0x84    GPR[30]
*                                        + 0x88    GPR[31]                       (HIGH Memory)
*********************************************************************************************************
*/

    .ent OSCtxSw
OSCtxSw:

    addi  $29, $29, -STK_CTX_SIZE              /* Adjust the stack pointer                             */

    sw    $1,  STK_OFFSET_GPR1($29)            /* Save the General Pupose Registers                    */
    sw    $2,  STK_OFFSET_GPR2($29)
    sw    $3,  STK_OFFSET_GPR3($29)
    sw    $4,  STK_OFFSET_GPR4($29)
    sw    $5,  STK_OFFSET_GPR5($29)
    sw    $6,  STK_OFFSET_GPR6($29)
    sw    $7,  STK_OFFSET_GPR7($29)
    sw    $8,  STK_OFFSET_GPR8($29)
    sw    $9,  STK_OFFSET_GPR9($29)
    sw    $10, STK_OFFSET_GPR10($29)
    sw    $11, STK_OFFSET_GPR11($29)
    sw    $12, STK_OFFSET_GPR12($29)
    sw    $13, STK_OFFSET_GPR13($29)
    sw    $14, STK_OFFSET_GPR14($29)
    sw    $15, STK_OFFSET_GPR15($29)
    sw    $16, STK_OFFSET_GPR16($29)
    sw    $17, STK_OFFSET_GPR17($29)
    sw    $18, STK_OFFSET_GPR18($29)
    sw    $19, STK_OFFSET_GPR19($29)
    sw    $20, STK_OFFSET_GPR20($29)
    sw    $21, STK_OFFSET_GPR21($29)
    sw    $22, STK_OFFSET_GPR22($29)
    sw    $23, STK_OFFSET_GPR23($29)
    sw    $24, STK_OFFSET_GPR24($29)
    sw    $25, STK_OFFSET_GPR25($29)
    sw    $26, STK_OFFSET_GPR26($29)
    sw    $27, STK_OFFSET_GPR27($29)
    sw    $28, STK_OFFSET_GPR28($29)
    sw    $30, STK_OFFSET_GPR30($29)
    sw    $31, STK_OFFSET_GPR31($29)
                                               /* Save the contents of the LO and HI registers         */
    mflo  $8
    mfhi  $9
    sw    $8,  STK_OFFSET_LO($29)
    sw    $9,  STK_OFFSET_HI($29)

    mfc0  $8,  $12, 0                          /* Save the Status register                             */
    sw    $8,  STK_OFFSET_SR($29)

    ori   $8,  $0,  0x007C                     /* Switch context only if a syscall exception occurred  */
    mfc0  $9,  $13, 0
    and   $9,  $9,  $8
    ori   $10, $0,  0x0020
    beq   $9,  $10, SAVE_SP
    nop

    la    $8,  BSP_Except_Handler             /* Call BSP_Except_Handler() for exceptions not...       */
    jalr  $8                                  /* ...triggered by syscall.                              */
    nop

    b     RESTORE_CTX
    nop

SAVE_SP:
    mfc0  $8,  $14, 0                          /* Save the EPC                                         */
    addi  $8,  $8,  4                          /* EPC will reference the instruction following syscall */
    sw    $8,  STK_OFFSET_EPC($29)

    la    $8,  OSTCBCurPtr                     /* Save the current task's stack pointer                */
    lw    $9,  0($8)
    sw    $29, 0($9)

    la    $8,  OSTaskSwHook                    /* Call OSTaskSwHook()                                  */
    jalr  $8
    nop

    la    $8,  OSPrioHighRdy                   /* Update the current priority                          */
    lbu   $9,  0($8)
    la    $10, OSPrioCur
    sb    $9,  0($10)

    la    $8,  OSTCBHighRdyPtr                 /* Update the current TCB                               */
    lw    $9,  0($8)
    la    $10, OSTCBCurPtr
    sw    $9,  0($10)

    lw    $29, 0($9)                           /* Load the new task's stack pointer                    */

RESTORE_CTX:

    lw    $8,  STK_OFFSET_SR($29)              /* Restore the Status register                          */
    mtc0  $8,  $12, 0
    ehb

    lw    $8,  STK_OFFSET_EPC($29)             /* Restore the EPC                                      */
    mtc0  $8,  $14, 0
    ehb

    lw    $8,  STK_OFFSET_LO($29)              /* Restore the contents of the LO and HI registers      */
    lw    $9,  STK_OFFSET_HI($29)
    mtlo  $8
    mthi  $9

    lw    $31, STK_OFFSET_GPR31($29)           /* Restore the General Purpose Registers                */
    lw    $30, STK_OFFSET_GPR30($29)
    lw    $28, STK_OFFSET_GPR28($29)
    lw    $27, STK_OFFSET_GPR27($29)
    lw    $26, STK_OFFSET_GPR26($29)
    lw    $25, STK_OFFSET_GPR25($29)
    lw    $24, STK_OFFSET_GPR24($29)
    lw    $23, STK_OFFSET_GPR23($29)
    lw    $22, STK_OFFSET_GPR22($29)
    lw    $21, STK_OFFSET_GPR21($29)
    lw    $20, STK_OFFSET_GPR20($29)
    lw    $19, STK_OFFSET_GPR19($29)
    lw    $18, STK_OFFSET_GPR18($29)
    lw    $17, STK_OFFSET_GPR17($29)
    lw    $16, STK_OFFSET_GPR16($29)
    lw    $15, STK_OFFSET_GPR15($29)
    lw    $14, STK_OFFSET_GPR14($29)
    lw    $13, STK_OFFSET_GPR13($29)
    lw    $12, STK_OFFSET_GPR12($29)
    lw    $11, STK_OFFSET_GPR11($29)
    lw    $10, STK_OFFSET_GPR10($29)
    lw    $9,  STK_OFFSET_GPR9($29)
    lw    $8,  STK_OFFSET_GPR8($29)
    lw    $7,  STK_OFFSET_GPR7($29)
    lw    $6,  STK_OFFSET_GPR6($29)
    lw    $5,  STK_OFFSET_GPR5($29)
    lw    $4,  STK_OFFSET_GPR4($29)
    lw    $3,  STK_OFFSET_GPR3($29)
    lw    $2,  STK_OFFSET_GPR2($29)
    lw    $1,  STK_OFFSET_GPR1($29)

    addi  $29, $29, STK_CTX_SIZE               /* Adjust the stack pointer                             */

    eret                                       /* Resume execution in new task                         */

    .end OSCtxSw


/*
*********************************************************************************************************
*                                          CoreTimerIntHandler
*
* Description: The core timer, which is implemented by the Coprocessor 0 Count and Compare registers,
*              is the source of uC/OS-III's tick interrupts.  This function handles those interrupts,
*              saving the current task's context and then calling OSTimeTick(), uC/OS-III's CPU-
*              independent routine for processing tick interrupts.
*
*              The interrupted task's context is saved onto its stack as follows:
*
*
*              OSTCBHighRdy->OSTCBStkPtr + 0x00    Free Entry                    (LOW Memory)
*                                        + 0x04    Status Register
*                                        + 0x08    EPC
*                                        + 0x0C    Special Purpose LO Register
*                                        + 0x10    Special Purpose HI Register
*                                        + 0x14    GPR[1]
*                                        + 0x18    GPR[2]
*                                        + 0x1C    GPR[3]
*                                        + 0x20    GPR[4]
*                                               |
*                                               |
*                                              \ /
*                                               V
*                                        + 0x80    GPR[28]
*                                        + 0x84    GPR[30]
*                                        + 0x88    GPR[31]                       (HIGH Memory)
*********************************************************************************************************
*/

    .ent CoreTimerIntHandler
CoreTimerIntHandler:

    addi  $29, $29, -STK_CTX_SIZE              /* Adjust the stack pointer                             */

    sw    $1,  STK_OFFSET_GPR1($29)            /* Save the General Pupose Registers                    */
    sw    $2,  STK_OFFSET_GPR2($29)
    sw    $3,  STK_OFFSET_GPR3($29)
    sw    $4,  STK_OFFSET_GPR4($29)
    sw    $5,  STK_OFFSET_GPR5($29)
    sw    $6,  STK_OFFSET_GPR6($29)
    sw    $7,  STK_OFFSET_GPR7($29)
    sw    $8,  STK_OFFSET_GPR8($29)
    sw    $9,  STK_OFFSET_GPR9($29)
    sw    $10, STK_OFFSET_GPR10($29)
    sw    $11, STK_OFFSET_GPR11($29)
    sw    $12, STK_OFFSET_GPR12($29)
    sw    $13, STK_OFFSET_GPR13($29)
    sw    $14, STK_OFFSET_GPR14($29)
    sw    $15, STK_OFFSET_GPR15($29)
    sw    $16, STK_OFFSET_GPR16($29)
    sw    $17, STK_OFFSET_GPR17($29)
    sw    $18, STK_OFFSET_GPR18($29)
    sw    $19, STK_OFFSET_GPR19($29)
    sw    $20, STK_OFFSET_GPR20($29)
    sw    $21, STK_OFFSET_GPR21($29)
    sw    $22, STK_OFFSET_GPR22($29)
    sw    $23, STK_OFFSET_GPR23($29)
    sw    $24, STK_OFFSET_GPR24($29)
    sw    $25, STK_OFFSET_GPR25($29)
    sw    $26, STK_OFFSET_GPR26($29)
    sw    $27, STK_OFFSET_GPR27($29)
    sw    $28, STK_OFFSET_GPR28($29)
    sw    $30, STK_OFFSET_GPR30($29)
    sw    $31, STK_OFFSET_GPR31($29)
                                               /* Save the contents of the LO and HI registers         */
    mflo  $8
    mfhi  $9
    sw    $8,  STK_OFFSET_LO($29)
    sw    $9,  STK_OFFSET_HI($29)

    mfc0  $8,  $14, 0                          /* Save the EPC                                         */
    sw    $8,  STK_OFFSET_EPC($29)

    mfc0  $8,  $12, 0
    sw    $8,  STK_OFFSET_SR($29)

    la    $8,  OSIntNestingCtr                 /* See if OSIntNesting == 0                             */
    lbu   $9,  0($8)
    bne   $0,  $9, TICK_INC_NESTING
    nop

    la    $10, OSTCBCurPtr                     /* Save the current task's stack pointer                */
    lw    $11, 0($10)
    sw    $29, 0($11)

TICK_INC_NESTING:

    addi  $9,  $9, 1                           /* Increment OSIntNesting                               */
    sb    $9,  0($8)

    mfc0  $8, $12, 0                           /* Update the Status register's IPL bits                */
    mfc0  $9, $13, 0
    andi  $9, $9,  0xFC00
    ins   $8, $0,  10, 6
    or    $8, $8,  $9
    mtc0  $8, $12, 0
    ehb

    la    $8,  BSP_TickISR_Handler             /* Call BSP_TickISR_Handler() to handle the interrupt   */
    jalr  $8
    nop

    la    $8,  OSTimeTick                      /* Call OSTimeTick()                                    */
    jalr  $8
    nop

    la    $8,  OSIntExit                       /* Call OSIntExit()                                     */
    jalr  $8
    nop

    lw    $8,  STK_OFFSET_SR($29)              /* Restore the Status register                          */
    mtc0  $8,  $12, 0
    ehb

    lw    $8,  STK_OFFSET_EPC($29)             /* Restore the EPC                                      */
    mtc0  $8,  $14, 0
    ehb

    lw    $8,  STK_OFFSET_LO($29)              /* Restore the contents of the LO and HI registers      */
    lw    $9,  STK_OFFSET_HI($29)
    mtlo  $8
    mthi  $9

    lw    $31, STK_OFFSET_GPR31($29)           /* Restore the General Purpose Registers                */
    lw    $30, STK_OFFSET_GPR30($29)
    lw    $28, STK_OFFSET_GPR28($29)
    lw    $27, STK_OFFSET_GPR27($29)
    lw    $26, STK_OFFSET_GPR26($29)
    lw    $25, STK_OFFSET_GPR25($29)
    lw    $24, STK_OFFSET_GPR24($29)
    lw    $23, STK_OFFSET_GPR23($29)
    lw    $22, STK_OFFSET_GPR22($29)
    lw    $21, STK_OFFSET_GPR21($29)
    lw    $20, STK_OFFSET_GPR20($29)
    lw    $19, STK_OFFSET_GPR19($29)
    lw    $18, STK_OFFSET_GPR18($29)
    lw    $17, STK_OFFSET_GPR17($29)
    lw    $16, STK_OFFSET_GPR16($29)
    lw    $15, STK_OFFSET_GPR15($29)
    lw    $14, STK_OFFSET_GPR14($29)
    lw    $13, STK_OFFSET_GPR13($29)
    lw    $12, STK_OFFSET_GPR12($29)
    lw    $11, STK_OFFSET_GPR11($29)
    lw    $10, STK_OFFSET_GPR10($29)
    lw    $9,  STK_OFFSET_GPR9($29)
    lw    $8,  STK_OFFSET_GPR8($29)
    lw    $7,  STK_OFFSET_GPR7($29)
    lw    $6,  STK_OFFSET_GPR6($29)
    lw    $5,  STK_OFFSET_GPR5($29)
    lw    $4,  STK_OFFSET_GPR4($29)
    lw    $3,  STK_OFFSET_GPR3($29)
    lw    $2,  STK_OFFSET_GPR2($29)
    lw    $1,  STK_OFFSET_GPR1($29)

    addi  $29, $29, STK_CTX_SIZE               /* Adjust the stack pointer                             */

    eret

    .end CoreTimerIntHandler
