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
*                                            ARMv8-A Port
*
* File    : os_cpu_c.c
* Version : V3.08.01
*********************************************************************************************************
* For     : ARMv8-A Cortex-A
* Mode    : ARM64
**********************************************************************************************************
*/


#define   OS_CPU_GLOBALS

#ifdef VSC_INCLUDE_SOURCE_FILE_NAMES
const  CPU_CHAR  *os_cpu_c__c = "$Id: $";
#endif


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include  <app_cfg.h>
#include  "../../../Source/os.h"
#include  "os_cpu.h"


#ifdef __cplusplus
extern  "C" {
#endif


/*
*********************************************************************************************************
*                                           IDLE TASK HOOK
*
* Description : This function is called by the idle task. This hook has been added to allow you to do
*               such things as STOP the CPU to conserve power.
*
* Argument(s) : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  OSIdleTaskHook (void)
{
#if (OS_CFG_APP_HOOKS_EN > 0u)
    if (OS_AppIdleTaskHookPtr != (OS_APP_HOOK_VOID)0) {
        (*OS_AppIdleTaskHookPtr)();
    }
#endif
}


/*
*********************************************************************************************************
*                                       OS INITIALIZATION HOOK
*
* Description : This function is called by OSInit() at the beginning of OSInit().
*
* Argument(s) : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  OSInitHook (void)
{
    CPU_STK_SIZE   i;
    CPU_STK       *p_stk;


    p_stk = OSCfg_ISRStkBasePtr;                                /* Clear the ISR stack                                  */
    for (i = 0u; i < OSCfg_ISRStkSize; i++) {
        *p_stk++ = (CPU_STK)0u;
    }
    OS_CPU_ExceptStkBase = (CPU_STK *)(OSCfg_ISRStkBasePtr + OSCfg_ISRStkSize - 1u);
    OS_CPU_ExceptStkBase = (CPU_STK *)((CPU_STK)OS_CPU_ExceptStkBase & ~(CPU_CFG_STK_ALIGN_BYTES - 1u));
}


/*
*********************************************************************************************************
*                                         STATISTIC TASK HOOK
*
* Description : This function is called every second by uC/OS-III's statistics task. This allows your
*               application to add functionality to the statistics task.
*
* Argument(s) : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  OSStatTaskHook (void)
{
#if (OS_CFG_APP_HOOKS_EN > 0u)
    if (OS_AppStatTaskHookPtr != (OS_APP_HOOK_VOID)0) {
        (*OS_AppStatTaskHookPtr)();
    }
#endif
}


/*
*********************************************************************************************************
*                                         TASK CREATION HOOK
*
* Description : This function is called when a task is created.
*
* Argument(s) : p_tcb        Pointer to the task control block of the task being created.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  OSTaskCreateHook (OS_TCB  *p_tcb)
{
#if (OS_CFG_APP_HOOKS_EN > 0u)
    if (OS_AppTaskCreateHookPtr != (OS_APP_HOOK_TCB)0) {
        (*OS_AppTaskCreateHookPtr)(p_tcb);
    }
#else
    (void)p_tcb;                                                /* Prevent compiler warning                             */
#endif
}


/*
*********************************************************************************************************
*                                         TASK DELETION HOOK
*
* Description : This function is called when a task is deleted.
*
* Argument(s) : p_tcb        Pointer to the task control block of the task being created.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  OSTaskDelHook (OS_TCB  *p_tcb)
{
#if (OS_CFG_APP_HOOKS_EN > 0u)
    if (OS_AppTaskDelHookPtr != (OS_APP_HOOK_TCB)0) {
        (*OS_AppTaskDelHookPtr)(p_tcb);
    }
#else
    (void)p_tcb;                                            /* Prevent compiler warning                               */
#endif
}


/*
*********************************************************************************************************
*                                          TASK RETURN HOOK
*
* Description : This function is called if a task accidentally returns. In other words, a task should
*               either be an infinite loop or delete itself when done.
*
* Argument(s) : p_tcb        Pointer to the task control block of the task being created.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  OSTaskReturnHook (OS_TCB  *p_tcb)
{
#if (OS_CFG_APP_HOOKS_EN > 0u)
    if (OS_AppTaskReturnHookPtr != (OS_APP_HOOK_TCB)0) {
        (*OS_AppTaskReturnHookPtr)(p_tcb);
    }
#else
    (void)p_tcb;                                            /* Prevent compiler warning                               */
