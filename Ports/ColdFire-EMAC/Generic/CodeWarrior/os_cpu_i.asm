/*
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
*/

/*
;********************************************************************************************************
;
;                       Generic Coldfire with EMAC Port for CodeWarrior Compiler
;
; File    : os_cpu_i.asm
; Version : V3.08.01
;********************************************************************************************************
*/

/*
;**************************************************************************************************
;                                             MACROS
;**************************************************************************************************
*/

       .macro    OS_EMAC_SAVE
                                      /* CODE BELOW TO SAVE EMAC REGISTERS        */
       MOVE.L    MACSR,D7             /* Save the MACSR                           */
       CLR.L     D0                   /* Disable rounding in the MACSR            */
       MOVE.L    D0,MACSR             /* Save the accumulators                    */
       MOVE.L    ACC0,D0
       MOVE.L    ACC1,D1
       MOVE.L    ACC2,D2
       MOVE.L    ACC3,D3
       MOVE.L    ACCEXT01,D4          /* Save the accumulator extensions          */
       MOVE.L    ACCEXT23,D5
       MOVE.L    MASK,D6              /* Save the address mask                    */
       LEA       -32(A7),A7           /* Move the EMAC state to the task's stack  */
       MOVEM.L   D0-D7,(A7)

      .endm



       .macro    OS_EMAC_RESTORE
                                      /* CODE BELOW TO RESTORE EMAC REGISTERS     */
       MOVEM.L    (A7),D0-D7          /* Restore the EMAC state                   */
       MOVE.L     #0,MACSR            /* Disable rounding in the MACSR            */
       MOVE.L     D0,ACC0             /* Restore the accumulators                 */
       MOVE.L     D1,ACC1
       MOVE.L     D2,ACC2
       MOVE.L     D3,ACC3
       MOVE.L     D4,ACCEXT01
       MOVE.L     D5,ACCEXT23
       MOVE.L     D6,MASK
       MOVE.L     D7,MACSR
       LEA        32(A7),A7

      .endm
