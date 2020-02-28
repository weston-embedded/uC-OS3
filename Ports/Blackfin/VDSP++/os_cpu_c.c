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
*                            uCOS-III port for Analog Device's Blackfin 533
*
*                                           Visual DSP++ 5.0
*
*                  This port was made with a large contribution of Analog Devices Inc
*                                           development team
*
* File    : os_cpu_c.c
* Version : V3.08.00
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include "../../../Source/os.h"
#include <cpu.h>


#ifdef __cplusplus
extern  "C" {
#endif


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define  EVENT_VECTOR_TABLE_ADDR  0xFFE02000            /* Event vector table start address            */
#define  IPEND                    0xFFE02108            /* Interrupt Pending Register                  */
#define  IPEND_BIT_4_MASK         0xFFFFFFEF            /* IPEND Register bit 4 mask                   */
#define  IVG_NUM                          16            /* Interrupt vector number                     */
#define  pIPEND  ((volatile unsigned long *)IPEND)      /* Pointer to IPEND Register                   */

/*
*********************************************************************************************************
*                                          GLOBAL VARIABLES
*********************************************************************************************************
*/

static  FNCT_PTR  OS_CPU_IntHanlderTab[IVG_NUM] = {(void *)0};

/*
*********************************************************************************************************
*                                             LOCAL FUNCTIONS
*********************************************************************************************************
*/

void     OSTaskSwHook           (void);
void     OSTCBInitHook          (OS_TCB *ptcb);
void     OSInitHookBegin        (void);
void     OSInitHookEnd          (void);
void     OSTaskStatHook         (void);
void     OSTimeTickHook         (void);
void     OSTaskIdleHook         (void);
void     OS_CPU_IntHandler      (void);
void     OS_CPU_RegisterHandler (CPU_INT08U ivg, FNCT_PTR fn, BOOLEAN nesting);

/*
*********************************************************************************************************
*                                            EXTERNAL FUNCTIONS
*********************************************************************************************************
*/

extern  void  OSCtxSw                    (void);             /* See OS_CPU_A.S                              */
extern  void  OS_CPU_NESTING_ISR         (void);             /* See OS_CPU_A.S                              */
extern  void  OS_CPU_NON_NESTING_ISR     (void);             /* See OS_CPU_A.S                              */
extern  void  OS_CPU_EnableIntEntry      (CPU_INT08U mask);  /* See OS_CPU_A.S                              */
extern  void  OS_CPU_DisableIntEntry     (CPU_INT08U mask);  /* See OS_CPU_A.S                              */
extern  void  OS_CPU_Invalid_Task_Return (void);             /* See OS_CPU_A.S                              */

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
* Note(s)    : 1) Interrupts should be disabled during this call.
*********************************************************************************************************
*/

void  OSInitHook (void)
{

	INT32U *pEventVectorTable;

    pEventVectorTable = ((INT32U*)EVENT_VECTOR_TABLE_ADDR);    /* Pointer to Event Vector Table        */
    pEventVectorTable[IVG14] = (INT32U)&OSCtxSw;               /* Register the context switch          */
                                                               /* handler for IVG14                    */
    OS_CPU_EnableIntEntry(IVG14);                              /* Enable Interrupt for IVG14           */

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
* Note(s)    : 1) Interrupts are disabled during this call.
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
*                                           TASK SWITCH HOOK
*                                        void  OSTaskSwHook (void)
*
* Description: This function is called when a task switch is performed.  This allows you to perform other
*              operations during a context switch.
*
* Arguments  : None
*
* Returns    : None
*
* Note(s)    : 1) Interrupts are disabled during this call.
*              2) It is assumed that the global pointer 'OSTCBHighRdy' points to the TCB of the task that
*                 will be 'switched in' (i.e. the highest priority task) and, 'OSTCBCur' points to the
*                 task being switched out (i.e. the preempted task).
*********************************************************************************************************
*/

#if (OS_CPU_HOOKS_EN > 0) && (OS_TASK_SW_HOOK_EN > 0)
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
    int_dis_time = CPU_IntDisMeasMaxCurReset();
    if (OSTCBCurPtr->IntDisTimeMax < int_dis_time) {
        OSTCBCurPtr->IntDisTimeMax = int_dis_time;
    }
#endif

#if OS_CFG_SCHED_LOCK_TIME_MEAS_EN > 0u

    if (OSTCBCurPtr->SchedLockTimeMax < OSSchedLockTimeMaxCur) {
        OSTCBCurPtr->SchedLockTimeMax = OSSchedLockTimeMaxCur;
    }
    OSSchedLockTimeMaxCur = (CPU_TS)0;