#endif
}


/*
**********************************************************************************************************
*                                      INITIALIZE A TASK'S STACK
*
* Description : This function is called by OS_Task_Create() or OSTaskCreateExt() to initialize the stack
*               frame of the task being created. This function is highly processor specific.
*
* Argument(s) : p_task       Pointer to the task entry point address.
*
*               p_arg        Pointer to a user supplied data area that will be passed to the task
*                            when the task first executes.
*
*               p_stk_base   Pointer to the base address of the stack.
*
*               stk_size     Size of the stack, in number of CPU_STK elements.
*
*               opt          Options used to alter the behavior of OS_Task_StkInit().
*                            (see OS.H for OS_TASK_OPT_xxx).
*
* Returns     : Always returns the location of the new top-of-stack' once the processor registers have
*               been placed on the stack in the proper order.
*
* Note(s)     : (1) The full stack frame is shown below. If SIMD is disabled, (OS_CPU_SIMD == 0),
*                   the stack frame will only contain the core registers.
*
*                                            [LOW MEMORY]
*                                   ******************************
*                                   -0x320              [  FPSR  ]
*                                   -0x318              [  FPCR  ]
*                                   ******************************
*                                   -0x310              [   V0   ]
*                                   -0x300              [   V1   ]
*                                   -0x2F0              [   V2   ]
*                                   -0x2E0              [   V3   ]
*                                   -0x2D0              [   V4   ]
*                                   -0x2C0              [   V5   ]
*                                   -0x2B0              [   V6   ]
*                                   -0x2A0              [   V7   ]
*                                   -0x290              [   V8   ]
*                                   -0x280              [   V9   ]
*                                   -0x270              [  V10   ]
*                                   -0x260              [  V11   ]
*                                   -0x250              [  V12   ]
*                                   -0x240              [  V13   ]
*                                   -0x230              [  V14   ]
*                                   -0x220              [  V15   ]
*                                   -0x210              [  V16   ]
*                                   -0x200              [  V17   ]
*                                   -0x1F0              [  V18   ]
*                                   -0x1E0              [  V19   ]
*                                   -0x1D0              [  V20   ]
*                                   -0x1C0              [  V21   ]
*                                   -0x1B0              [  V22   ]
*                                   -0x1A0              [  V23   ]
*                                   -0x190              [  V24   ]
*                                   -0x180              [  V25   ]
*                                   -0x170              [  V26   ]
*                                   -0x160              [  V27   ]
*                                   -0x150              [  V28   ]
*                                   -0x140              [  V29   ]
*                                   -0x130              [  V30   ]
*                                   -0x120              [  V31   ]
*                                   ******************************
*                                   -0x110              [PADDING ]
*                                   -0x108              [SPSR_ELx]
*                                   -0x100              [   LR   ]
*                                   -0x0F8              [ELR_ELx ]
*                                   ******************************
*                                   -0x0F0              [   R0   ]
*                                   -0x0E8              [   R1   ]
*                                   -0x0E0              [   R2   ]
*                                   -0x0D8              [   R3   ]
*                                   -0x0D0              [   R4   ]
*                                   -0x0C8              [   R5   ]
*                                   -0x0C0              [   R6   ]
*                                   -0x0B8              [   R7   ]
*                                   -0x0B0              [   R8   ]
*                                   -0x0A8              [   R9   ]
*                                   -0x0A0              [  R10   ]
*                                   -0x098              [  R11   ]
*                                   -0x090              [  R12   ]
*                                   -0x088              [  R13   ]
*                                   -0x080              [  R14   ]
*                                   -0x078              [  R15   ]
*                                   -0x070              [  R16   ]
*                                   -0x068              [  R17   ]
*                                   -0x060              [  R18   ]
*                                   -0x058              [  R19   ]
*                                   -0x050              [  R20   ]
*                                   -0x048              [  R21   ]
*                                   -0x040              [  R22   ]
*                                   -0x038              [  R23   ]
*                                   -0x030              [  R24   ]
*                                   -0x028              [  R25   ]
*                                   -0x020              [  R26   ]
*                                   -0x018              [  R27   ]
*                                   -0x010              [  R28   ]
*                                   -0x008              [  R29   ]
*                                   **********Stack Base**********
*                                   ******SP_BASE MOD 16 = 0******
*                                            [HIGH MEMORY]
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
    CPU_STK     task_addr;
    CPU_INT32U  i;


    (void)opt;                                                  /* Prevent compiler warning                             */
    (void)p_stk_limit;

                                                                /* Align stack pointer to 16 bytes                      */
    p_stk = &p_stk_base[stk_size];
    p_stk = (CPU_STK *)((CPU_STK)p_stk & ~(CPU_CFG_STK_ALIGN_BYTES - 1u));

    task_addr = (CPU_STK)p_task;

    for (i = 29; i > 0; i--) {
        *--p_stk = (CPU_INT64U)i;                               /* Reg X1-X29                                           */
    }

    *--p_stk = (CPU_STK)p_arg;                                  /* Reg X0 : argument                                    */

    *--p_stk = (CPU_STK)task_addr;                              /* Entry Point                                          */
    *--p_stk = (CPU_STK)OS_TaskReturn;                          /* Reg X30 (LR)                                         */

    *--p_stk = (CPU_STK)OS_CPU_SPSRGet();
    *--p_stk = (CPU_STK)OS_CPU_SPSRGet();

    if (OS_CPU_SIMDGet() == 1u) {
        for (i = 64; i > 0; i--) {
            *--p_stk = (CPU_INT64U)i;                           /* Reg Q0-Q31                                           */
        }

        *--p_stk = 0x0000000000000000;                          /* FPCR                                                 */
        *--p_stk = 0x0000000000000000;                          /* FPSR                                                 */
    }

    return (p_stk);
}


