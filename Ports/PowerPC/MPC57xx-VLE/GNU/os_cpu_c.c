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
*                                           MPC57xx VLE Port
*                                             GNU Toolchain
*
* File    : os_cpu_c.c
* Version : V3.08.01
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
#include  "os_cpu.h"


#ifdef __cplusplus
extern  "C" {
#endif

/*
*********************************************************************************************************
*                                             GLOBALS
*********************************************************************************************************
*/

        CPU_STK     *OS_CPU_ISRStkBase;                         /* 8-byte aligned base of the ISR stack.                */
        CPU_INT32U   OS_CPU_ISRNestingCtr;                      /* Total Nesting Counter: Kernel Aware and Fast IRQs    */

extern  char         _SDA_BASE_[];                              /* Defined by the linker, as per the PPC System V ABI.  */
extern  char         _SDA2_BASE_[];                             /* Defined by the linker, as per the PPC EABI.          */


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
* Note(s)    : (1) This function initializes the ISR stack.
*********************************************************************************************************
*/

void  OSInitHook (void)
{
    CPU_STK_SIZE   i;
    CPU_STK       *p_stk;


    p_stk = OSCfg_ISRStkBasePtr;                                /* Clear the ISR stack                                    */
    for (i = 0u; i < OSCfg_ISRStkSize; i++) {
        *p_stk++ = (CPU_STK)0u;
    }

    p_stk = &OSCfg_ISRStk[OS_CFG_ISR_STK_SIZE-1u];
    p_stk = (CPU_STK *)((CPU_INT32U)p_stk & 0xFFFFFFF8u);       /* Align top of stack to 8-bytes (EABI).                */

    OS_CPU_ISRStkBase  = p_stk;
    OS_CPU_ISRNestingCtr = 0;
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
#if (OS_CFG_TASK_STK_REDZONE_EN > 0u)
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
*
*              2) The stack frame used to save and restore context is shown below:
*
*                                 Low Address +-----------+ Top of Stack
*                                             | Backchain |
*                                             +-----------+
*                                             |  R1 (LR)  |
*                                             +-----------+
*                                             |    MSR    |
*                                             +-----------+
*                                             |   SRR0    |
*                                             +-----------+
*                                             |   SRR1    |
*                                             +-----------+
*                                             |    CTR    |
*                                             +-----------+
*                                             |    XER    |
*                                             +-----------+
*                                             |  SPEFSCR  |
*                                             +-----------+
*                                             |    R0     |
*                                             +-----------+
*                                             |    R2     |
*                                             +-----------+
*                                             |    R3     |
*                                             +-----------+
*                                                  ...
*                                             +-----------+
*                                             |    R31    |
*                                High Address +-----------+ Bottom of Stack
*
*
*
*              3) The back chain always points to the location of the previous frame's back chain.
*                 The first stack frame shall contain a null back chain, as per the PPC ABI.
*
*                                    Frame 1  +-----------+
*                                             | Backchain | ------+
*                                             +-----------+       |
*                                             |           |       |
*                                             | REGISTERS |       |
*                                             |           |       |
*                                             +-----------+       |
*                                                                 |
*                                                                 |
*                                    Frame 0  +-----------+       |
*                                             |   NULL    | <<----+
*                                             +-----------+
*                                             |  R1 (LR)  |
*                                             +-----------+
*
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
                                                                /* ---------------------------------------------------- */
                                                                /* --------------- INITIAL STACK FRAME ---------------- */
                                                                /* ---------------------------------------------------- */
    p_stk = &p_stk_base[stk_size-2u];                           /* Initial Stack is 2 words: LR and Backchain.          */

                                                                /* Align top of stack to 8-bytes (EABI).                */
    p_stk = (CPU_STK *)((CPU_INT32U)p_stk & 0xFFFFFFF8u);

    *(p_stk+1) = 0x00000000u;                                   /* LR: null for the initial frame.                      */
    *p_stk--   = 0x00000000u;                                   /* Backchain: null for the initial frame.               */

                                                                /* ---------------------------------------------------- */
                                                                /* ----------------- TASK STACK FRAME ----------------- */
                                                                /* ---------------------------------------------------- */
    *p_stk-- = 0x1F1F1F1Fu;                                     /* R31                                                  */
    *p_stk-- = 0x1E1E1E1Eu;                                     /* R30                                                  */
    *p_stk-- = 0x1D1D1D1Du;                                     /* R29                                                  */
    *p_stk-- = 0x1C1C1C1Cu;                                     /* R28                                                  */
    *p_stk-- = 0x1B1B1B1Bu;                                     /* R27                                                  */
    *p_stk-- = 0x1A1A1A1Au;                                     /* R26                                                  */
    *p_stk-- = 0x19191919u;                                     /* R25                                                  */
    *p_stk-- = 0x18181818u;                                     /* R24                                                  */
    *p_stk-- = 0x17171717u;                                     /* R23                                                  */
    *p_stk-- = 0x16161616u;                                     /* R22                                                  */
    *p_stk-- = 0x15151515u;                                     /* R21                                                  */
    *p_stk-- = 0x14141414u;                                     /* R20                                                  */
    *p_stk-- = 0x13131313u;                                     /* R19                                                  */
    *p_stk-- = 0x12121212u;                                     /* R18                                                  */
    *p_stk-- = 0x11111111u;                                     /* R17                                                  */
    *p_stk-- = 0x10101010u;                                     /* R16                                                  */
    *p_stk-- = 0x0F0F0F0Fu;                                     /* R15                                                  */
    *p_stk-- = 0x0E0E0E0Eu;                                     /* R14                                                  */
    *p_stk-- = (CPU_INT32U)_SDA_BASE_;                          /* R13                                                  */
    *p_stk-- = 0x0C0C0C0Cu;                                     /* R12                                                  */
    *p_stk-- = 0x0B0B0B0Bu;                                     /* R11                                                  */
    *p_stk-- = 0x0A0A0A0Au;                                     /* R10                                                  */
    *p_stk-- = 0x09090909u;                                     /* R9                                                   */
    *p_stk-- = 0x08080808u;                                     /* R8                                                   */
    *p_stk-- = 0x07070707u;                                     /* R7                                                   */
    *p_stk-- = 0x06060606u;                                     /* R6                                                   */
    *p_stk-- = 0x05050505u;                                     /* R5                                                   */
    *p_stk-- = 0x04040404u;                                     /* R4                                                   */
    *p_stk-- = (CPU_INT32U)p_arg;                               /* R3: Task Argument                                    */
    *p_stk-- = (CPU_INT32U)_SDA2_BASE_;                         /* R2                                                   */
    *p_stk-- = 0x00000000u;                                     /* R0                                                   */

    *p_stk-- = 0x00000000u;                                     /* Condition Register                                   */

    *p_stk-- = 0x00000000u;                                     /* SPEFSCR                                              */
    *p_stk-- = 0x00000000u;                                     /* XER                                                  */
    *p_stk-- = 0x00000000u;                                     /* CTR                                                  */
    *p_stk-- = 0x00008000u;                                     /* SRR1: External Interrupts enabled after RFI.         */
    *p_stk-- = (CPU_INT32U)p_task;                              /* SRR0: PC after RFI.                                  */
    *p_stk-- = 0x00000000u;                                     /* MSR: Interrupts disabled until RFI.                  */

    *p_stk-- = (CPU_INT32U)OS_TaskReturn;                       /* LR used to catch a task return.                      */
    *p_stk   = (CPU_INT32U)p_stk + (40u * 4u);                  /* Backchain                                            */

    return(p_stk);
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
#if (CPU_CFG_TS_EN > 0u)
    CPU_TS_Update();
#endif
}

#ifdef __cplusplus
}
#endif
