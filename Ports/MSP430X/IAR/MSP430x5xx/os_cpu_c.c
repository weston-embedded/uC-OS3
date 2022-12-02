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
*                                              TI MSP430X
*                                              MSP430x5xx
*
* File     : os_cpu.h
* Version  : V3.08.02
*********************************************************************************************************
* Compiler : IAR System Embedded Workbench for TI MSP430 V5.20
*********************************************************************************************************
*/

#include <os_cfg_app.h>

#define  OS_CPU_GLOBALS

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
    OSISRStk = &OSCfg_ISRStk[OS_CFG_ISR_STK_SIZE - 1];
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
*                                        INITIALIZE A TASK'S STACK
*
* Description: This function is called by either OSTaskCreate() to initialize the stack frame of the
*              task being created.  This function is highly processor specific.
*
* Arguments  : p_task        is a pointer to the task code
*
*              p_arg         is a pointer to a user supplied data area that will be passed to the task
*                            when the task first executes.
*
*              p_stk_base    is a pointer to the base of the stack.
*
*              p_stk_limit   is a stack threshold specified by application code.
*
*              stk_size      is the size of the stack.
*
*              opt           contains teh option flags passed to OSTaskCreate() by the application
*                            developer
*
* Returns    : Always returns the location of the new top-of-stack once the processor registers have
*              been placed on the stack in the proper order.
*
* Note(s)    : The register passing convention for the IAR compiler is that the LEFTMOST two parameters
*              are passed in registers:
*              1st parameter:  R12 for a 16-bit value or,  R13:R12 for a 32-bit value
*              2nd parameter:  R14 for a 16-bit value or,  R15:R14 for a 32-bit value
*
*              OSTaskStkInit() thus needs to place 'p_arg' in R12 since when the task is started, 'p_arg'
*              is the FIRST argument of the task declaration:
*
*                 void  Task (void *p_arg)
*                 {
*                 }
*********************************************************************************************************
*/

CPU_STK  *OSTaskStkInit (OS_TASK_PTR    p_task,
                         void          *p_arg,
                         CPU_STK       *p_stk_base,
                         CPU_STK       *p_stk_limit,
                         CPU_STK_SIZE   stk_size,
                         OS_OPT         opt)
{
    CPU_INT16U  *top;


    opt    = opt;
    top    = (CPU_INT16U *)&p_stk_base[stk_size - 1];
    top--;
    *top-- = (CPU_INT16U)((CPU_INT32U)p_task >>    16);
    *top-- = (CPU_INT16U)((CPU_INT32U)p_task & 0xFFFF);                 /* Interrupt return pointer                          */
    *top-- = (CPU_INT16U)0x0008;                                        /* Status register                                   */
    *top-- = (CPU_INT16U)0x0015;
    *top-- = (CPU_INT16U)0x1515;
    *top-- = (CPU_INT16U)0x0014;
    *top-- = (CPU_INT16U)0x1414;
    *top-- = (CPU_INT16U)0x0013;
    *top-- = (CPU_INT16U)0x1313;
    *top-- = (CPU_INT16U)((CPU_INT32U)p_arg >>    16);                  /* Pass 'p_arg' through register R12                 */
    *top-- = (CPU_INT16U)((CPU_INT32U)p_arg & 0xFFFF);                  /* Pass 'p_arg' through register R12                 */
    *top-- = (CPU_INT16U)0x0011;
    *top-- = (CPU_INT16U)0x1111;
    *top-- = (CPU_INT16U)0x0010;
    *top-- = (CPU_INT16U)0x1010;
    *top-- = (CPU_INT16U)0x0009;
    *top-- = (CPU_INT16U)0x0909;
    *top-- = (CPU_INT16U)0x0008;
    *top-- = (CPU_INT16U)0x0808;
    *top-- = (CPU_INT16U)0x0007;
    *top-- = (CPU_INT16U)0x0707;
    *top-- = (CPU_INT16U)0x0006;
    *top-- = (CPU_INT16U)0x0606;
    *top-- = (CPU_INT16U)0x0005;
    *top-- = (CPU_INT16U)0x0505;
    *top-- = (CPU_INT16U)0x0004;
    *top   = (CPU_INT16U)0x0404;
    return ((CPU_STK *)top);
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
#if (CPU_CFG_TS_EN > 0u)
    CPU_TS_Update();
#endif
}


#ifdef __cplusplus
}
#endif
