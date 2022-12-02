/*
*********************************************************************************************************
*                                              uC/OS-III
*                                        The Real-Time Kernel
*
*                    Copyright 2009-2022 Silicon Laboratories Inc. www.silabs.com
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
*                            uCOS-III port for Analog Device's Blackfin 533
*
*                                           Visual DSP++ 5.0
*
*                  This port was made with a large contribution of Analog Devices Inc
*                                           development team
*
* File    : os_cpu_a.asm
* Version : V3.08.02
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include <os_cfg.h>
#include <os_cpu.h>


/*
*********************************************************************************************************
*                                            LOCAL MACROS
*********************************************************************************************************
*/

#define  UPPER_( x )   (((x) >> 16) & 0x0000FFFF)
#define  LOWER_( x )   ((x) & 0x0000FFFF)
#define  LOAD(x, y)    x##.h = UPPER_(y); x##.l = LOWER_(y)
#define  LOADA(x, y)   x##.h = y; x##.l = y

/*
*********************************************************************************************************
*                                          PUBLIC FUNCTIONS
*********************************************************************************************************
*/

.global  _OS_CPU_EnableIntEntry;
.global  _OS_CPU_DisableIntEntry;
.global  _OS_CPU_NESTING_ISR;
.global  _OS_CPU_NON_NESTING_ISR;
.global  _OS_CPU_ISR_Entry;
.global  _OS_CPU_ISR_Exit;
.global  _OSStartHighRdy;
.global  _OSCtxSw;
.global  _OSIntCtxSw;
.global  _OS_CPU_Invalid_Task_Return;


/*
*********************************************************************************************************
*                                   Blackfin C Run-Time stack/frame Macros
*********************************************************************************************************
*/

#define INIT_C_RUNTIME_STACK(frame_size)                                \
    LINK frame_size;                                                    \
    SP += -12;        /* make space for outgoing arguments     */
                      /* when calling C-functions              */

#define DEL_C_RUNTIME_STACK()                                           \
    UNLINK;

/*
*********************************************************************************************************
*                                   WORKAROUND for Anomaly 05-00-0283 Macro
*********************************************************************************************************
*/

#define WORKAROUND_05000283()                                           \
    CC = R0 == R0;   /* always true                            */       \
    P0.L = 0x14;     /* MMR space - CHIPID                     */       \
    P0.H = 0xffc0;                                                      \
    IF CC JUMP 4;                                                       \
    R0 =  [ P0 ];    /* bogus MMR read that is speculatively   */
                     /* read and killed - never executed       */

/*
*********************************************************************************************************
*                                         EXTERNAL FUNCTIONS
*********************************************************************************************************
*/

.extern  _OS_CPU_IntHandler;
.extern  _OSTaskSwHook;
.extern  _OSIntEnter;
.extern  _OSIntExit;

/*
*********************************************************************************************************
*                                         EXTERNAL VARIABLES
*********************************************************************************************************
*/

.extern  _OSIntNestingCtr;
.extern  _OSPrioCur;
.extern  _OSPrioHighRdy;
.extern  _OSRunning;
.extern  _OSTCBCurPtr;
.extern  _OSTCBHighRdyPtr;


.section program;

/*
*********************************************************************************************************
*                                         OS_CPU_EnableIntEntry();
*
* Description : Enables an interrupt entry. Interrupt vector to enable is represented by the argument
*               passed to this function.
*
*
*
* Arguments   : Interrupt vector number (0 to 15) passed into R0 register
*
* Returns     : None
*
* Note(s)     : None
*********************************************************************************************************
*/

_OS_CPU_EnableIntEntry:

    R1   = 1;
    R1 <<= R0;
    CLI    R0;
    R1   = R1 | R0;
    STI    R1;

_OS_CPU_EnableIntEntry.end:
    RTS;

/*
*********************************************************************************************************
*                                           OS_CPU_DisableIntEntry();
*
* Description : Disables an interrupt entry. Interrupt vector to disable is represented by the argument
*               passed to this function.
*
*
*
* Arguments   : Interrupt vector number (0 to 15) passed into R0 register
*
* Returns     : None
*
* Note(s)     : None
*********************************************************************************************************
*/

_OS_CPU_DisableIntEntry:

    R1   =  1;
    R1 <<=  R0;
    R1   = ~R1;
    CLI     R0;
    R1   =  R1 & R0;
    STI     R1;

