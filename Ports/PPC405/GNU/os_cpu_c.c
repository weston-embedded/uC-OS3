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
*                                              PowerPC 405
*                                          GNU C/C++ Compiler
*
* File    : os_cpu_c.c
* Version : V3.08.02
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
*                                            LOCAL VARIABLES
*********************************************************************************************************
*/

extern  void  *_SDA_BASE_;
extern  void  *_SDA2_BASE_;


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
* Note(s)    : 1) Interrupts should be disabled during this call.
*********************************************************************************************************
*/

void  OSInitHook (void)
{
}


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
* Note(s)    : 1) Interrupts are disabled during this call.
*********************************************************************************************************
*/

void  OSTaskCreateHook (OS_TCB  *p_tcb)
{
#if OS_CFG_APP_HOOKS_EN > 0u
    if (OS_AppTaskCreateHookPtr != (OS_APP_HOOK_TCB)0) {
        (*OS_AppTaskCreateHookPtr)(p_tcb);
    }
#else
    (void)p_tcb;                                            /* Prevent compiler warning                               */
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
* Note(s)    : 1) Interrupts are disabled during this call.
*********************************************************************************************************
*/

void  OSTaskDelHook (OS_TCB  *p_tcb)
{
#if OS_CFG_APP_HOOKS_EN > 0u
    if (OS_AppTaskDelHookPtr != (OS_APP_HOOK_TCB)0) {
        (*OS_AppTaskDelHookPtr)(p_tcb);
    }
#else
    (void)p_tcb;                                            /* Prevent compiler warning                               */
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
    (void)p_tcb;                                            /* Prevent compiler warning                               */
#endif
}


/*
**********************************************************************************************************
*                                       INITIALIZE A TASK'S STACK
*
* Description: This function is called by OS_Task_Create() or OSTaskCreateExt() to initialize the stack
*              frame of the task being created. This function is highly processor specific.
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
* Returns    : Always returns the location of the new top-of-stack' once the processor registers have
*              been placed on the stack in the proper order.
*
* Note(s)    : 1) Interrupts are enabled when task starts executing.
*
*                 OSTCBHighRdyPtr->OSTCBStkPtr + 0x00     RMSR   (IE=1)       (LOW Memory)
*                                              + 0x04     R2
*                                              + 0x08     R3
*                                              + 0x0C     R4
*                                              + 0x10     R5     (p_arg passed to task)
*                                              + 0x14     R6
*                                              + 0x18     R7
*                                              + 0x1C     R8
*                                              + 0x20     R9
*                                              + 0x24     R10
*                                              + 0x28     R11
*                                              + 0x2C     R12
*                                              + 0x30     R13
*                                              + 0x34     R14
*                                              + 0x38     R15
*                                              + 0x3C     R17
*                                              + 0x40     R18
*                                              + 0x44     R19
*                                              + 0x48     R20
*                                              + 0x4C     R21
*                                              + 0x50     R22
*                                              + 0x54     R23
*                                              + 0x58     R24
*                                              + 0x5C     R25
*                                              + 0x60     R26
*                                              + 0x64     R27
*                                              + 0x68     R28
*                                              + 0x6C     R29
*                                              + 0x70     R30
*                                              + 0x74     R31                (HIGH Memory)
*                                              + 0x78     Empty
*                 ptos --------->              + 0x7C     Empty
*
*              2) R16 is not saved as part of the task context since it is used by the debugger.
**********************************************************************************************************
*/

