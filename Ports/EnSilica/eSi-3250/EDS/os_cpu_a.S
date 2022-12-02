#********************************************************************************************************
#                                              uC/OS-III
#                                        The Real-Time Kernel
#
#                    Copyright 2009-2022 Silicon Laboratories Inc. www.silabs.com
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
#                                        EnSilica eSi-32nn Port
#
# File      : os_cpu_a.S
# Version   : V3.08.02
#********************************************************************************************************
# For       : EnSilica eSi-32nn
# Mode      : Little-Endian, 8, 16 or 32 registers
# Toolchain : GNU C Compiler
#********************************************************************************************************

#********************************************************************************************************
#                                            PUBLIC FUNCTIONS
#********************************************************************************************************

    .global  OSCtxSw
    .global  OSIntCtxSw

    .global  OSStartHighRdy

    .global  OS_CPU_EnSilica_SystemCallHandler
    .global  OS_CPU_EnSilica_ExceptionHandler

    .global  OS_CPU_SetRegs


#********************************************************************************************************
#                                       EXTERNAL GLOBAL VARIABLES
#********************************************************************************************************

    .extern  OSPrioCur                                           # Declared as CPU_INT08U   ,  8-bit wide.
    .extern  OSPrioHighRdy                                       # Declared as CPU_INT08U   ,  8-bit wide.
    .extern  OSRunning                                           # Declared as CPU_INT08U   ,  8-bit wide.
    .extern  OSTCBCurPtr                                         # Declared as OS_TCB *     , 32-bit wide.
    .extern  OSTCBHighRdyPtr                                     # Declared as OS_TCB *     , 32-bit wide.
    .extern  OSTaskSwHook                                        # Declared as CPU_FNCT_VOID, 32-bit wide.
    .extern  OS_CPU_ExceptionHandler                             # Declared as CPU_FNCT_VOID, 32-bit wide.
    .extern  OS_CPU_InterruptHandler                             # Declared as CPU_FNCT_VOID, 32-bit wide.


#********************************************************************************************************
#                                                 MACROS
#********************************************************************************************************

.macro OS_CTX_RESTORE
                                                                # Restore PC.
    LW    arg0, (sp+=[1])
    WCSR  Thread, EPC, arg0
                                                                # Restore CAS.
    LW    arg0, (sp+=[1])
    WCSR  Thread, ECAS, arg0
                                                                # Restore TC.
    LW    arg0, (sp+=[1])
    WCSR  Thread, ETC, arg0
                                                                # Restore registers.
#if   __registers__ == 8
    POP   {r7-r1} , (sp)+=[1]
#elif __registers__ == 16
    POP   {r15-r1}, (sp)+=[1]
#elif __registers__ == 32
    POP   {r31-r1}, (sp)+=[1]
#endif
    ERET
.endm

.macro OS_REG_SAVE
                                                                # Save registers.
#if   __registers__ == 8
    PUSH  (sp)-=[1], {r1-r7}
#elif __registers__ == 16
    PUSH  (sp)-=[1], {r1-r15}
#elif __registers__ == 32
    PUSH  (sp)-=[1], {r1-r31}
#endif
.endm


#********************************************************************************************************
#                                                EQUATES
#********************************************************************************************************


#********************************************************************************************************
#                                       CODE GENERATION DIRECTIVES
#********************************************************************************************************

   .text


#********************************************************************************************************
#                                           START MULTITASKING
#                                       void OSStartHighRdy(void)
#
# Description : This function is called by OSStart() to start the highest priority task that was created
#               by your application before calling OSStart().
#
# Arguments   : none
#
# Note(s)     : 1) OSStartHighRdy() MUST:
#                      a) Call OSTaskSwHook().
#                      b) Restore context for OSTCBCurPtr.
#                      c) ERET into highest ready task.
#********************************************************************************************************

    .type       OSStartHighRdy, @function
OSStartHighRdy:
                                                                # Call OSTaskSwHook().
    L     arg0, (gp+[OSTaskSwHook])
    CALL  arg0
                                                                # Get SP from OSTCBCurPtr->StkPtr.
    L     arg0, (gp+[OSTCBCurPtr])
    LW    arg0, (arg0)
    LW    sp, (arg0)
                                                                # Restore Context.
    OS_CTX_RESTORE