_OS_CPU_DisableIntEntry.end:
    RTS;

/*
*********************************************************************************************************
*                                         NESTING INTERRUPTS HANDLER
*                                          OS_CPU_NESTING_ISR();
*
* Description : This routine is intented to the target of the interrupt processing functionality with
*               nesting  support.
*
*               It saves the current Task context (see _OS_CPU_ISR_Entry),
*               and calls the application interrupt handler OS_CPU_IntHandler (see os_cpu_c.c)
*
*
* Arguments   : None
*
* Returns     : None
*
* Note(s)     : None
*********************************************************************************************************
*/

_OS_CPU_NESTING_ISR:

    [ -- SP ] = R0;
    [ -- SP ] = P1;
    [ -- SP ] = RETS;
    R0        = NESTED;                                 /* To indicate that this ISR supports nesting  */

    CALL.X _OS_CPU_ISR_Entry;

    WORKAROUND_05000283()
    INIT_C_RUNTIME_STACK(0x0)

    CALL.X _OS_CPU_IntHandler;                          /* See os_cpu_c.c                              */
    SP     += -4;                                       /* Disable interrupts by this artificial pop   */
    RETI    = [ SP++ ];                                 /* of RETI register. Restore context need to   */
    CALL.X _OSIntExit;                                  /* be done while interrupts are disabled       */

    DEL_C_RUNTIME_STACK()
    JUMP.X _OS_CPU_ISR_Exit;

_OS_CPU_NESTING_ISR.end:
    NOP;

/*
*********************************************************************************************************
*                                         NON NESTING INTERRUPTS HANDLER
*                                          OS_CPU_NON_NESTING_ISR();
*
* Description : This routine is intented to the target of the interrupt processing functionality without
*               nesting  support.
*
*               It saves the current Task context (see _OS_CPU_ISR_Entry),
*               and calls the application interrupt handler OS_CPU_IntHandler (see os_cpu_c.c)
*
*
* Arguments   : None
*
* Returns     : None
*
* Note(s)     : None
*********************************************************************************************************
*/

_OS_CPU_NON_NESTING_ISR:

    [ -- SP ] = R0;
    [ -- SP ] = P1;
    [ -- SP ] = RETS;
    R0        = NOT_NESTED;                             /* This ISR doesn't support nesting            */

    CALL.X _OS_CPU_ISR_Entry;

    WORKAROUND_05000283()
    INIT_C_RUNTIME_STACK(0x0)

    CALL.X _OS_CPU_IntHandler;                          /* See os_cpu_c.c                              */
    CALL.X _OSIntExit;

    DEL_C_RUNTIME_STACK()
    JUMP.X _OS_CPU_ISR_Exit;

_OS_CPU_NON_NESTING_ISR.end:
    NOP;

/*
*********************************************************************************************************
*                                         OS_CPU_ISR_Entry()
*
* Description : This routine serves as the interrupt entry function for ISRs.
*
*
*               __OS_CPU_ISR_Entry consists of
*
*               1.'uCOS Interrupt Entry management' - incremements OSIntNesting and saves SP in the TCB if
*                  OSIntNesting == 1.
*               2.'CPU context save' - After OSIntNesting is incremented, it is safe to re-enable interrupts.
*                  Then the rest of the processor's context is saved.
*
*               Refer to VisualDSP++ C/C++ Compiler and Library Manual for Blackfin Processors
*                 and ADSP-BF53x/BF56x Blackfin� Processor Programming Reference Manual.
*
*               The convention for the task frame (after context save is complete) is as follows:
*               (stack represented from high to low memory as per convention)
*                                  (*** High memory ***) R0
*                                                        P1
*                                                        RETS       (function return address of thread)
*                                                        R1
*                                                        R2
*                                                        P0
*                                                        P2
*                                                        ASTAT
*                                                        RETI      (interrupt return address: $PC of thread)
*                                                        R7:3    (R7 is lower than R3)
*                                                        P5:3    (P5 is lower than P3)
*                                                        FP        (frame pointer)
*                                                        I3:0    (I3 is lower than I0)
*                                                        B3:0    (B3 is lower than B0)
*                                                        L3:0    (L3 is lower than L0)
*                                                        M3:0    (M3 is lower than M0)
*                                                        A0.x
*                                                        A0.w
*                                                        A1.x
*                                                        A1.w
*                                                        LC1:0    (LC1 is lower than LC0)
*                                                        LT1:0    (LT1 is lower than LT0)
*               OSTCBHighRdy--> OSTCBStkPtr --> (*** Low memory ***)LB1:0    (LB1 is lower than LB0)
*
* Arguments   : RO is set to NESTED  or NOT_NESTED constant.
*
* Returns     : None
*
* Note(s)     : None
*********************************************************************************************************
*/

