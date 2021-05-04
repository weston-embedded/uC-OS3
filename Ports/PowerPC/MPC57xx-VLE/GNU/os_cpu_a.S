#********************************************************************************************************
#                                              uC/OS-III
#                                        The Real-Time Kernel
#
#                    Copyright 2009-2021 Silicon Laboratories Inc. www.silabs.com
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
#                                           MPC57xx VLE Port
#                                             GNU Toolchain
#
# File    : os_cpu_a.S
# Version : V3.08.01
#********************************************************************************************************

    .include  "os_cpu_a.inc"

#********************************************************************************************************
#                                           PUBLIC FUNCTIONS
#********************************************************************************************************

    .global OSStartHighRdy
    .global OSCtxSw
    .global OSIntCtxSw

    .global OS_ExceptHandler_CriticalInput
    .global OS_ExceptHandler_MachineCheck
    .global OS_ExceptHandler_DataStorage
    .global OS_ExceptHandler_InstructionStorage
    .global OS_ExceptHandler_ExternalInput
    .global OS_ExceptHandler_Alignment
    .global OS_ExceptHandler_Program
    .global OS_ExceptHandler_PerformanceMonitor
    .global OS_ExceptHandler_SystemCall
    .global OS_ExceptHandler_Debug
    .global OS_ExceptHandler_EFPUDataException
    .global OS_ExceptHandler_EFPURoundException
    .global OS_ExceptHandler_EFPUUnavailable
    .global OS_ExceptHandler_IVOR13
    .global OS_ExceptHandler_IVOR14
    .global OS_ExceptHandler_IVOR15


    .extern OSTCBCurPtr                                         # 4 bytes
    .extern OSTCBHighRdyPtr                                     # 4 bytes
    .extern OSPrioCur                                           # 1 byte
    .extern OSPrioHighRdy                                       # 1 byte
    .extern OSTaskSwHook                                        # 4 bytes
    .extern OS_CPU_IntHandler                                   # 4 bytes
    .extern OSIntNestingCtr                                     # 4 bytes
    .extern OS_CPU_ISRStkBase                                   # 4 bytes
    .extern OS_CPU_ISRNestingCtr                                # 1 byte


#********************************************************************************************************
#                                      CODE GENERATION DIRECTIVES
#********************************************************************************************************

    .section  .text


#********************************************************************************************************
#                                          START MULTITASKING
#
# Description : This function is called by OSStart() to start the highest priority task that was created
#               by your application before calling OSStart().
#
# Arguments   : none
#
# Note(s)     : 1) See Note #2 of OSTaskStkInit() in os_cpu.h for the stack frame's layout.
#
#               2) OSStartHighRdy() MUST:
#                      a) Call OSTaskSwHook() then,
#                      b) Switch to the highest priority task.
#********************************************************************************************************

OSStartHighRdy:
    e_bl        OSTaskSwHook                                    # Call OSTaskSwHook.

    e_li        r0, 1                                           # Set OSRunning to 1.
    e_lis       r11,OSRunning@ha                                # ...
    e_stb       r0,OSRunning@l(r11)                             # ...

    e_lis       r11,OSTCBHighRdyPtr@ha                          # Get pointer to ready task TCB.
    e_lwz       r11,OSTCBHighRdyPtr@l(r11)                      # ...

    e_lis       r12,OSTCBCurPtr@ha                              # Save as current task TCB ptr.
    e_stw       r11,OSTCBCurPtr@l(r12)                          # ...

    e_lwz       r1, 0(r11)                                      # Get new stack pointer.

    epilogue                                                    # Restore context.

    se_rfi                                                      # Run task.


#********************************************************************************************************
#                                           EXCEPTION VECTORS
#********************************************************************************************************

#********************************************************************************************************
#                                    OS_ExceptHandler_CriticalInput
#                                                 IVOR0
#
# Description: Unused.
#********************************************************************************************************

OS_ExceptHandler_CriticalInput:
    e_b         OS_ExceptHandler_CriticalInput


#********************************************************************************************************
#                                     OS_ExceptHandler_MachineCheck
#                                                 IVOR1
#
# Description:  Unused.
#********************************************************************************************************

OS_ExceptHandler_MachineCheck:
    e_b         OS_ExceptHandler_MachineCheck


#********************************************************************************************************
#                                     OS_ExceptHandler_DataStorage
#                                                 IVOR2
#
# Description: Unused.
#********************************************************************************************************

OS_ExceptHandler_DataStorage:
    e_b         OS_ExceptHandler_DataStorage


#********************************************************************************************************
#                                  OS_ExceptHandler_InstructionStorage
#                                                 IVOR3
#
# Description: Unused.
#********************************************************************************************************

OS_ExceptHandler_InstructionStorage:
    e_b         OS_ExceptHandler_InstructionStorage