#********************************************************************************************************
#                         PERFORM A CONTEXT SWITCH (From task level) - OSCtxSw()
#
# Note(s) : 1) OSCtxSw() is called when OS wants to perform a task context switch.  This function
#              triggers the SystemCall exception which is where the real work is done.
#********************************************************************************************************

    .type       OSCtxSw, @function
OSCtxSw:
    SCALL
    RET


#********************************************************************************************************
#                     PERFORM A CONTEXT SWITCH (From interrupt level) - OSIntCtxSw()
#
# Note(s) : 1) OSIntCtxSw() is called by OSIntExit() when it determines a context switch is needed as
#              the result of an interrupt.
#********************************************************************************************************

    .type       OSIntCtxSw, @function
OSIntCtxSw:
                                                                # Call OSTaskSwHook().
    L     arg0, (gp+[OSTaskSwHook])
    CALL  arg0
                                                                # OSPrioCur = OSPrioHighRdy;
    L     arg0, (gp+[OSPrioHighRdy])
    L     arg1, (gp+[OSPrioCur])
    LB    arg0, (arg0)
    SB    (arg1), arg0
                                                                # OSTCBCurPtr = OSTCBHighRdyPtr;
    L     arg0, (gp+[OSTCBHighRdyPtr])
    L     arg1, (gp+[OSTCBCurPtr])
    LW    arg0, (arg0)
    SW    (arg1), arg0
                                                                # Get SP from OSTCBHighRdyPtr->StkPtr.
    LW    sp, (arg0)
                                                                # Restore Context.
    OS_CTX_RESTORE


#********************************************************************************************************
#                                       GENERIC EXCEPTION HANDLER
#                              void OS_CPU_EnSilica_ExceptionHandler(void)
#
# Note(s) : 1) This is the global exception handler. It handles all kernel-aware exceptions and interrupts
#              within an EnSilica core.
#
#           2) Based on the Exception ID as defined in Table 26 of 'eSi-RISC Architecture Manual', the
#              global exception handler calls:
#                  OS_CPU_ExceptionHandler      If the Exception ID (EID) is <  0x20
#                  OS_CPU_InterruptHandler      If the Exception ID (EID) is >= 0x20
#
#           3) This handler should fill all Exception Table entries except for entry 0 (Reset) and
#              5 (SystemCall).
#********************************************************************************************************

    .type       OS_CPU_EnSilica_ExceptionHandler, @function
OS_CPU_EnSilica_ExceptionHandler:
                                                                # Save registers.
    OS_REG_SAVE
                                                                # Save TC.
    RCSR  arg0, Thread, ETC
    SW    (sp)-=[1], arg0
                                                                # Save CAS.
    RCSR  arg0, Thread, ECAS
    SW    (sp)-=[1], arg0
                                                                # Save PC.
    RCSR  arg0, Thread, EPC
    SW    (sp)-=[1], arg0
                                                                # Exception or Interrupt?
    RCSR  arg1, Thread, EID                                     #   1: Interrupt.
    L     arg0, 0x20                                            #   0: Exception.
    CMPGE arg2, arg1, arg0
                                                                # OSRunning?
    L     arg3, (gp+[OSRunning])
    LB    arg3, (arg3)
    BNZ   arg3, OS_CPU_EnSilica_ExceptionHandler_Running

#********************************************************************************************************
#                               GENERIC EXCEPTION HANDLER: OS NOT RUNNING
#
# Note(s) : 1) If the OS is not running, the pseudo-code is:
#              a) Call either OS_CPU_ExceptionHandler or OS_CPU_InterruptHandler;
#              b) Restore context and return to interrupted code.
#
#           2) Register usage:
#                  arg0     Argument
#                  arg1     EID
#                  arg2     Result of 'EID >= 0x20'
#                  arg3     Buffer
#********************************************************************************************************

OS_CPU_EnSilica_ExceptionHandler_NotRunning:
                                                                # Call Exception or Interrupt Handler?
    MV    arg0, arg1
    BZ    arg2, 1f
    ADD   arg0, -0x18
    L     arg3, (gp+[OS_CPU_InterruptHandler])
    CALL  arg3
    B     2f
1:
    L     arg3, (gp+[OS_CPU_ExceptionHandler])
    CALL  arg3
2:
                                                                # Restore Context.
    OS_CTX_RESTORE