_OS_CPU_ISR_Entry:
ucos_ii_interrupt_entry_mgmt:

    [ -- SP ] = R1;
    [ -- SP ] = R2;
    [ -- SP ] = P0;
    [ -- SP ] = P2;
    [ -- SP ] = ASTAT;

    LOADA(P0 , _OSRunning);
    R2        = B [ P0 ] ( Z );
    CC        = R2 == 1;                            /* Is OSRunning set                                          */
    IF ! CC JUMP ucos_ii_interrupt_entry_mgmt.end;
    LOADA(P0, _OSIntNestingCtr);
    R2        = B [ P0 ] ( Z );
    R1        = 255 ( X );
    CC        = R2 < R1;                            /* Nesting < 255                                             */
    IF ! CC JUMP ucos_ii_interrupt_entry_mgmt.end;
    R2        = R2.B (X);
    R2       += 1;                                  /* Increment OSIntNesting                                    */
    B [ P0 ]  = R2;
    CC        = R2 == 1;                            /* Save SP if OSIntNesting == 1                              */
    IF ! CC JUMP ucos_ii_interrupt_entry_mgmt.end;
    LOADA(P2, _OSTCBCurPtr);
    P0        = [ P2 ];
    R2        = SP;
    R1        = 144;
    R2        = R2 - R1;
    [ P0 ]    = R2;

ucos_ii_interrupt_entry_mgmt.end:
    NOP;

interrupt_cpu_save_context:

    CC        = R0 == NESTED;
    IF !CC JUMP non_reetrant_isr;

reetrant_isr:
    [ -- SP ] = RETI;                               /* If ISR is REENTRANT, then simply push RETI onto stack     */
                                                    /* IPEND[4] is currently set, globally disabling interrupts  */
                                                    /* IPEND[4] will be cleared when RETI is pushed onto stack   */
    JUMP save_remaining_context;

non_reetrant_isr:
    R1        = RETI;                               /* If ISR is NON-REENTRANT, then save RETI through R1        */
    [ -- SP ] = R1;                                 /* IPEND[4] is currently set, globally disabling interrupts  */
                                                    /* IPEND[4] will stay set  when RETI is saved through R1     */


save_remaining_context:
    [ -- SP ] = (R7:3, P5:3);
    [ -- SP ] = FP;
    [ -- SP ] = I0;
    [ -- SP ] = I1;
    [ -- SP ] = I2;
    [ -- SP ] = I3;
    [ -- SP ] = B0;
    [ -- SP ] = B1;
    [ -- SP ] = B2;
    [ -- SP ] = B3;
    [ -- SP ] = L0;
    [ -- SP ] = L1;
    [ -- SP ] = L2;
    [ -- SP ] = L3;
    [ -- SP ] = M0;
    [ -- SP ] = M1;
    [ -- SP ] = M2;
    [ -- SP ] = M3;
    R1.L      = A0.x;
    [ -- SP ] = R1;
    R1        = A0.w;
    [ -- SP ] = R1;
    R1.L      = A1.x;
    [ -- SP ] = R1;
    R1        = A1.w;
    [ -- SP ] = R1;
    [ -- SP ] = LC0;
    R3        = 0;
    LC0       = R3;
    [ -- SP ] = LC1;
    R3        = 0;
    LC1       = R3;
    [ -- SP ] = LT0;
    [ -- SP ] = LT1;
    [ -- SP ] = LB0;
    [ -- SP ] = LB1;
    L0        = 0 ( X );
    L1        = 0 ( X );
    L2        = 0 ( X );
    L3        = 0 ( X );

interrupt_cpu_save_context.end:
_OS_CPU_ISR_Entry.end:
    RTS;