#********************************************************************************************************
#                                    OS_ExceptHandler_ExternalInput
#                                                 IVOR4
#
# Description: Switches to interrupt stack and calls the user-defined function
#              void OS_CPU_IntHandler(void).
#********************************************************************************************************

OS_ExceptHandler_ExternalInput:
    prologue

    e_lis       r11,OS_CPU_ISRNestingCtr@ha                     # Increment the ISR nesting counter to reflect the current nesting level.
    e_or2i      r11,OS_CPU_ISRNestingCtr@l                      # ...
    e_lwz       r12,0(r11)                                      # ...
    e_add2i.    r12,1                                           # ...
    e_stw       r12,0(r11)                                      # ...

    e_cmpl16i   r12,1                                           # Check to see if we are the first interrupt.
    e_bgt       IntStkInUse                                     # If we are nested the ISR stack is already being used.
                                                                # Otherwise, the first interrupt must switch to the ISR stack.

    e_lis       r11,OSTCBCurPtr@ha                              # Get a pointer to current TCB.
    e_lwz       r11,OSTCBCurPtr@l(r11)                          # ...
    e_stw       r1,0(r11)                                       # Save the current stack pointer in the current TCB.

    e_lis       r11,OS_CPU_ISRStkBase@ha                        # Get a pointer to 8-byte aligned base of the ISR stack.
    e_lwz       r11,OS_CPU_ISRStkBase@l(r11)                    # ...
    mr          r1,r11                                          # Switch to the ISR stack.


IntStkInUse:
                                                                ### ALLOCATE INITIAL FRAME OF THE ISR CALL STACK ###
    e_stwu      0,-4(r1)                                        # Clear the initial frame (Backchain must be null according to EABI)
    e_stwu      0,-4(r1)                                        # ...

    e_bl        OS_CPU_IntHandler                               # Unnested kernel-aware interrupts will return through OSIntCtxSw.

                                                                ### NESTED OR FAST ISRS ONLY ###
    wrteei      0                                               # OS_CPU_IntHandler may have enabled interrupts. Make sure they are disabled.

    e_add2i.    r1,8                                            ### DEALLOCATE INITIAL FRAME OF THE ISR CALL STACK ###

    e_lis       r11,OS_CPU_ISRNestingCtr@ha                     # Decrement the ISR nesting counter to reflect the current nesting level.
    e_or2i      r11,OS_CPU_ISRNestingCtr@l                      # ...
    e_lwz       r12,0(r11)                                      # ...
    e_add2i.    r12,-1                                          # ...
    e_stw       r12,0(r11)                                      # ...

    e_cmpl16i   r12,0                                           # Check to see if we are nested.
    e_bgt       NestedReturn                                    # If we are, the previous frame is in the ISR stack.
                                                                # Otherwise, we must switch back to the task stack.

    e_lis       r11,OSTCBCurPtr@ha                              # Get pointer to current TCB.
    e_lwz       r11,OSTCBCurPtr@l(r11)                          # ...
    e_lwz       r1,0(r11)                                       # Load stack pointer from current TCB.


NestedReturn:
    epilogue                                                    # If we are nested or not kernel-aware, pop the previous ISR context and rfi.

    se_rfi


#********************************************************************************************************
#                                PERFORM A CONTEXT SWITCH (From an ISR)
#
# Description : This routine is called when an ISR makes a higher priority task ready-to-run.
#
#               Interrupts should be disabled by the kernel before this point.
#********************************************************************************************************

OSIntCtxSw:
    e_bl        OSTaskSwHook                                    # Call OSTaskSwHook.

    e_lis       r11,OSTCBHighRdyPtr@ha                          # Get pointer to ready task TCB.
    e_lwz       r11,OSTCBHighRdyPtr@l(r11)                      # ...

    e_lis       r12,OSTCBCurPtr@ha                              # Save as current task TCB ptr.
    e_stw       r11,OSTCBCurPtr@l(r12)                          # ...

    e_lis       r12,OSPrioHighRdy@ha                            # Get High Ready Priority.
    e_lbz       r10,OSPrioHighRdy@l(r12)                        # ...

    e_lis       r12,OSPrioCur@ha                                # Save as Current Priority.
    e_stb       r10,OSPrioCur@l(r12)                            # ...

    e_lwz       r1,0(r11)                                       # Get new stack pointer.

    e_lis       r11,OS_CPU_ISRNestingCtr@ha                     # Decrement the ISR nesting counter to reflect the current nesting level.
    e_or2i      r11,OS_CPU_ISRNestingCtr@l                      # ...
    e_lwz       r12,0(r11)                                      # ...
    e_add2i.    r12,-1                                          # ...
    e_stw       r12,0(r11)                                      # ...

    epilogue                                                    # Restore context.

    se_rfi                                                      # Run task