CPU_STK  *OSTaskStkInit (OS_TASK_PTR    p_task,
                         void          *p_arg,
                         CPU_STK       *p_stk_base,
                         CPU_STK       *p_stk_limit,
                         CPU_STK_SIZE   stk_size,
                         OS_OPT         opt)
{
    CPU_STK     *pstk;
    CPU_INT32U   msr;


    (void)p_stk_limit;                                      /* Prevent compiler warning                               */
    (void)opt;

    __asm__ __volatile__("mfmsr %0\n" : "=r" (msr));        /* Obtain the current value of the MSR                    */
    msr |= 0x00028000;                                      /* Interrupts will be enabled when the task is started    */

    pstk    = (CPU_STK *)(p_stk_base + stk_size);           /* Load stack pointer                                     */
    pstk--;                                                 /* Make sure we point to free entry ...                   */
    pstk--;                                                 /* ... compiler uses top-of-stack so free an extra one.   */
    *pstk-- = (CPU_INT32U)0x31313131;                       /* r31                                                    */
    *pstk-- = (CPU_INT32U)0x30303030;                       /* r30                                                    */
    *pstk-- = (CPU_INT32U)0x29292929;                       /* r29                                                    */
    *pstk-- = (CPU_INT32U)0x28282828;                       /* r28                                                    */
    *pstk-- = (CPU_INT32U)0x27272727;                       /* r27                                                    */
    *pstk-- = (CPU_INT32U)0x26262626;                       /* r26                                                    */
    *pstk-- = (CPU_INT32U)0x25252525;                       /* r25                                                    */
    *pstk-- = (CPU_INT32U)0x24242424;                       /* r24                                                    */
    *pstk-- = (CPU_INT32U)0x23232323;                       /* r23                                                    */
    *pstk-- = (CPU_INT32U)0x22222222;                       /* r22                                                    */
    *pstk-- = (CPU_INT32U)0x21212121;                       /* r21                                                    */
    *pstk-- = (CPU_INT32U)0x20202020;                       /* r20                                                    */
    *pstk-- = (CPU_INT32U)0x19191919;                       /* r19                                                    */
    *pstk-- = (CPU_INT32U)0x18181818;                       /* r18                                                    */
    *pstk-- = (CPU_INT32U)0x17171717;                       /* r17                                                    */
    *pstk-- = (CPU_INT32U)0x16161616;                       /* r16                                                    */
    *pstk-- = (CPU_INT32U)0x15151515;                       /* r15                                                    */
    *pstk-- = (CPU_INT32U)0x14141414;                       /* r14                                                    */
    *pstk-- = (CPU_INT32U)_SDA_BASE_;                       /* r13                                                    */
    *pstk-- = (CPU_INT32U)0x12121212;                       /* r12                                                    */
    *pstk-- = (CPU_INT32U)0x11111111;                       /* r11                                                    */
    *pstk-- = (CPU_INT32U)0x10101010;                       /* r10                                                    */
    *pstk-- = (CPU_INT32U)0x09090909;                       /* r09                                                    */
    *pstk-- = (CPU_INT32U)0x08080808;                       /* r08                                                    */
    *pstk-- = (CPU_INT32U)0x07070707;                       /* r07                                                    */
    *pstk-- = (CPU_INT32U)0x06060606;                       /* r06                                                    */
    *pstk-- = (CPU_INT32U)0x05050505;                       /* r05                                                    */
    *pstk-- = (CPU_INT32U)0x04040404;                       /* r04                                                    */
    *pstk-- = (CPU_INT32U)p_arg;                            /* r03                                                    */
    *pstk-- = (CPU_INT32U)_SDA2_BASE_;                      /* r02                                                    */
    *pstk-- = (CPU_INT32U)0x00000000;                       /* r00                                                    */
    *pstk-- = (CPU_INT32U)0x0F0F0F0F;                       /* User SPR General-Purpose Register 0                    */
    *pstk-- = (CPU_INT32U)0x0F0F0F0F;                       /* Condition Register                                     */
    *pstk-- = (CPU_INT32U)0x0F0F0F0F;                       /* Fixed-Point Exception Register                         */
    *pstk-- = (CPU_INT32U)0x0F0F0F0F;                       /* Count Register                                         */
    *pstk-- = (CPU_INT32U)0x0F0F0F0F;                       /* Link Register                                          */
    *pstk-- = (CPU_INT32U)p_task;                           /* Return address                                         */
    *pstk-- = msr;                                          /* Machine-State Register; interrupts are enabled         */
    pstk--;

    return (pstk);                                          /* Return new top of stack                                */
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



#if OS_CFG_APP_HOOKS_EN > 0u
    if (OS_AppTaskSwHookPtr != (OS_APP_HOOK_VOID)0) {
        (*OS_AppTaskSwHookPtr)();
    }
#endif

#if OS_CFG_TASK_PROFILE_EN > 0u
    ts = OS_TS_GET();
    if (OSTCBCurPtr != OSTCBHighRdyPtr) {
        OSTCBCurPtr->CyclesDelta  = ts - OSTCBCurPtr->CyclesStart;
        OSTCBCurPtr->CyclesTotal += (OS_CYCLES)OSTCBCurPtr->CyclesDelta;
    }

    OSTCBHighRdyPtr->CyclesStart = ts;
#endif

#ifdef  CPU_CFG_INT_DIS_MEAS_EN
    int_dis_time = CPU_IntDisMeasMaxCurReset();             /* Keep track of per-task interrupt disable time          */
    if (OSTCBCurPtr->IntDisTimeMax < int_dis_time) {
        OSTCBCurPtr->IntDisTimeMax = int_dis_time;
    }
#endif

#if OS_CFG_SCHED_LOCK_TIME_MEAS_EN > 0u
                                                            /* Keep track of per-task scheduler lock time             */
    if (OSTCBCurPtr->SchedLockTimeMax < OSSchedLockTimeMaxCur) {
        OSTCBCurPtr->SchedLockTimeMax = OSSchedLockTimeMaxCur;
    }
    OSSchedLockTimeMaxCur = (CPU_TS)0;                      /* Reset the per-task value                               */
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

#ifdef __cplusplus
}
#endif
