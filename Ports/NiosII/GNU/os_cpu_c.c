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
*                                          Altera NiosII Port
*
* File      : os_cpu_c.c
* Version   : V3.08.00
*********************************************************************************************************
* For       : Altera NiosII
* Toolchain : GNU - Altera NiosII
*********************************************************************************************************
*/

#include <reent.h>
#include <string.h>

#include <stddef.h>

#define  OS_CPU_GLOBALS
#include "includes.h" /* Standard includes for uC/OS-III */
#include "../../../Source/os.h"

#include "system.h"

#ifdef __cplusplus
extern  "C" {
#endif


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
* Description: This function is called by OSInit() at the beginning of OSInit().
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
*                                                  REDZONE HIT HOOK
*
* Description: This function is called when a task's stack overflowed.
*
* Arguments  : p_tcb        Pointer to the task control block of the offending task. NULL if ISR.
*
* Note(s)    : None.
************************************************************************************************************************
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
*              p_stk_limit   is a pointer to the stack limit address. It is not used for this port.
*
*              p_stk_base    is a pointer to the base address of the stack.
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
*              2) All tasks run in Thread mode, using process stack.
************************************************************************************************************************
*/

CPU_STK     *OSTaskStkInit         (OS_TASK_PTR     p_task,
                                    void            *p_arg,
                                    CPU_STK         *p_stk_base,
                                    CPU_STK         *p_stk_limit,
                                    CPU_STK_SIZE    stk_size,
                                    OS_OPT          opt )
{
   CPU_INT32U   *frame_pointer;
   CPU_INT32U   *stk;

#if (OS_THREAD_SAFE_NEWLIB > 0u)
   struct _reent* local_impure_ptr;

   /*
    * create and initialise the impure pointer used for Newlib thread local storage.
    * This is only done if the C library is being used in a thread safe mode. Otherwise
    * a single reent structure is used for all threads, which saves memory.
    */

   local_impure_ptr = (struct _reent*)((((CPU_INT32U)(p_stk_base + stk_size)) & ~0x3) - sizeof(struct _reent));

   _REENT_INIT_PTR (local_impure_ptr);

   /*
    * create a stack frame at the top of the stack (leaving space for the
    * reentrant data structure).
    */

   frame_pointer = (CPU_INT32U*) local_impure_ptr;
#else
   frame_pointer =   (CPU_INT32U*) (((CPU_INT32U)(p_stk_base + stk_size)) & ~0x3);
#endif /* OS_THREAD_SAFE_NEWLIB */
   stk = frame_pointer - 13;

   /* Now fill the stack frame. */

   stk[12] = (CPU_INT32U)p_task;    /* task address (ra) */
   stk[11] = (CPU_INT32U)p_arg;     /* first register argument (r4) */

#if (OS_THREAD_SAFE_NEWLIB > 0u)
   stk[10] = (CPU_INT32U) local_impure_ptr; /* value of _impure_ptr for this thread */
#endif /* OS_THREAD_SAFE_NEWLIB */
   stk[0]  = ((CPU_INT32U)&OSStartTsk) + 4;/* exception return address (ea) */

   /* The next three lines don't generate any code, they just put symbols into
    * the debug information which will later be used to navigate the thread
    * data structures
    */
   __asm__ (".set OSTCBNext_OFFSET,%0"   :: "i" (offsetof(OS_TCB, NextPtr)));
   __asm__ (".set OSTCBPrio_OFFSET,%0"   :: "i" (offsetof(OS_TCB, Prio)));
   __asm__ (".set OSTCBStkPtr_OFFSET,%0" :: "i" (offsetof(OS_TCB, StkPtr)));

   return((CPU_STK *)stk);
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
#if (OS_CFG_TASK_STK_REDZONE_EN > 0u)
    CPU_BOOLEAN  stk_status;
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
    int_dis_time = CPU_IntDisMeasMaxCurReset();             /* Keep track of per-task interrupt disable time          */
    if (int_dis_time > OSTCBCurPtr->IntDisTimeMax) {
        OSTCBCurPtr->IntDisTimeMax = int_dis_time;
    }
#endif

#if OS_CFG_SCHED_LOCK_TIME_MEAS_EN > 0u
    if (OSSchedLockTimeMaxCur > OSTCBCurPtr->SchedLockTimeMax) { /* Keep track of per-task scheduler lock time        */
        OSTCBCurPtr->SchedLockTimeMax = OSSchedLockTimeMaxCur;
        OSSchedLockTimeMaxCur         = (CPU_TS)0;               /* Reset the per-task value                          */
    }
#endif

#if (OS_CFG_TASK_STK_REDZONE_EN > 0u)
                                                                /* Check if stack overflowed.                           */
    stk_status = OSTaskStkRedzoneChk((OS_TCB *)0u);
    if (stk_status != OS_TRUE) {
        OSRedzoneHitHook(OSTCBCurPtr);
    }
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

#ifdef ALT_INICHE
extern void cticks_hook(void);
#endif

void  OSTimeTickHook (void)
{
#if OS_CFG_APP_HOOKS_EN > 0u

#ifdef ALT_INICHE
    /* Service the Interniche timer Don't use pointer its part of bsp */
    cticks_hook();
#endif

    if (OS_AppTimeTickHookPtr != (OS_APP_HOOK_VOID)0) {
        (*OS_AppTimeTickHookPtr)();
    }
#endif
}


#ifdef __cplusplus
}
#endif