#endif
}
#endif

/*
*********************************************************************************************************
*                                           TASK DELETION HOOK
*
* Description: This function is called when a task is deleted.
*
* Arguments  : p_tcb        Pointer to the task control block of the task being deleted.
*
* Note(s)    : 1) Interrupts are disabled during this call.
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


/*
*********************************************************************************************************
*                                           ISR HANDLER REGISTRATION
*                          void OS_CPU_RegisterHandler(INT8U ivg, FNCT_PTR fn, BOOLEAN nesting)
*
* Description : Registers Interrupts handler routine within the event table OS_CPU_IntrHanlderTab.
*               Chooses OS_CPU_NESTING_ISR or OS_CPU_NON_NESTING_ISR as ISR Handler depending on the value
*               of nesting argument.
*               Enables Interrupt for a given IVG (first argument)
*
* Arguments   : INT8U ivg       : interrupt vector groupe number (IVG0 to IVG15)
*               FNCT_PTR fn     : function pointer to the handler routine for a given IVG
*               BOOLEAN nesting : to choose Nested ISR or not
*
* Returns     : None
*
* Note(s)     : 1) IVG14 is reserved for task-level context switching
*               2) IVG6  is used for Core Timer to drive the OS ticks
*********************************************************************************************************
*/

void  OS_CPU_RegisterHandler (INT8U ivg, FNCT_PTR fn, BOOLEAN nesting)
{
    CPU_INT32U *pEventVectorTable;


    if (ivg > IVG15) {                                            /* The Blackfin 533 supports only 16 vectors  */
        return;
    }

    if (ivg == IVG14) {                                           /* IVG14 is reserved for task-level context   */
        return;                                                   /* switching                                  */
    }

    if (ivg == IVG4) {                                            /* Reserved vector                            */
        return;
    }

    pEventVectorTable = (INT32U*)EVENT_VECTOR_TABLE_ADDR;         /* pEventVectorTable points to the start      */
                                                                  /* address of Event vector table              */
    if (nesting == NESTED) {
        pEventVectorTable[ivg] = (INT32U)&OS_CPU_NESTING_ISR;     /* Select Nested ISR if nesting is required   */
                                                                  /* for the given ivg                          */
    } else {
        pEventVectorTable[ivg] = (INT32U)&OS_CPU_NON_NESTING_ISR; /* Select Non Nested ISR if nesting is not    */
                                                                  /* required for the given ivg                 */
    }

    OS_CPU_IntHanlderTab[ivg] = fn;                               /* Register Handler routine for the given ivg */
    OS_CPU_EnableIntEntry(ivg);

}

/*
*********************************************************************************************************
*                                           ISR HANDLER GLOBAL ROUTINE
*                                          void OS_CPU_IntHandler(void)
*
* Description : This routine is called by  OS_CPU_NON_NESTING_ISR or OS_CPU_NESTING_ISR when an
*               interrupt occurs. It determines the high priority pending interrupt request
*               depending on the value of IPEND register, and then selects the Handler routine for that
*               IRQ using OS_CPU_IntrHanlderTab table.
*
* Arguments   : None
*
* Returns     : None
*
* Note(s)     : None
*********************************************************************************************************
*/

void  OS_CPU_IntHandler (void)
{
    CPU_INT32U  status;
    CPU_INT32U  mask;
    CPU_INT08U  i;


    mask   = 1;
    status = *pIPEND & IPEND_BIT_4_MASK;                  /* Use IPEND_BIT_4_MASK to avoid testing     */
                                                          /* IPEND[4] :disable interrupts              */
    for (i =0; i < IVG_NUM; i++) {

        if ((1 << i) == (status & mask)) {

            if (OS_CPU_IntHanlderTab[i] != (void *)0) {   /* Be sure that Handler routine was          */
                                                          /* registered                                */
                 OS_CPU_IntHanlderTab[i]();               /* Branch to the Handler routine             */

            }
            break;

        }
        mask <<=1;
    }
}