/*
*********************************************************************************************************
*                                              OS_CPU_ISR_Exit ()
*
*
* Description : ThIS routine serves as the interrupt exit function for all ISRs (whether reentrant
*               (nested) or non-reentrant (non-nested)). RETI is populated by a stack pop for both nested
*               as well non-nested interrupts.
*
*               This is a straigtforward implementation restoring the processor's context as per the
*               following task stack frame convention, returns from the interrupt with the RTI instruction.
*
*               Refer to VisualDSP++ C/C++ Compiler and Library Manual for Blackfin Processors
*               and ADSP-BF53x/BF56x Blackfin� Processor Programming Reference Manual.
*
*               The convention for the task frame (after context save is complete) is as follows:
*               (stack represented from high to low memory as per convention)
*                                  (*** High memory ***) R0
*                                                        P1
*                                                        RETS       (function return address of thread)
*                                                        R1
*                                                        R2
*                                                        P0
*                                                        P2
*                                                        ASTAT
*                                                        RETI      (interrupt return address: $PC of thread)
*                                                        R7:3    (R7 is lower than R3)
*                                                        P5:3    (P5 is lower than P3)
*                                                        FP        (frame pointer)
*                                                        I3:0    (I3 is lower than I0)
*                                                        B3:0    (B3 is lower than B0)
*                                                        L3:0    (L3 is lower than L0)
*                                                        M3:0    (M3 is lower than M0)
*                                                        A0.x
*                                                        A0.w
*                                                        A1.x
*                                                        A1.w
*                                                        LC1:0    (LC1 is lower than LC0)
*                                                        LT1:0    (LT1 is lower than LT0)
*               OSTCBHighRdy--> OSTCBStkPtr --> (*** Low memory ***)LB1:0    (LB1 is lower than LB0)
*
* Arguments   : None
*
* Returns     : None
*
* Note(s)     : None
*********************************************************************************************************
*/

_OS_CPU_ISR_Exit:
interrupt_cpu_restore_context:

    LB1   = [ SP ++ ];
    LB0   = [ SP ++ ];
    LT1   = [ SP ++ ];
    LT0   = [ SP ++ ];
    LC1   = [ SP ++ ];
    LC0   = [ SP ++ ];
    R0    = [ SP ++ ];
    A1.w  = R0;
    R0    = [ SP ++ ];
    A1.x  = R0.L;
    R0    = [ SP ++ ];
    A0.w  = R0;
    R0    = [ SP ++ ];
    A0.x  = R0.L;
    M3    = [ SP ++ ];
    M2    = [ SP ++ ];
    M1    = [ SP ++ ];
    M0    = [ SP ++ ];
    L3    = [ SP ++ ];
    L2    = [ SP ++ ];
    L1    = [ SP ++ ];
    L0    = [ SP ++ ];
    B3    = [ SP ++ ];
    B2    = [ SP ++ ];
    B1    = [ SP ++ ];
    B0    = [ SP ++ ];
    I3    = [ SP ++ ];
    I2    = [ SP ++ ];
    I1    = [ SP ++ ];
    I0    = [ SP ++ ];
    FP    = [ SP ++ ];
    (R7:3, P5:3) = [ SP ++ ];
    RETI  = [ SP ++ ];                 /* IPEND[4] will stay set when RETI popped from stack           */
    ASTAT = [ SP ++ ];
    P2    = [ SP ++ ];
    P0    = [ SP ++ ];
    R2    = [ SP ++ ];
    R1    = [ SP ++ ];
    RETS  = [ SP ++ ];
    P1    = [ SP ++ ];

interrupt_cpu_restore_context.end:

    R0    = [ SP ++ ];
    RTI;                              /* Reenable interrupts via IPEND[4] bit after RTI executes.      */
    NOP;                              /* Return to task                                                */
    NOP;

_OS_CPU_ISR_Exit.end:
    NOP;


