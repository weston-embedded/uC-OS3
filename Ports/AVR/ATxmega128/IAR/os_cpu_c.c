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
*                                         ATMEL AVR Xmega Port
*
* File      : os_cpu_c.c
* Version   : V3.08.02
*********************************************************************************************************
* Toolchain : IAR AVR32
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

#include  "../../../../Source/os.h"

#ifdef __cplusplus
extern  "C" {
#endif

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
* Note(s)    : None.
*********************************************************************************************************
*/

void  OSInitHook (void)
{
    CPU_STK_SIZE   i;
    CPU_STK       *p_stk;


    p_stk = OSCfg_ISRStkBasePtr;                            /* Clear the ISR stack                                    */
    for (i = 0u; i < OSCfg_ISRStkSize; i++) {
        *p_stk++ = (CPU_STK)0u;
    }
    OS_CPU_ExceptStkBase = (CPU_STK *)(OSCfg_ISRStkBasePtr + OSCfg_ISRStkSize - 1u);
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
**********************************************************************************************************
*/

CPU_STK  *OSTaskStkInit (OS_TASK_PTR    p_task,
                         void          *p_arg,
                         CPU_STK       *p_stk_base,
                         CPU_STK       *p_stk_limit,
                         CPU_STK_SIZE   stk_size,
                         OS_OPT         opt)
{
    CPU_STK     *psoft_stk;
    CPU_STK     *phard_stk;                                 /* Setup AVR hardware stack.                              */
    CPU_INT32U   tmp;


    (void)opt;                                              /* Prevent compiler warning                               */
    (void)p_stk_limit;

    psoft_stk     = (CPU_STK *)&p_stk_base[stk_size - 1u];
    phard_stk     = (CPU_STK *)&p_stk_base[stk_size - 1u]
                  - OSTaskStkSize                           /* Task stack size                                          */
                  + OSTaskStkSizeHard;                      /* AVR return stack ("hardware stack")                      */
    tmp           = (CPU_INT32U)p_task;
    *phard_stk--  = (CPU_STK)(tmp & 0xFF);                  /* Put task start address on top of "hardware stack"        */
    tmp         >>= 8;
    *phard_stk--  = (CPU_STK)(tmp & 0xFF);
    tmp         >>= 8;
    *phard_stk--  = (CPU_STK)(tmp & 0xFF);

    *psoft_stk--  = (CPU_STK)0x00;                          /* R0    = 0x00                                           */
    *psoft_stk--  = (CPU_STK)0x01;                          /* R1    = 0x01                                           */
    *psoft_stk--  = (CPU_STK)0x02;                          /* R2    = 0x02                                           */
    *psoft_stk--  = (CPU_STK)0x03;                          /* R3    = 0x03                                           */
    *psoft_stk--  = (CPU_STK)0x00;                          /* R4      depending on "Register Utilization" settings,  */
    *psoft_stk--  = (CPU_STK)0x00;                          /* R5      R4 to R15 may be locked to be used for global  */
    *psoft_stk--  = (CPU_STK)0x00;                          /* R6      variables                                      */
    *psoft_stk--  = (CPU_STK)0x00;                          /* R7                                                     */
    *psoft_stk--  = (CPU_STK)0x00;                          /* R8                                                     */
    *psoft_stk--  = (CPU_STK)0x00;                          /* R9                                                     */
    *psoft_stk--  = (CPU_STK)0x00;                          /* R10                                                    */
    *psoft_stk--  = (CPU_STK)0x00;                          /* R11                                                    */
    *psoft_stk--  = (CPU_STK)0x00;                          /* R12                                                    */
    *psoft_stk--  = (CPU_STK)0x00;                          /* R13                                                    */
    *psoft_stk--  = (CPU_STK)0x00;                          /* R14                                                    */
    *psoft_stk--  = (CPU_STK)0x00;                          /* R15                                                    */
    tmp           = (CPU_INT16U)p_arg;
    *psoft_stk--  = (CPU_STK)tmp;                           /* 'p_arg' passed in R17:R16                              */
    *psoft_stk--  = (CPU_STK)(tmp >> 8);
    *psoft_stk--  = (CPU_STK)0x18;                          /* R18   = 0x18                                           */
    *psoft_stk--  = (CPU_STK)0x19;                          /* R19   = 0x19                                           */
    *psoft_stk--  = (CPU_STK)0x20;                          /* R20   = 0x20                                           */
    *psoft_stk--  = (CPU_STK)0x21;                          /* R21   = 0x21                                           */
    *psoft_stk--  = (CPU_STK)0x22;                          /* R22   = 0x22                                           */
    *psoft_stk--  = (CPU_STK)0x23;                          /* R23   = 0x23                                           */
    *psoft_stk--  = (CPU_STK)0x24;                          /* R24   = 0x24                                           */
    *psoft_stk--  = (CPU_STK)0x25;                          /* R25   = 0x25                                           */
    *psoft_stk--  = (CPU_STK)0x26;                          /* R26   = 0x26                                           */
    *psoft_stk--  = (CPU_STK)0x27;                          /* R27   = 0x27                                           */
                                                            /* R28     R29:R28 is the software stack which gets ...   */
                                                            /* R29             ... in the TCB.                        */
    *psoft_stk--  = (CPU_STK)0x30;                          /* R30   = 0x30                                           */
    *psoft_stk--  = (CPU_STK)0x31;                          /* R31   = 0x31                                           */
    *psoft_stk--  = (CPU_STK)0x00;                          /* RAMPD = 0x00                                           */
    *psoft_stk--  = (CPU_STK)0x00;                          /* RAMPX = 0x00                                           */
    *psoft_stk--  = (CPU_STK)0x00;                          /* RAMPZ = 0x00                                           */
    *psoft_stk--  = (CPU_STK)0x00;                          /* EIND  = 0x00                                           */
    *psoft_stk--  = (CPU_STK)0x80;                          /* SREG  = Interrupts enabled                             */
    tmp           = (CPU_INT16U)phard_stk;
    *psoft_stk--  = (CPU_STK)(tmp >> 8);                    /* SPH                                                    */
    *psoft_stk    = (CPU_STK) tmp;                          /* SPL                                                    */

    return (psoft_stk);
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
