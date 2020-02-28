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
*                           Renesas SH2A-FPU Specific code (RESBANK version)
*                                   Renesas SH SERIES C/C++ Compiler
*
* File    : os_cpu_c.c
* Version : V3.08.00
*********************************************************************************************************
*/

#define  OS_CPU_GLOBALS
#include "../../../../Source/os.h"


#ifdef __cplusplus
extern  "C" {
#endif


/*
************************************************************************************************************************
*                                                      #defines
************************************************************************************************************************
*/

#define INT_MASK_ON  0x00000000L
#define INT_MASK_OFF 0x000000C0L

/*
************************************************************************************************************************
*                                                   IDLE TASK HOOK
*
* Description: This function is called by the idle task.  This hook has been added to allow you to do such things as
*              STOP the CPU to conserve power.
*
* Arguments  : none
*
* Note(s)    : none
************************************************************************************************************************
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
************************************************************************************************************************
*                                                OS INITIALIZATION HOOK
*
* Description: This function is called by OSInit() towards the beginning of OSInit().
*
* Arguments  : none
*
* Note(s)    : none
************************************************************************************************************************
*/

void  OSInitHook (void)
{
}

/*
************************************************************************************************************************
*                                                 STATISTIC TASK HOOK
*
* Description: This function is called every second by uC/OS-III's statistics task.  This allows your application to add
*              functionality to the statistics task.
*
* Arguments  : none
*
* Note(s)    : none
************************************************************************************************************************
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
************************************************************************************************************************
*                                                  TASK CREATION HOOK
*
* Description: This function is called when a task is created.
*
* Arguments  : p_tcb   is a pointer to the task control block of the task being created.
*
* Note(s)    : none
************************************************************************************************************************
*/

void  OSTaskCreateHook (OS_TCB *p_tcb)
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
************************************************************************************************************************
*                                                   TASK DELETION HOOK
*
* Description: This function is called when a task is deleted.
*
* Arguments  : p_tcb   is a pointer to the task control block of the task being deleted.
*
* Note(s)    : none
************************************************************************************************************************
*/

void  OSTaskDelHook (OS_TCB *p_tcb)
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
************************************************************************************************************************
*                                                   TASK RETURN HOOK
*
* Description: This function is called if a task accidentally returns.  In other words, a task should either be an
*              infinite loop or delete itself when done.
*
* Arguments  : p_tcb   is a pointer to the task control block of the task that is returning.
*
* Note(s)    : none
************************************************************************************************************************
*/

void  OSTaskReturnHook (OS_TCB  *p_tcb)
{
#if OS_CFG_APP_HOOKS_EN > 0u
    if (OS_AppTaskReturnHookPtr != (OS_APP_HOOK_TCB)0) {
        (*OS_AppTaskReturnHookPtr)(p_tcb);
    }
#else
    (void)p_tcb;
#endif
}

/*
************************************************************************************************************************
*                                               INITIALIZE A TASK'S STACK
*
* Description: This function is called by OS_Task_Create() to initialize the stack frame of the task being created. This
*              function is highly processor specific.
*
* Arguments  : p_task        is a pointer to the task entry point address.
*
*              p_arg         is a pointer to a user supplied data area that will be passed to the task when the task
*                            first executes.
*
*              p_stk_base    is a pointer to the base address of the stack.
*
*              p_stk_limit   is a pointer to the stack limit address.  The SH2A-FPU doesn't support automatic stack
*                            checking and thus this argument is not used.
*
*              stk_size      is the size of the stack in number of CPU_STK elements.
*
*              opt           specifies options that can be used to alter the behavior of OS_Task_StkInit().
*                            (see OS.H for OS_TASK_OPT_xxx).
*
* Returns    : Always returns the location of the new top-of-stack' once the processor registers have been placed on the
*              stack in the proper order.
*
* Note(s)    : 1) Interrupts are enabled when your task starts executing.
************************************************************************************************************************
*/

CPU_STK  *OSTaskStkInit (OS_TASK_PTR   p_task,
                         void         *p_arg,
                         CPU_STK      *p_stk_base,
                         CPU_STK      *p_stk_limit,
                         CPU_STK_SIZE  stk_size,
                         OS_OPT        opt)
{
    CPU_STK  *p_stk;


    (void)&opt;                                 /* prevent compiler warning                                           */
    (void)&p_stk_limit;

    p_stk    = &p_stk_base[stk_size];           /* load stack pointer                                                 */

    *--p_stk = (CPU_INT32U)INT_MASK_ON;         /* SR                                                                 */
    *--p_stk = (CPU_INT32U)p_task;              /* PC of task                                                         */

    *--p_stk = 0x00000000L;                     /* FR0   ( 0.00000)                                                   */
    *--p_stk = 0x3F800000L;                     /* FR1   ( 1.00000)                                                   */
    *--p_stk = 0x40000000L;                     /* FR2   ( 2.00000)                                                   */
    *--p_stk = 0x40400000L;                     /* FR3   ( 3.00000)                                                   */
    *--p_stk = 0x40800000L;                     /* FR4   ( 4.00000)                                                   */
    *--p_stk = 0x40A00000L;                     /* FR5   ( 5.00000)                                                   */
    *--p_stk = 0x40C00000L;                     /* FR6   ( 6.00000)                                                   */
    *--p_stk = 0x40E00000L;                     /* FR7   ( 7.00000)                                                   */
    *--p_stk = 0x41000000L;                     /* FR8   ( 8.00000)                                                   */
    *--p_stk = 0x41100000L;                     /* FR9   ( 9.00000)                                                   */
    *--p_stk = 0x41200000L;                     /* FR10  (10.00000)                                                   */
    *--p_stk = 0x41300000L;                     /* FR11  (11.00000)                                                   */
    *--p_stk = 0x00000000L;                     /* FPUL                                                               */
    *--p_stk = 0x00040001L;                     /* FPSCR                                                              */
    *--p_stk = 0x41400000L;                     /* FR12  (12.00000)                                                   */
    *--p_stk = 0x41500000L;                     /* FR13  (13.00000)                                                   */
    *--p_stk = 0x41600000L;                     /* FR14  (14.00000)                                                   */
    *--p_stk = 0x41700000L;                     /* FR15  (15.00000)                                                   */

    *--p_stk = 0x00000000L;                     /* R0                                                                 */
	*--p_stk = (CPU_INT32U)get_tbr();           /* TBR                                                                */

                                                /* Below structure is same as register bank                           */
    *--p_stk = OS_Get_GBR();                    /* GBR                                                                */
    *--p_stk = (CPU_INT32U)OS_TaskReturn;       /* PR (Return address in case task returns accidentally)              */

    *--p_stk = 0xA0A0A0A0L;                     /* MACL                                                               */
    *--p_stk = 0xA1A1A1A1L;                     /* MACH                                                               */

    *--p_stk = 0x14141414L;                     /* R14                                                                */
    *--p_stk = 0x13131313L;                     /* R13                                                                */
    *--p_stk = 0x12121212L;                     /* R12                                                                */
    *--p_stk = 0x11111111L;                     /* R11                                                                */
    *--p_stk = 0x10101010L;                     /* R10                                                                */
    *--p_stk = 0x09090909L;                     /* R9                                                                 */
    *--p_stk = 0x08080808L;                     /* R8                                                                 */
    *--p_stk = 0x07070707L;                     /* R7                                                                 */
    *--p_stk = 0x06060606L;                     /* R6                                                                 */
    *--p_stk = 0x05050505L;                     /* R5                                                                 */
    *--p_stk = (CPU_INT32U)p_arg;               /* pass p_arg in R4                                                   */
    *--p_stk = 0x03030303L;                     /* R3                                                                 */
    *--p_stk = 0x02020202L;                     /* R2                                                                 */
    *--p_stk = 0x01010101L;                     /* R1                                                                 */
    *--p_stk = 0x00000000L;                     /* R0                                                                 */

    return ((CPU_STK *)p_stk);
}

/*
************************************************************************************************************************
*                                                   TASK SWITCH HOOK
*
* Description: This function is called when a task switch is performed.  This allows you to perform other operations
*              during a context switch.
*
* Arguments  : none
*
* Note(s)    : 1) Interrupts are disabled during this call.
*              2) It is assumed that the global pointer 'OSTCBHighRdyPtr' points to the TCB of the task that will be
*                 'switched in' (i.e. the highest priority task) and, 'OSTCBCurPtr' points to the task being switched out
*                 (i.e. the preempted task).
************************************************************************************************************************
*/

void  OSTaskSwHook (void)
{
#if OS_CFG_TASK_PROFILE_EN > 0u && defined(CPU_CFG_INT_DIS_MEAS_EN)
    CPU_TS     ts;
#endif
#ifdef  CPU_CFG_INT_DIS_MEAS_EN
    CPU_TS     int_dis_time;
#endif



#if OS_CFG_APP_HOOKS_EN > 0u
    if (OS_AppTaskSwHookPtr != (OS_APP_HOOK_VOID)0) {
        (*OS_AppTaskSwHookPtr)();
    }
#endif

#if OS_CFG_TASK_PROFILE_EN > 0u && defined(CPU_CFG_INT_DIS_MEAS_EN)
    ts = OS_TS_GET();
    if (OSTCBCurPtr != OSTCBHighRdyPtr) {
        OSTCBCurPtr->CyclesDelta  = ts - OSTCBCurPtr->CyclesStart;
        OSTCBCurPtr->CyclesTotal += (OS_CYCLES)OSTCBCurPtr->CyclesDelta;
    }

    OSTCBHighRdyPtr->CyclesStart = ts;
#endif

#ifdef  CPU_CFG_INT_DIS_MEAS_EN
    int_dis_time = CPU_IntDisMeasMaxCurReset();                 /* Keep track of per-task interrupt disable time          */
    if (int_dis_time > OSTCBCurPtr->IntDisTimeMax) {
        OSTCBCurPtr->IntDisTimeMax = int_dis_time;
    }
#endif

#if OS_CFG_SCHED_LOCK_TIME_MEAS_EN > 0u
    if (OSSchedLockTimeMaxCur > OSTCBCurPtr->SchedLockTimeMax) { /* Keep track of per-task scheduler lock time        */
        OSTCBCurPtr->SchedLockTimeMax = OSSchedLockTimeMaxCur;
    }
    OSSchedLockTimeMaxCur = (CPU_TS)0;                           /* Reset the per-task value                          */
#endif
}

/*
************************************************************************************************************************
*                                                      TICK HOOK
*
* Description: This function is called every tick.
*
* Arguments  : none
*
* Note(s)    : 1) This function is assumed to be called from the Tick ISR.
************************************************************************************************************************
*/

void  OSTimeTickHook (void)
{
#if OS_CFG_APP_HOOKS_EN > 0u
    if (OS_AppTimeTickHookPtr != (OS_APP_HOOK_VOID)0) {
        (*OS_AppTimeTickHookPtr)();
    }
#endif

#if (CPU_CFG_TS_EN > 0u)
    CPU_TS_Update();
#endif
}


#ifdef __cplusplus
}
#endif