/*
*********************************************************************************************************
*                                          START MULTITASKING
*                                           OSStartHighRdy()
*
* Description: Starts the highest priority task that is available to run.OSStartHighRdy() MUST:
*
*              a) Call OSTaskSwHook() then,
*              b) Set OSRunning to TRUE,
*              c) Switch to the highest priority task.
*
*              Refer to VisualDSP++ C/C++ Compiler and Library Manual for Blackfin Processors
*              and ADSP-BF53x/BF56x Blackfin� Processor Programming Reference Manual.
*
*              The convention for the task frame (after context save is complete) is as follows:
*              (stack represented from high to low memory as per convention)
*
*                                 (*** High memory ***) R0
*                                                       P1
*                                                       RETS       (function return address of thread)
*                                                       R1
*                                                       R2
*                                                       P0
*                                                       P2
*                                                       ASTAT
*                                                       RETI      (interrupt return address: $PC of thread)
*                                                       R7:3    (R7 is lower than R3)
*                                                       P5:3    (P5 is lower than P3)
*                                                       FP      (frame pointer)
*                                                       I3:0    (I3 is lower than I0)
*                                                       B3:0    (B3 is lower than B0)
*                                                       L3:0    (L3 is lower than L0)
*                                                       M3:0    (M3 is lower than M0)
*                                                       A0.x
*                                                       A0.w
*                                                       A1.x
*                                                       A1.w
*                                                       LC1:0    (LC1 is lower than LC0)
*                                                       LT1:0    (LT1 is lower than LT0)
*               OSTCBHighRdy--> OSTCBStkPtr --> (*** Low memory ***)LB1:0    (LB1 is lower than LB0)
*
* Arguments   : None
*
* Returns     : None
*
* Note(s)     : None
*********************************************************************************************************
*/

_OSStartHighRdy:


    LOADA(P1, _OSTCBHighRdyPtr);            /* Get the SP for the highest ready task                   */
    P2     = [ P1 ];
    SP     = [ P2 ];

                                            /* Restore CPU context without  popping off RETI           */
    P1     = 140;                           /* Skipping over LB1:0, LT1:0, LC1:0, A1:0, M3:0,          */
    SP     = SP + P1;                       /* L3:0, B3:0, I3:0, FP, P5:3, R7:3                        */
    RETS   = [ SP ++ ];                     /* Pop off RETI value into RETS                            */
    SP    += 12;                            /* Skipping over ASAT, P2, P0                              */

    R1     = 0;                             /* Zap loop counters to zero, to make sure                 */
    LC0    = R1; LC1 = R1;                  /* that hw loops are disabled                              */
    L0     = R1; L1 = R1;                   /* Clear the DAG Length regs too, so that it's safe        */
    L2     = R1; L3 = R1;                   /* to use I-regs without them wrapping around.             */

    R2     = [ SP ++ ];                     /* Loading the 3rd argument of the C function - R2         */
    R1     = [ SP ++ ];                     /* Loading the 2nd argument of the C function - R1         */
    SP    += 8;                             /* Skipping over RETS, P1                                  */
    R0     = [ SP ++ ];                     /* Loading the 1st argument of the C function - R0         */

                                            /* Return to high ready task                               */
_OSStartHighRdy.end:
    RTS;


/*
*********************************************************************************************************
*                                          O/S CPU Context Switch
*                                             OSCtxSw()
*
* Description :  This function is called to switch the context of the current running task
*                This function is registered as the IVG14 handler and will be called to handle both
*                interrupt and task level context switches.
*
*                Refer to VisualDSP++ C/C++ Compiler and Library Manual for Blackfin Processors
*                and ADSP-BF53x/BF56x Blackfin� Processor Programming Reference Manual.
*
*                The convention for the task frame (after context save is complete) is as follows:
*                (stack represented from high to low memory as per convention)
*                                          (*** High memory ***) R0
*                                                                P1
*                                                                RETS       (function return address of thread)
*                                                                R1
*                                                                R2
*                                                                P0
*                                                                P2
*                                                                ASTAT
*                                                                RETI      (interrupt return address: $PC of thread)
*                                                                R7:3    (R7 is lower than R3)
*                                                                P5:3    (P5 is lower than P3)
*                                                                FP        (frame pointer)
*                                                                I3:0    (I3 is lower than I0)
*                                                                B3:0    (B3 is lower than B0)
*                                                                L3:0    (L3 is lower than L0)
*                                                                M3:0    (M3 is lower than M0)
*                                                                A0.x
*                                                                A0.w
*                                                                A1.x
*                                                                A1.w
*                                                                LC1:0    (LC1 is lower than LC0)
*                                                                LT1:0    (LT1 is lower than LT0)
*                OSTCBHighRdy--> OSTCBStkPtr --> (*** Low memory ***)LB1:0    (LB1 is lower than LB0)
*
* Arguments   : None
*
* Returns     : None
*
* Note(s)     : None
*********************************************************************************************************
*/