/*
*********************************************************************************************************
*                                          TASK SWITCH HOOK
*
* Description : This function is called when a task switch is performed.  This allows you to perform
*               other operations during a context switch.
*
* Argument(s) : None.
*
* Note(s)     : 1) Interrupts are disabled during this call.
*
*               2) It is assumed that the global pointer 'OSTCBHighRdyPtr' points to the TCB of the task
*                  that will be 'switched in' (i.e. the highest priority task) and, 'OSTCBCurPtr' points
*                  to the task being switched out (i.e. the preempted task).
*********************************************************************************************************
*/

void  OSTaskSwHook (void)
{
#if (OS_CFG_TASK_PROFILE_EN > 0u)
    CPU_TS  ts;
#endif
#ifdef  CPU_CFG_INT_DIS_MEAS_EN
    CPU_TS  int_dis_time;
#endif



#if (OS_CFG_APP_HOOKS_EN > 0u)
    if (OS_AppTaskSwHookPtr != (OS_APP_HOOK_VOID)0) {
        (*OS_AppTaskSwHookPtr)();
    }
#endif

#if (OS_CFG_TASK_PROFILE_EN > 0u)
    ts = OS_TS_GET();
    if (OSTCBCurPtr != OSTCBHighRdyPtr) {
        OSTCBCurPtr->CyclesDelta  = ts - OSTCBCurPtr->CyclesStart;
        OSTCBCurPtr->CyclesTotal += (OS_CYCLES)OSTCBCurPtr->CyclesDelta;
    }

    OSTCBHighRdyPtr->CyclesStart = ts;
#endif

#ifdef  CPU_CFG_INT_DIS_MEAS_EN
    int_dis_time = CPU_IntDisMeasMaxCurReset();                   /* Keep track of per-task interrupt disable time    */
    if (OSTCBCurPtr->IntDisTimeMax < int_dis_time) {
        OSTCBCurPtr->IntDisTimeMax = int_dis_time;
    }
#endif

#if (OS_CFG_SCHED_LOCK_TIME_MEAS_EN > 0u)
    if (OSTCBCurPtr->SchedLockTimeMax < OSSchedLockTimeMaxCur) {  /* Keep track of per-task scheduler lock time       */
        OSTCBCurPtr->SchedLockTimeMax = OSSchedLockTimeMaxCur;
    }
    OSSchedLockTimeMaxCur = (CPU_TS)0;                            /* Reset the per-task value                         */
#endif
}


/*
*********************************************************************************************************
*                                              TICK HOOK
*
* Description : This function is called every tick.
*
* Argument(s) : None.
*
* Note(s)     : 1) This function is assumed to be called from the Tick ISR.
*********************************************************************************************************
*/

void  OSTimeTickHook (void)
{
#if (OS_CFG_APP_HOOKS_EN > 0u)
    if (OS_AppTimeTickHookPtr != (OS_APP_HOOK_VOID)0) {
        (*OS_AppTimeTickHookPtr)();
    }
#endif
}

#ifdef __cplusplus
}
#endif
