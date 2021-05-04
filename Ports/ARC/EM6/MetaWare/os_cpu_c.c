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
*                                         Synopsys ARC EM6 Port
*
* File      : os_cpu_c.c
* Version   : V3.08.01
*********************************************************************************************************
* For       : Synopsys ARC EM6
* Mode      : Little-Endian, 32 registers, FPU, Code Density, Loop Counter, Stack Check
* Toolchain : MetaWare C/C++ Clang-based Compiler
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
*                                         OS INITIALIZATION HOOK
*
* Description: This function is called by OSInit() at the beginning of OSInit().
*
* Arguments  : None.
*
* Note(s)    : 1) This hook sets up automatic context save on interrupts.
*********************************************************************************************************
*/

void  OSInitHook (void)
{
                                                                /* Set automatic context save on interrupt for:         */
    CPU_AR_WR(CPU_AR_AUX_IRQ_CTRL, CPU_AR_AUX_IRQ_CTRL_L  |     /*   loop registers.                                    */
                                   CPU_AR_AUX_IRQ_CTRL_LP |     /*   code-density registers.                            */
                                   CPU_AR_AUX_IRQ_CTRL_NR);     /*   r27..r0 registers.                                 */

                                                                /* Setup interrupt stack.                               */
    CPU_AR_WR(CPU_AR_USTACK_TOP , (CPU_INT32U)&(OSCfg_ISRStkBasePtr[0]));
    CPU_AR_WR(CPU_AR_USTACK_BASE, (CPU_INT32U)&(OSCfg_ISRStkBasePtr[OSCfg_ISRStkSize]));
}


/*
*********************************************************************************************************
*                                            REDZONE HIT HOOK
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
    (void)p_tcb;                                                /* Prevent compiler warning.                            */
    CPU_SW_EXCEPTION(;);
#endif
}
#endif


/*
*********************************************************************************************************
*                                          STATISTIC TASK HOOK
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
*                                           TASK CREATION HOOK
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
    (void)p_tcb;                                                /* Prevent compiler warning.                            */
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
    (void)p_tcb;                                                /* Prevent compiler warning.                            */
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
    (void)p_tcb;                                                /* Prevent compiler warning.                            */
#endif
}


/*
*********************************************************************************************************
*                                       INITIALIZE A TASK'S STACK
*
* Description: This function is called by OSTaskCreate() to initialize the stack frame of the task being
*              created. This function is highly processor specific.
*
* Arguments  : p_task       Pointer to the task entry point address.
*
*              p_arg        Pointer to a user supplied data area that will be passed to the task
*                            when the task first executes.
*
*              p_stk_base   Pointer to the base address of the stack.
*
*              stk_size     Size of the stack, in number of CPU_STK elements.
*
*              opt          Options used to alter the behavior of OSTaskStkInit().
*                            (see OS.H for OS_TASK_OPT_xxx).
*
* Returns    : Always returns the location of the new top-of-stack' once the processor registers have
*              been placed on the stack in the proper order.
*
* Note(s)    : 1) Interrupts are enabled when task starts executing.
*
*              2) Registers are stacked in the following order, from high to low:
*
*                   STATUS32  (0x0A)
*                   PC        (r63)
*                   JLI_BASE  (0x290)
*                   LDI_BASE  (0x291)
*                   EI_BASE   (0x292)
*                   LP_COUNT  (r60)
*                   LP_START  (0x02)
*                   LP_END    (0x03)
*                   FP        (r27)
*                   GP        (r26)
*                   r25
*                   .
*                   .
*                   .
*                   r0
*                   BLINK     (r31)
*                   r30
*                   ILINK     (r29)
*                   ACCH      (r59)
*                   ACCL      (r58)
*********************************************************************************************************
*/

CPU_STK  *OSTaskStkInit (OS_TASK_PTR    p_task,
                         void          *p_arg,
                         CPU_STK       *p_stk_base,
                         CPU_STK       *p_stk_limit,
                         CPU_STK_SIZE   stk_size,
                         OS_OPT         opt)
{
    CPU_STK   *p_stk;
    CPU_DATA   i;


    (void)opt;                                                  /* 'opt' is not used, prevent warning                   */

    p_stk = &p_stk_base[stk_size];                              /* Load stack pointer                                   */
                                                                /* Align the stack to 4 bytes.                          */
    p_stk = (CPU_STK *)((CPU_STK)(p_stk) & 0xFFFFFFFCu);

                                                                /* Registers stacked as in saved by regular interrupt.  */
#if (OS_CFG_STAT_TASK_STK_CHK_EN > 0u)
    *(--p_stk) = (CPU_STK)(CPU_AR_STATUS32_IE |                 /*   STATUS32  (0x0A) with SC bit set.                  */
                           CPU_AR_STATUS32_SC);
#else
    *(--p_stk) = (CPU_STK)CPU_AR_STATUS32_IE;                   /*   STATUS32  (0x0A)                                   */
#endif
    *(--p_stk) = (CPU_STK)p_task;                               /*   PC        (r63)                                    */
    *(--p_stk) = (CPU_STK)0x0;                                  /*   JLI_BASE  (0x290)                                  */
    *(--p_stk) = (CPU_STK)0x0;                                  /*   LDI_BASE  (0x291)                                  */
    *(--p_stk) = (CPU_STK)0x0;                                  /*   EI_BASE   (0x292)                                  */
    *(--p_stk) = (CPU_STK)0x0;                                  /*   LP_COUNT  (r60)                                    */
    *(--p_stk) = (CPU_STK)0x0;                                  /*   LP_START  (0x02)                                   */
    *(--p_stk) = (CPU_STK)0x0;                                  /*   LP_END    (0x03)                                   */

    *(--p_stk) = (CPU_STK)0x0;                                  /*   FP        (r27)                                    */
    *(--p_stk) = (CPU_STK)_core_read(26);                       /*   GP        (r26)                                    */


    for (i = 25; i >= 1; --i) {                                 /*   r25..r1                                            */
       *(--p_stk) = (CPU_STK)(0x01010101*i);
    }

    *(--p_stk) = (CPU_STK)p_arg;                                /*   r0                                                 */

    *(--p_stk) = (CPU_STK)OS_TaskReturn;                        /*   BLINK     (r31)                                    */
    *(--p_stk) = (CPU_STK)0x1E1E1E1E;                           /*   r30                                                */
    *(--p_stk) = (CPU_STK)0x1D1D1D1D;                           /*   ILINK     (r29)                                    */
    *(--p_stk) = (CPU_STK)0x59595959;                           /*   ACCH      (r59)                                    */
    *(--p_stk) = (CPU_STK)0x58585858;                           /*   ACCL      (r58)                                    */

    return (p_stk);
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
#if (OS_CFG_TASK_STK_REDZONE_EN > 0u)
    CPU_BOOLEAN  stk_status;
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
    stk_status = OSTaskStkRedzoneChk((OS_TCB *)0);
    if (stk_status != OS_TRUE) {
        OSRedzoneHitHook(OSTCBCurPtr);
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
}


#ifdef __cplusplus
}
#endif