#********************************************************************************************************
#                                 GENERIC EXCEPTION HANDLER: OS RUNNING
#
# Note(s) : 1) If the OS is running, the pseudo-code is:
#              a) OSTCBCurPtr->StkPtr = SP;
#              b) Call OSIntEnter();
#              c) Call either OS_CPU_ExceptionHandler or OS_CPU_InterruptHandler;
#              d) Call OSIntExit();
#              e) (if OSIntExit() returns): Restore context and return to interrupted task.
#
#           2) Register usage:
#                  arg0     Buffer/Argument
#                  arg1     EID
#                  arg2     Result of 'EID >= 0x20'
#                  arg3     Buffer
#********************************************************************************************************

OS_CPU_EnSilica_ExceptionHandler_Running:
                                                                # Save SP in OSTCBCurPtr->StkPtr.
    L     arg0, (gp+[OSTCBCurPtr])
    LW    arg0, (arg0)
    SW    (arg0), sp
                                                                # Call OSIntEnter().
    L     arg0, (gp+[OSIntEnter])
    CALL  arg0
                                                                # Call Exception or Interrupt Handler?
    MV    arg0, arg1
    BZ    arg2, 1f
    ADD   arg0, -0x18
    L     arg3, (gp+[OS_CPU_InterruptHandler])
    CALL  arg3
    B     2f
1:
    L     arg3, (gp+[OS_CPU_ExceptionHandler])
    CALL  arg3
2:
                                                                # Call OSIntExit().
    L     arg3, (gp+[OSIntExit])
    CALL  arg3
                                                                # Restore Context.
    OS_CTX_RESTORE

#********************************************************************************************************
#                                      HANDLE SystemCall EXCEPTION
#                              void OS_CPU_EnSilica_SystemCallHandler(void)
#
# Note(s) : 1) SystemCall is used to cause a context switch. The current context, r1-rN is
#              saved on the current stack before saving the EPC and ECAS.
#
#           2) Pseudo-code is:
#              a) Save registers r1 to rN;
#              b) Save ETC, ECAS and EPC;
#              c) Save the SP (r0): OSTCBCurPtr->OSTCBStkPtr = SP;
#              d) Call OSTaskSwHook();
#              e) Set current high priority   , OSPrioCur   = OSPrioHighRdy;
#              f) Get current ready thread TCB, OSTCBCurPtr = OSTCBHighRdyPtr;
#              g) Get new SP (r0): SP = OSTCBHighRdyPtr->OSTCBStkPtr;
#              h) Restore EPC, ECAS and ETC;
#              i) Restore rN to r1;
#              j) Exception return.
#
#           3) On entry into SystemCall handler:
#              a) The processor is in Exception mode.
#              b) OSTCBCurPtr      points to the OS_TCB of the task to suspend
#                 OSTCBHighRdyPtr  points to the OS_TCB of the task to resume
#
#           4) This function MUST be placed in entry 5 (SystemCall) of the Exception Table.
#********************************************************************************************************

    .type       OS_CPU_EnSilica_SystemCallHandler, @function
OS_CPU_EnSilica_SystemCallHandler:
                                                                # Save registers.
    OS_REG_SAVE
                                                                # Save TC.
    RCSR  arg0, Thread, ETC
    SW    (sp)-=[1], arg0
                                                                # Save CAS.
    RCSR  arg0, Thread, ECAS
    SW    (sp)-=[1], arg0
                                                                # Save PC. Adjust because EPC points to 'scall'.
    RCSR  arg0, Thread, EPC
    ADD   arg0, 2
    SW    (sp)-=[1], arg0
                                                                # Save SP in OSTCBCurPtr->StkPtr.
    L     arg0, (gp+[OSTCBCurPtr])
    LW    arg0, (arg0)
    SW    (arg0), sp
                                                                # Call OSTaskSwHook().
    L     arg0, (gp+[OSTaskSwHook])
    CALL  arg0
                                                                # OSPrioCur = OSPrioHighRdy;
    L     arg0, (gp+[OSPrioHighRdy])
    L     arg1, (gp+[OSPrioCur])
    LB    arg0, (arg0)
    SB    (arg1), arg0

                                                                # OSTCBCurPtr = OSTCBHighRdyPtr;
    L     arg0, (gp+[OSTCBHighRdyPtr])
    L     arg1, (gp+[OSTCBCurPtr])
    LW    arg0, (arg0)
    SW    (arg1), arg0

                                                                # Get SP from OSTCBHighRdyPtr->StkPtr.
    LW    sp, (arg0)
                                                                # Restore Context.
    OS_CTX_RESTORE


#********************************************************************************************************
#                                       CODE GENERATION DIRECTIVES
#********************************************************************************************************

.end