/*
*********************************************************************************************************
*                                        INITIALIZE A TASK'S STACK
*
* Description: This function is called by either OSTaskCreate() or OSTaskCreateExt() to initialize the
*              stack frame of the task being created.  This function is highly processor specific.
*
* Arguments  :p_task       Pointer to the task entry point address.
*
*             p_arg        Pointer to a user supplied data area that will be passed to the task
*                               when the task first executes.
*
*             p_stk_base   Pointer to the base address of the stack.
*
*             p_stk_limit  Pointer to the stack limit
*
*             stk_size     Size of the stack, in number of CPU_STK elements.
*
*             opt          Options used to alter the behavior of OS_Task_StkInit().
*                            (see OS.H for OS_TASK_OPT_xxx).
*
* Returns    : Always returns the location of the new top-of-stack' once the processor registers have
*              been placed on the stack in the proper order.
*
* Note(s)    : This function does the following (refer to Porting chapter of uCOS-III book)
*               (1) Simulate a function call to the task with an argument
*               (2) Simulate ISR vector
*               (3) Setup stack frame to contain desired initial values of all registers
*               (4) Return top of stack pointer to the caller
*
*            Refer to VisualDSP++ C/C++ Compiler and Library Manual for Blackfin Processors
*            and ADSP-BF53x/BF56x Blackfin� Processor Programming Reference Manual.
*
*             The convention for the task frame (after context save is complete) is as follows:
*                      (stack represented from high to low memory as per convention)
*                                          (*** High memory ***) R0
*                                                                P1
*                                                                RETS       (function return address of thread)
*                                                                R1
*                                                                R2
*                                                                P0
*                                                                P2
*                                                                ASTAT
*                                                                RETI      (interrupt return address: $PC of thread)
*                                                                R7:3    (R7 is lower than R3)
*                                                                P5:3    (P5 is lower than P3)
*                                                                FP        (frame pointer)
*                                                                I3:0    (I3 is lower than I0)
*                                                                B3:0    (B3 is lower than B0)
*                                                                L3:0    (L3 is lower than L0)
*                                                                M3:0    (M3 is lower than M0)
*                                                                A0.x
*                                                                A0.w
*                                                                A1.x
*                                                                A1.w
*                                                                LC1:0    (LC1 is lower than LC0)
*                                                                LT1:0    (LT1 is lower than LT0)
*            OSTCBHighRdy--> OSTCBStkPtr --> (*** Low memory ***)LB1:0    (LB1 is lower than LB0)
*********************************************************************************************************
*/

CPU_STK  *OSTaskStkInit (OS_TASK_PTR    p_task,
                         void          *p_arg,
                         CPU_STK       *p_stk_base,
                         CPU_STK       *p_stk_limit,
                         CPU_STK_SIZE   stk_size,
                         OS_OPT         opt)

{
    CPU_STK     *stk;
    CPU_INT08U   i;


    (void)p_stk_limit;                            /* 'p_stk_limit' & 'opt' is not used, prevent warning    */
    (void)opt;

    stk    = (CPU_STK *)(p_stk_base + stk_size);  /* Load stack pointer                                    */

                                                  /* Simulate a function call to the task with an argument */
    stk   -= 3;                                   /* 3 words assigned for incoming args (R0, R1, R2)       */

                                                  /* Now simulating vectoring to an ISR                    */
    *--stk = (CPU_STK) p_arg;                     /* R0 value - caller's incoming argument #1              */
    *--stk = (CPU_STK) 0;                         /* P1 value - value irrelevant                           */

    *--stk = (CPU_STK)OS_CPU_Invalid_Task_Return; /* RETS value - NO task should return with RTS.          */
                                                  /* however OS_CPU_Invalid_Task_Return is a safety        */
                                                  /* catch-allfor tasks that return with an RTS            */

    *--stk = (CPU_STK) p_arg;                     /* R1 value - caller's incoming argument #2              */
                                                  /* (not relevant in current test example)                */
    *--stk = (CPU_STK) p_arg;                     /* R2 value - caller's incoming argument #3              */
                                                  /* (not relevant in current test example)                */
    *--stk = (CPU_STK) 0;                         /* P0 value - value irrelevant                           */
    *--stk = (CPU_STK) 0;                         /* P2 value - value irrelevant                           */
    *--stk = (CPU_STK) 0;                         /* ASTAT value - caller's ASTAT value - value            */
                                                  /* irrelevant                                            */

    *--stk = (CPU_STK) p_task;                    /* RETI value- pushing the start address of the task     */

    for (i = 35; i>0; i--) {                      /* remaining reg values - R7:3, P5:3,                    */
                                                  /* 4 words of A1:0(.W,.X), LT0, LT1,                     */
        *--stk = (CPU_STK)0;                      /* LC0, LC1, LB0, LB1,I3:0, M3:0, L3:0, B3:0,            */
    }                                             /* All values irrelevant                                 */

    return ((CPU_STK *)stk);                      /* Return top-of-stack                                   */
}


#ifdef __cplusplus
}
#endif
