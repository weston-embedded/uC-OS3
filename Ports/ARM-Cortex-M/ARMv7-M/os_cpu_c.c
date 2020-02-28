/*
*********************************************************************************************************
*                                              uC/OS-III
*                                        The Real-Time Kernel
*
*                    Copyright 2009-2020 Silicon Laboratories Inc. www.silabs.com
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
*                                             ARMv7-M Port
*
* File    : os_cpu_c.c
* Version : V3.08.00
*********************************************************************************************************
* For     : ARMv7-M Cortex-M
* Mode    : Thumb-2 ISA
*********************************************************************************************************
* Note(s) : (1) This port supports the ARM Cortex-M3, Cortex-M4 and Cortex-M7 architectures.
*           (2) It has been tested with the following Hardware Floating Point Unit.
*               (a) Single-precision: FPv4-SP-D16-M and FPv5-SP-D16-M
*               (b) Double-precision: FPv5-D16-M
*********************************************************************************************************
*/

#define   OS_CPU_GLOBALS

#ifdef VSC_INCLUDE_SOURCE_FILE_NAMES
const  CPU_CHAR  *os_cpu_c__c = "$Id: $";
#endif


/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#include  "../../../Source/os.h"


#ifdef __cplusplus
extern  "C" {
#endif

/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/

CPU_INT32U  OS_KA_BASEPRI_Boundary;                             /* Base Priority boundary.                              */


/*
*********************************************************************************************************
*                                         FLOATING POINT DEFINES
*********************************************************************************************************
*/

#define  CPU_REG_FP_FPCCR              (*((CPU_REG32 *)0xE000EF34uL))   /* Floating-Point Context Control Reg.         */

                                                                        /* Enabled FP lazy stacking and enable ..      */
                                                                        /* ..automatic state saving.                   */
#define  CPU_REG_FPCCR_LAZY_STK                        0xC0000000uL


/*
*********************************************************************************************************
*                                           IDLE TASK HOOK
*
* Description: This function is called by the idle task.  This hook has been added to allow you to do
*              such things as STOP the CPU to conserve power.
*
* Arguments  : None.
*
* Note(s)    : None.
*********************************************************************************************************
*/

void  OSIdleTaskHook (void)
{
#if OS_CFG_APP_HOOKS_EN > 0u
    if (OS_AppIdleTaskHookPtr != (OS_APP_HOOK_VOID)0) {
        (*OS_AppIdleTaskHookPtr)();
    }
#endif
}


/*
*********************************************************************************************************
*                                       OS INITIALIZATION HOOK
*
* Description: This function is called by OSInit() at the beginning of OSInit().
*
* Arguments  : None.
*
* Note(s)    : 1) When using hardware floating point please do the following during the reset handler:
*                 a) Set full access for CP10 & CP11 bits in CPACR register.
*                 b) Set bits ASPEN and LSPEN in FPCCR register.
*********************************************************************************************************
*/

void  OSInitHook (void)
{
#if (OS_CPU_ARM_FP_EN > 0u)
    CPU_INT32U   reg_val;
#endif
                                                                /* 8-byte align the ISR stack.                          */
    OS_CPU_ExceptStkBase = (CPU_STK *)(OSCfg_ISRStkBasePtr + OSCfg_ISRStkSize);
    OS_CPU_ExceptStkBase = (CPU_STK *)((CPU_STK)(OS_CPU_ExceptStkBase) & 0xFFFFFFF8);

#if (OS_CPU_ARM_FP_EN > 0u)
    reg_val = CPU_REG_FP_FPCCR;                                 /* Check the floating point mode.                       */
    if ((reg_val & CPU_REG_FPCCR_LAZY_STK) != CPU_REG_FPCCR_LAZY_STK) {
        while (1u) {                                            /* See Note (1).                                        */
            ;
        }
    }
#endif
                                                                /* Set BASEPRI boundary from the configuration.         */
    OS_KA_BASEPRI_Boundary = (CPU_INT32U)(CPU_CFG_KA_IPL_BOUNDARY << (8u - CPU_CFG_NVIC_PRIO_BITS));
}


/*
*********************************************************************************************************
*                                           REDZONE HIT HOOK
*
* Description: This function is called when a task's stack overflowed.
*
* Arguments  : p_tcb        Pointer to the task control block of the offending task. NULL if ISR.
*
* Note(s)    : None.
*********************************************************************************************************
*/
#if (OS_CFG_TASK_STK_REDZONE_EN > 0u)
void  OSRedzoneHitHook (OS_TCB  *p_tcb)
{
#if OS_CFG_APP_HOOKS_EN > 0u
    if (OS_AppRedzoneHitHookPtr != (OS_APP_HOOK_TCB)0) {
        (*OS_AppRedzoneHitHookPtr)(p_tcb);
    } else {
        CPU_SW_EXCEPTION(;);
    }
#else
    (void)p_tcb;                                                /* Prevent compiler warning                             */
    CPU_SW_EXCEPTION(;);
#endif
}
#endif


/*
*********************************************************************************************************
*                                         STATISTIC TASK HOOK
*
* Description: This function is called every second by uC/OS-III's statistics task.  This allows your
*              application to add functionality to the statistics task.
*
* Arguments  : None.
*
* Note(s)    : None.
*********************************************************************************************************
*/

void  OSStatTaskHook (void)
{
#if OS_CFG_APP_HOOKS_EN > 0u
    if (OS_AppStatTaskHookPtr != (OS_APP_HOOK_VOID)0) {
        (*OS_AppStatTaskHookPtr)();
    }
#endif
}


/*
*********************************************************************************************************
*                                          TASK CREATION HOOK
*
* Description: This function is called when a task is created.
*
* Arguments  : p_tcb        Pointer to the task control block of the task being created.
*
* Note(s)    : None.
*********************************************************************************************************
*/

void  OSTaskCreateHook (OS_TCB  *p_tcb)
{
#if OS_CFG_APP_HOOKS_EN > 0u
    if (OS_AppTaskCreateHookPtr != (OS_APP_HOOK_TCB)0) {
        (*OS_AppTaskCreateHookPtr)(p_tcb);
    }
#else
    (void)p_tcb;                                                /* Prevent compiler warning                             */
#endif
}


/*
*********************************************************************************************************
*                                           TASK DELETION HOOK
*
* Description: This function is called when a task is deleted.
*
* Arguments  : p_tcb        Pointer to the task control block of the task being deleted.
*
* Note(s)    : None.
*********************************************************************************************************
*/

void  OSTaskDelHook (OS_TCB  *p_tcb)
{
#if OS_CFG_APP_HOOKS_EN > 0u
    if (OS_AppTaskDelHookPtr != (OS_APP_HOOK_TCB)0) {
        (*OS_AppTaskDelHookPtr)(p_tcb);
    }
#else
    (void)p_tcb;                                                /* Prevent compiler warning                             */
#endif
}


/*
*********************************************************************************************************
*                                            TASK RETURN HOOK
*
* Description: This function is called if a task accidentally returns.  In other words, a task should
*              either be an infinite loop or delete itself when done.
*
* Arguments  : p_tcb        Pointer to the task control block of the task that is returning.
*
* Note(s)    : None.
*********************************************************************************************************
*/

void  OSTaskReturnHook (OS_TCB  *p_tcb)
{
#if OS_CFG_APP_HOOKS_EN > 0u
    if (OS_AppTaskReturnHookPtr != (OS_APP_HOOK_TCB)0) {
        (*OS_AppTaskReturnHookPtr)(p_tcb);
    }
#else
    (void)p_tcb;                                                /* Prevent compiler warning                             */
#endif
}


/*
*********************************************************************************************************
*                                        INITIALIZE A TASK'S STACK
*
* Description: This function is called by either OSTaskCreate() or OSTaskCreateExt() to initialize the
*              stack frame of the task being created.  This function is highly processor specific.
*
* Arguments  : p_task       Pointer to the task entry point address.
*
*              p_arg        Pointer to a user supplied data area that will be passed to the task
*                               when the task first executes.
*
*              p_stk_base   Pointer to the base address of the stack.
*
*              stk_size     Size of the stack, in number of CPU_STK elements.
*
*              opt          Options used to alter the behavior of OS_Task_StkInit().
*                            (see OS.H for OS_TASK_OPT_xxx).
*
* Returns    : Always returns the location of the new top-of-stack once the processor registers have
*              been placed on the stack in the proper order.
*
* Note(s)    : (1) Interrupts are enabled when task starts executing.
*
*              (2) All tasks run in Thread mode, using process stack.
*
*              (3) There are two different stack frames depending on whether the Floating-Point(FP)
*                  co-processor is enabled or not.
*
*                  (a) The stack frame shown in the diagram is used when the Co-processor Access Control
*                      Register(CPACR) is disabling the Floating Point Unit. In this case, the FP
*                      registers(S0- S31) & FP Status Control(FPSCR) register are not saved in the stack frame.
*
*                  (b) The stack frame shown in the diagram is used when the Floating Point Unit is enabled,
*                      that is, CP10 and CP11 field in CPACR are ones and FPCCR sets bits ASPEN and LSPEN to 1.
*
*                      (1) When enabling the FPU through CPACR, make sure to set bits ASPEN and LSPEN in the
*                          Floating-Point Context Control Register (FPCCR).
*
*                                          +-------------+
*                                          |             |
*                                          +-------------+
*                                          |             |
*                                          +-------------+
*                                          |    FPSCR    |
*                                          +-------------+
*                                          |     S15     |
*                                          +-------------+
*                                          |     S14     |
*                                          +-------------+
*                                          |     S13     |
*                                          +-------------+
*                                                .
*                                                .
*                                                .
*                                          +-------------+
*                                          |     S2      |
*                                          +-------------+
*                                          |     S1      |
*                    +-------------+       +-------------+
*                    |             |       |     S0      |
*                    +-------------+       +-------------+
*                    |    xPSR     |       |    xPSR     |
*                    +-------------+       +-------------+
*                    | Return Addr |       | Return Addr |
*                    +-------------+       +-------------+
*                    |  LR(R14)    |       |   LR(R14)   |
*                    +-------------+       +-------------+
*                    |    R12      |       |     R12     |
*                    +-------------+       +-------------+
*                    |    R3       |       |     R3      |
*                    +-------------+       +-------------+
*                    |    R2       |       |     R0      |
*                    +-------------+       +-------------+
*                    |    R1       |       |     R1      |
*                    +-------------+       +-------------+
*                    |    R0       |       |     R0      |
*                    +-------------+       +-------------+
*                    | EXEC_RETURN |       | EXEC_RETURN |
*                    +-------------+       +-------------+
*                    |    R11      |       |     R11     |
*                    +-------------+       +-------------+
*                    |    R10      |       |     R10     |
*                    +-------------+       +-------------+
*                    |    R9       |       |     R9      |
*                    +-------------+       +-------------+
*                    |    R8       |       |     R8      |
*                    +-------------+       +-------------+
*                    |    R7       |       |     R7      |
*                    +-------------+       +-------------+
*                    |    R6       |       |     R6      |
*                    +-------------+       +-------------+
*                    |    R5       |       |     R5      |
*                    +-------------+       +-------------+
*                    |    R4       |       |     R4      |
*                    +-------------+       +-------------+
*                          (a)             |     S31     |
*                                          +-------------+
*                                          |     S30     |
*                                          +-------------+
*                                          |     S29     |
                                           +-------------+
*                                                .
*                                                .
*                                                .
*                                          +-------------+
*                                          |     S17     |
                                           +-------------+
*                                          |     S16     |
*                                          +-------------+
*                                               (b)
*
*             (4) The SP must be 8-byte aligned in conforming to the Procedure Call Standard for the ARM architecture
*
*                    (a) Section 2.1 of the  ABI for the ARM Architecture Advisory Note. SP must be 8-byte aligned
*                        on entry to AAPCS-Conforming functions states :
*
*                        The Procedure Call Standard for the ARM Architecture [AAPCS] requires primitive
*                        data types to be naturally aligned according to their sizes (for size = 1, 2, 4, 8 bytes).
*                        Doing otherwise creates more problems than it solves.
*
*                        In return for preserving the natural alignment of data, conforming code is permitted
*                        to rely on that alignment. To support aligning data allocated on the stack, the stack
*                        pointer (SP) is required to be 8-byte aligned on entry to a conforming function. In
*                        practice this requirement is met if:
*
*                           (1) At each call site, the current size of the calling function's stack frame is a multiple of 8 bytes.
*                               This places an obligation on compilers and assembly language programmers.
*
*                           (2) SP is a multiple of 8 when control first enters a program.
*                               This places an obligation on authors of low level OS, RTOS, and runtime library
*                               code to align SP at all points at which control first enters
*                               a body of (AAPCS-conforming) code.
*
*                       In turn, this requires the value of SP to be aligned to 0 modulo 8:
*
*                           (3) By exception handlers, before calling AAPCS-conforming code.
*
*                           (4) By OS/RTOS/run-time system code, before giving control to an application.
*
*                 (b) Section 2.3.1 corrective steps from the the SP must be 8-byte aligned on entry
*                     to AAPCS-conforming functions advisory note also states.
*
*                     " This requirement extends to operating systems and run-time code for all architecture versions
*                       prior to ARMV7 and to the A, R and M architecture profiles thereafter. Special considerations
*                       associated with ARMV7M are discussed in section 2.3.3"
*
*                     (1) Even if the SP 8-byte aligment is not a requirement for the ARMv7M profile, the stack is aligned
*                         to 8-byte boundaries to support legacy execution enviroments.
*
*                 (c) Section 5.2.1.2 from the Procedure Call Standard for the ARM
*                     architecture states :  "The stack must also conform to the following
*                     constraint at a public interface:
*
*                     (1) SP mod 8 = 0. The stack must be double-word aligned"
*
*                 (d) From the ARM Technical Support Knowledge Base. 8 Byte stack aligment.
*
*                     "8 byte stack alignment is a requirement of the ARM Architecture Procedure
*                      Call Standard [AAPCS]. This specifies that functions must maintain an 8 byte
*                      aligned stack address (e.g. 0x00, 0x08, 0x10, 0x18, 0x20) on all external
*                      interfaces. In practice this requirement is met if:
*
*                      (1) At each external interface, the current stack pointer
*                          is a multiple of 8 bytes.
*
*                      (2) Your OS maintains 8 byte stack alignment on its external interfaces
*                          e.g. on task switches"
*
*              (5) Exception Return Behavior(EXEC_RETURN)
*                  0xFFFFFFFD      Return to Thread mode, exception return uses non-floating point state
*                                  from the PSP and execution uses PSP after return.
*
*                  0xFFFFFFED      Return to Thread mode, exception return uses floating point state
*                                  from the PSP and execution uses PSP after return.
**********************************************************************************************************
*/

CPU_STK  *OSTaskStkInit (OS_TASK_PTR    p_task,
                         void          *p_arg,
                         CPU_STK       *p_stk_base,
                         CPU_STK       *p_stk_limit,
                         CPU_STK_SIZE   stk_size,
                         OS_OPT         opt)
{
    CPU_STK    *p_stk;


    (void)opt;                                                  /* 'opt' is not used, prevent warning                   */

    p_stk = &p_stk_base[stk_size];                              /* Load stack pointer                                   */
                                                                /* Align the stack to 8-bytes.                          */
    p_stk = (CPU_STK *)((CPU_STK)(p_stk) & 0xFFFFFFF8u);
                                                                /* Registers stacked as if auto-saved on exception      */
#if (OS_CPU_ARM_FP_EN > 0u)                                     /* FPU auto-saved registers.                            */
     --p_stk;
    *(--p_stk) = (CPU_STK)0x02000000u;                          /* FPSCR                                                */
                                                                /* Initialize S0-S15 floating point registers           */
    *(--p_stk) = (CPU_STK)0x41700000u;                          /* S15                                                  */
    *(--p_stk) = (CPU_STK)0x41600000u;                          /* S14                                                  */
    *(--p_stk) = (CPU_STK)0x41500000u;                          /* S13                                                  */
    *(--p_stk) = (CPU_STK)0x41400000u;                          /* S12                                                  */
    *(--p_stk) = (CPU_STK)0x41300000u;                          /* S11                                                  */
    *(--p_stk) = (CPU_STK)0x41200000u;                          /* S10                                                  */
    *(--p_stk) = (CPU_STK)0x41100000u;                          /* S9                                                   */
    *(--p_stk) = (CPU_STK)0x41000000u;                          /* S8                                                   */
    *(--p_stk) = (CPU_STK)0x40E00000u;                          /* S7                                                   */
    *(--p_stk) = (CPU_STK)0x40C00000u;                          /* S6                                                   */
    *(--p_stk) = (CPU_STK)0x40A00000u;                          /* S5                                                   */
    *(--p_stk) = (CPU_STK)0x40800000u;                          /* S4                                                   */
    *(--p_stk) = (CPU_STK)0x40400000u;                          /* S3                                                   */
    *(--p_stk) = (CPU_STK)0x40000000u;                          /* S2                                                   */
    *(--p_stk) = (CPU_STK)0x3F800000u;                          /* S1                                                   */
    *(--p_stk) = (CPU_STK)0x00000000u;                          /* S0                                                   */
#endif
    *(--p_stk) = (CPU_STK)0x01000000u;                          /* xPSR                                                 */
    *(--p_stk) = (CPU_STK)p_task;                               /* Entry Point                                          */
    *(--p_stk) = (CPU_STK)OS_TaskReturn;                        /* R14 (LR)                                             */
    *(--p_stk) = (CPU_STK)0x12121212u;                          /* R12                                                  */
    *(--p_stk) = (CPU_STK)0x03030303u;                          /* R3                                                   */
    *(--p_stk) = (CPU_STK)0x02020202u;                          /* R2                                                   */
    *(--p_stk) = (CPU_STK)p_stk_limit;                          /* R1                                                   */
    *(--p_stk) = (CPU_STK)p_arg;                                /* R0 : argument                                        */

#if (OS_CPU_ARM_FP_EN > 0u)
    *(--p_stk) = (CPU_STK)0xFFFFFFEDuL;                         /* R14: EXEC_RETURN; See Note 5                         */
#else
    *(--p_stk) = (CPU_STK)0xFFFFFFFDuL;                         /* R14: EXEC_RETURN; See Note 5                         */
#endif
                                                                /* Remaining registers saved on process stack           */
    *(--p_stk) = (CPU_STK)0x11111111uL;                         /* R11                                                  */
    *(--p_stk) = (CPU_STK)0x10101010uL;                         /* R10                                                  */
    *(--p_stk) = (CPU_STK)0x09090909uL;                         /* R9                                                   */
    *(--p_stk) = (CPU_STK)0x08080808uL;                         /* R8                                                   */
    *(--p_stk) = (CPU_STK)0x07070707uL;                         /* R7                                                   */
    *(--p_stk) = (CPU_STK)0x06060606uL;                         /* R6                                                   */
    *(--p_stk) = (CPU_STK)0x05050505uL;                         /* R5                                                   */
    *(--p_stk) = (CPU_STK)0x04040404uL;                         /* R4                                                   */

#if (OS_CPU_ARM_FP_EN > 0u)
                                                                /* Initialize S16-S31 floating point registers          */
    *(--p_stk) = (CPU_STK)0x41F80000u;                          /* S31                                                  */
    *(--p_stk) = (CPU_STK)0x41F00000u;                          /* S30                                                  */
    *(--p_stk) = (CPU_STK)0x41E80000u;                          /* S29                                                  */
    *(--p_stk) = (CPU_STK)0x41E00000u;                          /* S28                                                  */
    *(--p_stk) = (CPU_STK)0x41D80000u;                          /* S27                                                  */
    *(--p_stk) = (CPU_STK)0x41D00000u;                          /* S26                                                  */
    *(--p_stk) = (CPU_STK)0x41C80000u;                          /* S25                                                  */
    *(--p_stk) = (CPU_STK)0x41C00000u;                          /* S24                                                  */
    *(--p_stk) = (CPU_STK)0x41B80000u;                          /* S23                                                  */
    *(--p_stk) = (CPU_STK)0x41B00000u;                          /* S22                                                  */
    *(--p_stk) = (CPU_STK)0x41A80000u;                          /* S21                                                  */
    *(--p_stk) = (CPU_STK)0x41A00000u;                          /* S20                                                  */
    *(--p_stk) = (CPU_STK)0x41980000u;                          /* S19                                                  */
    *(--p_stk) = (CPU_STK)0x41900000u;                          /* S18                                                  */
    *(--p_stk) = (CPU_STK)0x41880000u;                          /* S17                                                  */
    *(--p_stk) = (CPU_STK)0x41800000u;                          /* S16                                                  */
#endif

    return (p_stk);
}


/*
*********************************************************************************************************
*                                           TASK SWITCH HOOK
*
* Description: This function is called when a task switch is performed.  This allows you to perform other
*              operations during a context switch.
*
* Arguments  : None.
*
* Note(s)    : 1) Interrupts are disabled during this call.
*              2) It is assumed that the global pointer 'OSTCBHighRdyPtr' points to the TCB of the task
*                 that will be 'switched in' (i.e. the highest priority task) and, 'OSTCBCurPtr' points
*                 to the task being switched out (i.e. the preempted task).
*********************************************************************************************************
*/

void  OSTaskSwHook (void)
{
#if OS_CFG_TASK_PROFILE_EN > 0u
    CPU_TS  ts;
#endif
#ifdef  CPU_CFG_INT_DIS_MEAS_EN
    CPU_TS  int_dis_time;
#endif
#if (OS_CFG_TASK_STK_REDZONE_EN > 0u)
    CPU_BOOLEAN  stk_status;
#endif

#if (OS_CPU_ARM_FP_EN > 0u)
    OS_CPU_FP_Reg_Push(OSTCBCurPtr->StkPtr);                    /* Push the FP registers of the current task.           */
#endif

#if OS_CFG_APP_HOOKS_EN > 0u
    if (OS_AppTaskSwHookPtr != (OS_APP_HOOK_VOID)0) {
        (*OS_AppTaskSwHookPtr)();
    }
#endif

    OS_TRACE_TASK_SWITCHED_IN(OSTCBHighRdyPtr);

#if OS_CFG_TASK_PROFILE_EN > 0u
    ts = OS_TS_GET();
    if (OSTCBCurPtr != OSTCBHighRdyPtr) {
        OSTCBCurPtr->CyclesDelta  = ts - OSTCBCurPtr->CyclesStart;
        OSTCBCurPtr->CyclesTotal += (OS_CYCLES)OSTCBCurPtr->CyclesDelta;
    }

    OSTCBHighRdyPtr->CyclesStart = ts;
#endif

#ifdef  CPU_CFG_INT_DIS_MEAS_EN
    int_dis_time = CPU_IntDisMeasMaxCurReset();                 /* Keep track of per-task interrupt disable time        */
    if (OSTCBCurPtr->IntDisTimeMax < int_dis_time) {
        OSTCBCurPtr->IntDisTimeMax = int_dis_time;
    }
#endif

#if OS_CFG_SCHED_LOCK_TIME_MEAS_EN > 0u
                                                                /* Keep track of per-task scheduler lock time           */
    if (OSTCBCurPtr->SchedLockTimeMax < OSSchedLockTimeMaxCur) {
        OSTCBCurPtr->SchedLockTimeMax = OSSchedLockTimeMaxCur;
    }
    OSSchedLockTimeMaxCur = (CPU_TS)0;                          /* Reset the per-task value                             */
#endif

#if (OS_CFG_TASK_STK_REDZONE_EN > 0u)
                                                                /* Check if stack overflowed.                           */
    stk_status = OSTaskStkRedzoneChk((OS_TCB *)0u);
    if (stk_status != OS_TRUE) {
        OSRedzoneHitHook(OSTCBCurPtr);
    }
#endif

#if (OS_CPU_ARM_FP_EN > 0u)
    OS_CPU_FP_Reg_Pop(OSTCBHighRdyPtr->StkPtr);                 /* Pop the FP registers of the highest ready task.      */
#endif
}


/*
*********************************************************************************************************
*                                              TICK HOOK
*
* Description: This function is called every tick.
*
* Arguments  : None.
*
* Note(s)    : 1) This function is assumed to be called from the Tick ISR.
*********************************************************************************************************
*/

void  OSTimeTickHook (void)
{
#if OS_CFG_APP_HOOKS_EN > 0u
    if (OS_AppTimeTickHookPtr != (OS_APP_HOOK_VOID)0) {
        (*OS_AppTimeTickHookPtr)();
    }
#endif
}


/*
*********************************************************************************************************
*                                          SYS TICK HANDLER
*
* Description: Handle the system tick (SysTick) interrupt, which is used to generate the uC/OS-III tick
*              interrupt.
*
* Arguments  : None.
*
* Note(s)    : 1) This function MUST be placed on entry 15 of the Cortex-M vector table.
*********************************************************************************************************
*/

void  OS_CPU_SysTickHandler  (void)
{
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
    OSIntEnter();                                               /* Tell uC/OS-III that we are starting an ISR           */
    CPU_CRITICAL_EXIT();

    OSTimeTick();                                               /* Call uC/OS-III's OSTimeTick()                        */

    OSIntExit();                                                /* Tell uC/OS-III that we are leaving the ISR           */
}


/*
*********************************************************************************************************
*                                         INITIALIZE SYS TICK
*
* Description: Initialize the SysTick using the CPU clock frequency.
*
* Arguments  : cpu_freq         CPU clock frequency.
*
* Note(s)    : 1) This function MUST be called after OSStart() & after processor initialization.
*
*              2) Either OS_CPU_SysTickInitFreq or OS_CPU_SysTickInit() can be called.
*********************************************************************************************************
*/

void  OS_CPU_SysTickInitFreq (CPU_INT32U  cpu_freq)
{
#if (OS_CFG_TICK_EN > 0u)
    CPU_INT32U  cnts;


    cnts = (cpu_freq / (CPU_INT32U)OSCfg_TickRate_Hz);          /* Determine nbr SysTick cnts between two OS tick intr. */

    OS_CPU_SysTickInit(cnts);
#else
    (void)cpu_freq;
#endif
}


/*
*********************************************************************************************************
*                                         INITIALIZE SYS TICK
*
* Description: Initialize the SysTick using the number of countes between two ticks.
*
* Arguments  : cnts         Number of SysTick counts between two OS tick interrupts.
*
* Note(s)    : 1) This function MUST be called after OSStart() & after processor initialization.
*
*              2) Either OS_CPU_SysTickInitFreq or OS_CPU_SysTickInit() can be called.
*********************************************************************************************************
*/

void  OS_CPU_SysTickInit (CPU_INT32U  cnts)
{
#if (OS_CFG_TICK_EN > 0u)
    CPU_INT32U  prio;
    CPU_INT32U  basepri;


                                                                /* Set BASEPRI boundary from the configuration.         */
    basepri             = (CPU_INT32U)(CPU_CFG_KA_IPL_BOUNDARY << (8u - CPU_CFG_NVIC_PRIO_BITS));
    CPU_REG_SYST_RVR    = cnts - 1u;                            /* Set Reload Register                                  */

                                                                /* Set SysTick handler prio.                            */
    prio                = CPU_REG_SCB_SHPRI3;
    prio               &= 0x00FFFFFFu;
    prio               |= (basepri << 24u);

    CPU_REG_SCB_SHPRI3  = prio;

                                                                /* Enable timer.                                        */
    CPU_REG_SYST_CSR   |= CPU_REG_SYST_CSR_CLKSOURCE |
                          CPU_REG_SYST_CSR_ENABLE;

    CPU_REG_SYST_CSR   |= CPU_REG_SYST_CSR_TICKINT;             /* Enable timer interrupt.                              */
#else
    (void)cnts;
#endif
}

#ifdef __cplusplus
}
#endif
