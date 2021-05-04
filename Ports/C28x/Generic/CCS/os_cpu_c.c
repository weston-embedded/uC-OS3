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
*                                             TI C28x Port
*
* File      : os_cpu_c.c
* Version   : V3.08.01
*********************************************************************************************************
* For       : TI C28x
* Mode      : C28 Object mode
* Toolchain : TI C/C++ Compiler
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
*                                             LOCAL DEFINES
*********************************************************************************************************
*/


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
   (void)&p_tcb;                                                /* Prevent compiler warning.                            */
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
   (void)&p_tcb;                                                /* Prevent compiler warning.                            */
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
   (void)&p_tcb;                                                /* Prevent compiler warning.                            */
#endif
}


/*
**********************************************************************************************************
*                                       INITIALIZE A TASK'S STACK
*
* Description: This function is called by OSTaskCreate() to initialize the stack frame of the task being
*              created. This function is highly processor specific.
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
*              opt          Options used to alter the behavior of OSTaskStkInit().
*                            (see OS.H for OS_OPT_TASK_xxx).
*
* Returns    : Always returns the location of the new top-of-stack' once the processor registers have
*              been placed on the stack in the proper order.
*
* Note(s)    : 1) Interrupts are enabled when task starts executing.
*
*              2) See inline comments for stack frame format.
**********************************************************************************************************
*/

CPU_STK  *OSTaskStkInit (OS_TASK_PTR    p_task,
                         void          *p_arg,
                         CPU_STK       *p_stk_base,
                         CPU_STK       *p_stk_limit,
                         CPU_STK_SIZE   stk_size,
                         OS_OPT         opt)
{
    CPU_STK     *p_stk;
    CPU_INT32U  *p_stk32;


   (void)&p_stk_limit;                                          /* Prevent compiler warnings.                           */
   (void)&opt;
   (void)&stk_size;

                                                                /* Load and pre-align stack pointer.                    */
    p_stk = &p_stk_base[0];
    if ((CPU_INT32U)(void *)p_stk % 2u == 0u) {
        p_stk++;
    }
    p_stk32    = (CPU_INT32U *)p_stk;
                                                                /* Save registers as if auto-saved.                     */
                                                                /* Follow stacking method in "9) Perform automatic ...  */
                                                                /* ..context save" of section 3.4 in "SPRU430E"         */
    *p_stk32++ = (0x11110000) | OS_CPU_GetST0();                /*   T:ST0                                              */
    *p_stk32++ =  0x33332222;                                   /*   AH:AL                                              */
    *p_stk32++ =  0x55554444;                                   /*   PH:PL                                              */
    *p_stk32++ =  0x77776666;                                   /*   AR1:AR0                                            */
    *p_stk32++ = (0x00000000) | OS_CPU_GetST1();                /*   DP:ST1                                             */
    *p_stk32++ =  0x00000000;                                   /*   DBGSTAT:IER                                        */
    *p_stk32++ = (CPU_INT32U)p_task;                            /*   Save Return Address [PC+1].                        */

                                                                /* Save remaining registers.                            */
    *p_stk32++ =  0x77776666;                                   /*   AR1H:AR0H                                          */
    *p_stk32++ =  0x99999999;                                   /*   XAR2                                               */
    *p_stk32++ =  0xAAAAAAAA;                                   /*   XAR3                                               */
    *p_stk32++ = (CPU_INT32U)p_arg;                             /*   XAR4: void * parameter.                            */
    *p_stk32++ =  0xCCCCCCCC;                                   /*   XAR5                                               */
    *p_stk32++ =  0xDDDDDDDD;                                   /*   XAR6                                               */
    *p_stk32++ =  0xEEEEEEEE;                                   /*   XAR7                                               */
    *p_stk32++ =  0xFFFFFFFF;                                   /*   XT                                                 */
    *p_stk32++ = (CPU_INT32U)&OS_TaskReturn;                    /*   RPC                                                */

#if __TMS320C28XX_FPU32__ == 1                                  /* Save FPU registers, if enabled.                      */
    *p_stk32++ =  0x00000000;                                   /*   R0H                                                */
    *p_stk32++ =  0x11111111;                                   /*   R1H                                                */
    *p_stk32++ =  0x22222222;                                   /*   R2H                                                */
    *p_stk32++ =  0x33333333;                                   /*   R3H                                                */
    *p_stk32++ =  0x44444444;                                   /*   R4H                                                */
    *p_stk32++ =  0x55555555;                                   /*   R5H                                                */
    *p_stk32++ =  0x66666666;                                   /*   R6H                                                */
    *p_stk32++ =  0x77777777;                                   /*   R7H                                                */
    *p_stk32++ =  0x00000000;                                   /*   STF                                                */
    *p_stk32++ =  0x00000000;                                   /*   RB                                                 */
#endif
                                                                /* Return pointer to next free location.                */
    return ((CPU_STK *)p_stk32);
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
#if (CPU_CFG_TS_EN > 0u)
    CPU_TS_Update();
#endif
}

#ifdef __cplusplus
}
#endif
