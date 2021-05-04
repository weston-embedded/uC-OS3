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
*                                         Generic ColdFire Port
*
* File      : os_cpu_c.c
* Version   : V3.08.01
*********************************************************************************************************
* Toolchain : IAR EWCF
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
*
*              2) The initial value of the Status Register (SR) OS_INITIAL_SR sets the CPU to run in
*                 SUPERVISOR mode. It is assumed that all uC/OS-III tasks run in supervisor mode.
**********************************************************************************************************
*/

CPU_STK *OSTaskStkInit (OS_TASK_PTR   p_task,
                        void         *p_arg,
                        CPU_STK      *p_stk_base,
                        CPU_STK      *p_stk_limit,
                        CPU_STK_SIZE  stk_size,
                        OS_OPT        opt)
{
    CPU_STK  *p_stk;


    (void)opt;                                              /* 'opt' is not used, prevent compiler warning            */

    switch (p_stk_base[stk_size - 1u] & 0x00000003) {       /* Align the stack on a longword boundary                 */
        case 0:
             p_stk = &p_stk_base[stk_size - 1u] + 0;
             break;

        case 1:
             p_stk = &p_stk_base[stk_size - 1u] - 1;
             break;

        case 2:
             p_stk = &p_stk_base[stk_size - 1u] - 2;
             break;

        case 3:
             p_stk = &p_stk_base[stk_size - 1u] - 3;
             break;
    }

    *p_stk = 0;                                             /* ------- SIMULATE CALL TO FUNCTION WITH ARGUMENT ------ */
    *--p_stk = (CPU_INT32U)p_arg;                           /*    p_arg                                               */
    *--p_stk = (CPU_INT32U)OS_TaskReturn;                   /*    Task return address                                 */


                                                            /* ----------- SIMULATE INTERRUPT STACK FRAME ----------- */
    *--p_stk = (CPU_INT32U)p_task;                          /*    Task return address                                 */
    *--p_stk = (CPU_INT32U)(0x40000000 | OS_INITIAL_SR);    /*    format and status register                          */


                                                            /* ------------ SAVE ALL PROCESSOR REGISTERS ------------ */
    *--p_stk = (CPU_INT32U)0x00A600A6L;                     /* Register A6                                            */
    *--p_stk = (CPU_INT32U)0x00A500A5L;                     /* Register A5                                            */
    *--p_stk = (CPU_INT32U)0x00A400A4L;                     /* Register A4                                            */
    *--p_stk = (CPU_INT32U)0x00A300A3L;                     /* Register A3                                            */
    *--p_stk = (CPU_INT32U)0x00A200A2L;                     /* Register A2                                            */
    *--p_stk = (CPU_INT32U)0x00A100A1L;                     /* Register A1                                            */
    *--p_stk = (CPU_INT32U)p_arg;                           /* Register A0                                            */
    *--p_stk = (CPU_INT32U)0x00D700D7L;                     /* Register D7                                            */
    *--p_stk = (CPU_INT32U)0x00D600D6L;                     /* Register D6                                            */
    *--p_stk = (CPU_INT32U)0x00D500D5L;                     /* Register D5                                            */
    *--p_stk = (CPU_INT32U)0x00D400D4L;                     /* Register D4                                            */
    *--p_stk = (CPU_INT32U)0x00D300D3L;                     /* Register D3                                            */
    *--p_stk = (CPU_INT32U)0x00D200D2L;                     /* Register D2                                            */
    *--p_stk = (CPU_INT32U)p_stk_limit;                     /* Register D1                                            */
    *--p_stk = (CPU_INT32U)p_arg;                           /* Register D0                                            */
    return (p_stk);                                         /* Return pointer to new top-of-stack                     */
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