_OSCtxSw:
                                           /* Save context, interrupts disabled by IPEND[4] bit         */
    [ -- SP ]    = R0;
    [ -- SP ]    = P1;
    [ -- SP ]    = RETS;
    [ -- SP ]    = R1;
    [ -- SP ]    = R2;
    [ -- SP ]    = P0;
    [ -- SP ]    = P2;
    [ -- SP ]    = ASTAT;
    R1           = RETI;                   /* IPEND[4] is currently set, globally disabling interrupts  */
                                           /* IPEND[4] will stay set when RETI is saved through R1      */

    [ -- SP ]    = R1;
    [ -- SP ]    = (R7:3, P5:3);
    [ -- SP ]    = FP;
    [ -- SP ]    = I0;
    [ -- SP ]    = I1;
    [ -- SP ]    = I2;
    [ -- SP ]    = I3;
    [ -- SP ]    = B0;
    [ -- SP ]    = B1;
    [ -- SP ]    = B2;
    [ -- SP ]    = B3;
    [ -- SP ]    = L0;
    [ -- SP ]    = L1;
    [ -- SP ]    = L2;
    [ -- SP ]    = L3;
    [ -- SP ]    = M0;
    [ -- SP ]    = M1;
    [ -- SP ]    = M2;
    [ -- SP ]    = M3;
    R1.L         = A0.x;
    [ -- SP ]    = R1;
    R1           = A0.w;
    [ -- SP ]    = R1;
    R1.L         = A1.x;
    [ -- SP ]    = R1;
    R1           = A1.w;
    [ -- SP ]    = R1;
    [ -- SP ]    = LC0;
    R3           = 0;
    LC0          = R3;
    [ -- SP ]    = LC1;
    R3           = 0;
    LC1          = R3;
    [ -- SP ]    = LT0;
    [ -- SP ]    = LT1;
    [ -- SP ]    = LB0;
    [ -- SP ]    = LB1;
    L0           = 0 ( X );
    L1           = 0 ( X );
    L2           = 0 ( X );
    L3           = 0 ( X );

                                           /* Note: OSCtxSw uses call-preserved registers (R4:7, P3:5)  */
                                           /* unlike OSIntCtxSw to allow calling _OSTaskSwHook.         */
    LOADA(P3, _OSPrioHighRdy);             /* Get a high ready task priority                            */
    R4           = B[ P3 ](Z);
    LOADA(P3, _OSPrioCur);                 /* Get a current task priority                               */
    R5           = B[ P3 ](Z);
    LOADA(P4, _OSTCBCurPtr);               /* Get a pointer to the current task's TCB                   */
    P5           = [ P4 ];
    [ P5 ]       = SP;                     /* Context save done so save SP in the TCB                   */

    B[ P3 ]      = R4;                     /* OSPrioCur = OSPrioHighRdy                                 */
    LOADA(P3, _OSTCBHighRdyPtr);           /* Get a pointer to the high ready task's TCB                */
    P5           = [ P3 ];
    [ P4 ]       = P5;                     /* OSTCBCur = OSTCBHighRdy                                   */

_OSCtxSw_modify_SP:

    SP           = [ P5 ];                 /* Make it the current task by switching the stack pointer   */

_AbortOSCtxSw:
_CtxSwRestoreCtx:
_OSCtxSw.end:
                                           /* Restoring CPU context and return to task                  */
    LB1          = [ SP ++ ];
    LB0          = [ SP ++ ];
    LT1          = [ SP ++ ];
    LT0          = [ SP ++ ];
    LC1          = [ SP ++ ];
    LC0          = [ SP ++ ];
    R0           = [ SP ++ ];
    A1.w         = R0;
    R0           = [ SP ++ ];
    A1.x         = R0.L;
    R0           = [ SP ++ ];
    A0.w         = R0;
    R0           = [ SP ++ ];
    A0.x         = R0.L;
    M3           = [ SP ++ ];
    M2           = [ SP ++ ];
    M1           = [ SP ++ ];
    M0           = [ SP ++ ];
    L3           = [ SP ++ ];
    L2           = [ SP ++ ];
    L1           = [ SP ++ ];
    L0           = [ SP ++ ];
    B3           = [ SP ++ ];
    B2           = [ SP ++ ];
    B1           = [ SP ++ ];
    B0           = [ SP ++ ];
    I3           = [ SP ++ ];
    I2           = [ SP ++ ];
    I1           = [ SP ++ ];
    I0           = [ SP ++ ];
    FP           = [ SP ++ ];
    (R7:3, P5:3) = [ SP ++ ];
    RETI         = [ SP ++ ];              /* IPEND[4] will stay set when RETI popped from stack        */
    ASTAT        = [ SP ++ ];
    P2           = [ SP ++ ];
    P0           = [ SP ++ ];
    R2           = [ SP ++ ];
    R1           = [ SP ++ ];
    RETS         = [ SP ++ ];
    P1           = [ SP ++ ];
    R0           = [ SP ++ ];
    RTI;                                   /* Reenable interrupts via IPEND[4] bit after RTI executes.  */
                                           /* Return to task                                            */

