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
*                                         ARC
*                                         MetaWare
*
* Filename     : os_cpu_c.c
* Version      : V3.08.01
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

#include  "os.h"

#ifdef __cplusplus
extern  "C" {
#endif

extern void task_startup_routine(void);
extern void dispatch(void);
uint32_t exc_nest_count;
volatile uint32_t ulCriticalNesting = 999UL;
volatile unsigned int context_switch_reqflg;
/*
*********************************************************************************************************
*                                             LOCAL DEFINES
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             IDLE TASK HOOK
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
*                                        OS INITIALIZATION HOOK
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
    OS_SysTickInit();
}


/*
*********************************************************************************************************
*                                           REDZONE HIT HOOK
*
* Description: This function is called when a task's stack overflowed.
*
* Arguments  : p_tcb        Pointer to the task control block of the offending task.
*
* Note(s)    : None.
*********************************************************************************************************
*/
#if (OS_CFG_TASK_STK_REDZONE_EN == DEF_ENABLED)
void  OSRedzoneHitHook (OS_TCB  *p_tcb)
{
#if OS_CFG_APP_HOOKS_EN > 0u
    if (OS_AppRedzoneHitHookPtr != (OS_APP_HOOK_TCB)0) {
        (*OS_AppRedzoneHitHookPtr)(p_tcb);
    }
#endif
    (void)p_tcb;                                                /* Prevent compiler warning                             */
    CPU_SW_EXCEPTION(;);
}
#endif


/*
*********************************************************************************************************
*                                          STATISTIC TASK HOOK
*
* Description: This function is called periodically by uC/OS-III's statistics task.  This allows your
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
*                                          TASK DELETION HOOK
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
*                                           TASK RETURN HOOK
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
**********************************************************************************************************
*                                        INITIALIZE A TASK'S STACK
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
    CPU_STK  *p_stk;


    (void)p_stk_limit;                                          /* Prevent compiler warning                             */
    (void)opt;

    p_stk    = &p_stk_base[stk_size];                           /* Load stack pointer                                   */
    /* Put CPU register values onto the stack                                                                      */

    *(--p_stk) = (CPU_STK) p_arg;
    *(--p_stk) = (CPU_STK) p_task;
    *(--p_stk) = (CPU_STK) task_startup_routine;   //dispatch return address
    *(--p_stk) = (CPU_STK) 0u;                     //preserve a place for critical nesting counter
    return (p_stk);
}


void  OSCtxSw(void)
{
    CPU_SR_ALLOC();
    CPU_CRITICAL_ENTER();
    dispatch();
    CPU_CRITICAL_EXIT();
}

void  OSIntCtxSw(void)
{
    CPU_SR_ALLOC();
    CPU_CRITICAL_ENTER();
    context_switch_reqflg = true;
    CPU_CRITICAL_EXIT();
}

/*
*********************************************************************************************************
*                                            TASK SWITCH HOOK
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
#if (OS_CFG_TASK_STK_REDZONE_EN == DEF_ENABLED)
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

#if (OS_CFG_TASK_STK_REDZONE_EN == DEF_ENABLED)
                                                                /* Check if stack overflowed.                           */
    stk_status = OSTaskStkRedzoneChk(DEF_NULL);
    if (stk_status != DEF_OK) {
        CPU_SW_EXCEPTION(;);
    }
#endif
}


/*
*********************************************************************************************************
*                                               TICK HOOK
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
#if (CPU_CFG_TS_EN == DEF_ENABLED)
    CPU_TS_Update();
#endif
}

static void  OS_CPU_SysTickHandler  (void)
{
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
    OSIntEnter();                                               /* Tell uC/OS-III that we are starting an ISR           */
    CPU_CRITICAL_EXIT();

    OSTimeTick();                                               /* Call uC/OS-III's OSTimeTick()                        */

    OSIntExit();                                                /* Tell uC/OS-III that we are leaving the ISR           */
}

extern EMBARC_HOOK_VOID board_timer_isr_hook;

int OS_SysTickInit(void)
{
    board_timer_isr_hook = OS_CPU_SysTickHandler;

    return 0;
}

void OS_Set_StackCheck(OS_TCB *old, OS_TCB *new)
{
    (void)old;
    if (new != NULL) {
        arc_kernel_stack_check_configure((uint32_t)(new->StkBasePtr), (uint32_t)(new->StkSize + new->StkBasePtr));
    }
}

#ifdef __cplusplus
}
#endif