#********************************************************************************************************
#                                      OS_ExceptHandler_Alignment
#                                                 IVOR5
#
# Description: Unused.
#********************************************************************************************************

OS_ExceptHandler_Alignment:
    e_b         OS_ExceptHandler_Alignment


#********************************************************************************************************
#                                       OS_ExceptHandler_Program
#                                                 IVOR6
#
# Description: Unused.
#********************************************************************************************************

OS_ExceptHandler_Program:
    e_b         OS_ExceptHandler_Program


#********************************************************************************************************
#                                  OS_ExceptHandler_PerformanceMonitor
#                                                 IVOR7
#
# Description: Unused.
#********************************************************************************************************

OS_ExceptHandler_PerformanceMonitor:
    e_b         OS_ExceptHandler_PerformanceMonitor


#********************************************************************************************************
#                                      OS_ExceptHandler_SystemCall
#                                                 IVOR8
#
# Description: Triggered by the kernel in order to perform a context switch.
#********************************************************************************************************

OS_ExceptHandler_SystemCall:
    e_b         OSCtxSw


#********************************************************************************************************
#                                       TASK LEVEL CONTEXT SWITCH
#
# Description : This routine is called when a task makes a higher priority task ready-to-run.
#               The pseudo code is:
#
#                   OS_CTX_SAVE
#                   OSTCBCurPtr->SP = SP
#                   OSTaskSwHook()
#                   OSPrioCur       = OSPrioHighRdy
#                   OSTCBCurPtr     = OSTCBHighRdyPtr
#                   SP              = OSTCBCurPtr->StkPtr
#                   OS_CTX_RESTORE
#                   Return from interrupt/exception
#
#               Interrupts should be disabled by the kernel before this point.
#********************************************************************************************************

OSCtxSw:
    prologue                                                    # Save context.

    e_lis       r11,OSTCBCurPtr@ha                              # Get pointer to current TCB.
    e_lwz       r11,OSTCBCurPtr@l(r11)                          # ...
    e_stw       r1, 0(r11)                                      # Save stack pointer in current TCB.

    e_bl        OSTaskSwHook                                    # Call OSTaskSwHook.

    e_lis       r11,OSTCBHighRdyPtr@ha                          # Get pointer to ready task TCB.
    e_lwz       r11,OSTCBHighRdyPtr@l(r11)                      # ...

    e_lis       r12,OSTCBCurPtr@ha                              # Save as current task TCB ptr.
    e_stw       r11,OSTCBCurPtr@l(r12)                          # ...

    e_lis       r12,OSPrioHighRdy@ha                            # Get High Ready Priority.
    e_lbz       r10,OSPrioHighRdy@l(r12)                        # ...

    e_lis       r12,OSPrioCur@ha                                # Save as Current Priority.
    e_stb       r10,OSPrioCur@l(r12)                            # ...

    e_lwz       r1,0(r11)                                       # Get new stack pointer from TCB.

    epilogue                                                    # Restore context.

    se_rfi                                                      # Run task.


#********************************************************************************************************
#                                        OS_ExceptHandler_Debug
#                                                 IVOR9
#
# Description: Unused.
#********************************************************************************************************

OS_ExceptHandler_Debug:
    e_b         OS_ExceptHandler_Debug


#********************************************************************************************************
#                                   OS_ExceptHandler_EFPUDataException
#                                                IVOR10
#
# Description: Unused.
#********************************************************************************************************

OS_ExceptHandler_EFPUDataException:
    e_b         OS_ExceptHandler_EFPUDataException


#********************************************************************************************************
#                                   OS_ExceptHandler_EFPURoundException
#                                                IVOR11
#
# Description: Unused.
#********************************************************************************************************

OS_ExceptHandler_EFPURoundException:
    e_b         OS_ExceptHandler_EFPURoundException


#********************************************************************************************************
#                                   OS_ExceptHandler_EFPUUnavailable
#                                                IVOR12
#
# Description: Unused.
#********************************************************************************************************

OS_ExceptHandler_EFPUUnavailable:
    e_b         OS_ExceptHandler_EFPUUnavailable


#********************************************************************************************************
#                                       OS_ExceptHandler_IVOR13
#                                                IVOR13
#
# Description: Reserved.
#********************************************************************************************************

OS_ExceptHandler_IVOR13:
    e_b         OS_ExceptHandler_IVOR13


#********************************************************************************************************
#                                        OS_ExceptHandler_IVOR14
#                                                IVOR14
#
# Description: Reserved.
#********************************************************************************************************

OS_ExceptHandler_IVOR14:
    e_b         OS_ExceptHandler_IVOR14


#********************************************************************************************************
#                                        OS_ExceptHandler_IVOR15
#                                                IVOR15
#
# Description: Reserved.
#********************************************************************************************************

OS_ExceptHandler_IVOR15:
    e_b         OS_ExceptHandler_IVOR15


    .end