/*
*********************************************************************************************************
*                                             OSIntCtxSw()
*
* Description :  Performs the Context Switch from an ISR.
*
*                OSIntCtxSw() must implement the following pseudo-code:
*
*                  OSTaskSwHook();
*                  OSPrioCur = OSPrioHighRdy;
*                  OSTCBCur  = OSTCBHighRdy;
*                  SP        = OSTCBHighRdy->OSTCBStkPtr;
*
*
*
* Arguments   : None
*
* Returns     : None
*
* Note(s)     : OSIntCtxSw uses scratch registers (R0:3, P0:2, ASTAT)
*               unlike OSCtxSw because it is called from OSIntExit and will
*               return back to OSIntExit through an RTS.
*********************************************************************************************************
*/

_OSIntCtxSw:

    LOADA(P0, _OSPrioCur);                    /* Get a current task priority                           */
    LOADA(P1, _OSPrioHighRdy);                /* Get a high ready task priority                        */
    R0      = B[ P1 ](Z);
    B[ P0 ] = R0;                             /* OSPrioCur = OSPrioHighRdy                             */
    LOADA(P0, _OSTCBCurPtr);                  /* Get a pointer to the current task's TCB               */
    LOADA(P1, _OSTCBHighRdyPtr);              /* Get a pointer to the high ready task's TCB            */
    P2 = [ P1 ];
    [ P0 ]  = P2;                             /* OSTCBCur = OSTCBHighRdy                               */

_OSIntCtxSw_modify_SP:
    /* Load stack pointer into P0 from task TCB. Do not modify SP because we return to                 */
    /* OSIntExit, which modifies SP through the the UNLINK instruction. Instead, we                    */
    /* modify the FP of the ISR frame. This essentially 'moves the frame of the ISR                    */
    /* 'above' the frame the of the new task. Thus, when the UNLINK instruction that                   */
    /* unlinks the frame of the ISR, the SP of the new task is loaded. Thus, the new                   */
    /* context is restored.                                                                            */


    R0      = [ P2 ];                         /* Get stack pointer to the high ready task              */
    R0     += -8;                             /* Subtract 2 stack items for FP, RETS (LINK instruction */
                                              /* pushes FP, RETS onto stack)                           */
    [ FP ]  = R0;                             /* Modify the FP of ISR with the task's SP.              */


    SP     += -4;                             /* Interrupts are re-enabled when OSIntCtxSw() returns   */
    RETI    = [ SP++ ];                       /* to OSIntExit() (where OS_EXIT_CRITICAL() restores     */
                                              /* original IMASK value).                                */
                                              /* However, the context restore process must be          */
                                              /* uninterruptible - we accomplish this by an artificial'*/
                                              /* stack pop into the RETI register, thus setting the    */
                                              /* global interrupts disable bit (IPEND[4])              */
                                              /* Before the stack pop, adjust the stack pointer.       */

_OSIntCtxSw.end:
    RTS;


/*
*********************************************************************************************************
*                                          _OS_Invalid_Task_Return ()
*
* Description :  All tasks (threads) within �COS-II are while(1) loops - once done with it's operations,
*                task can supsend, delete itself etc. to yield the processor. Under NO circumstances can
*                a task return  with a RTS. However, in case of a return, the following function serves
*                as a placeholder for debugging  purposes.
*
* Arguments   : None
*
* Returns     : None
*
* Note(s)     : None
*********************************************************************************************************
*/

_OS_CPU_Invalid_Task_Return:

_OS_CPU_Invalid_Task_Return.end:              /* Stay here                                             */
    JUMP 0;
