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
*                                           PIC24 MPLab Port
*
* File    : os_cpu_c.c
* Version : V3.08.00
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
* Note(s)    : 1) You may pass a task creation parameters through the opt variable. You MUST only use the
*                 upper 8 bits of 'opt' because the lower bits are reserved by uC/OS-III.  If you make
*                 changes to the code below, you will need to ensure that it doesn't affect the behaviour
*                 of OSTaskIdle() and OSTaskStat().
*
*              2) Registers are initialized to make them easy to differentiate with a debugger.
*
*              3) Setup the stack frame of the task:
*
*                        p_stk -  0  ->
*                        p_stk -  2  ->  CORCON
*                        p_stk -  4  ->  SR.8
*                        p_stk -  8  ->  RCOUNT
*                        p_stk - 10  ->  PSVPAG
*                        p_stk - 12  ->  TBLPAG
*                        p_stk - 14  ->  W14
*                        p_stk - 16  ->  W13
*                        p_stk - 18  ->  W12
*                        p_stk - 20  ->  W11
*                        p_stk - 22  ->  W10
*                        p_stk - 24  ->  W9
*                        p_stk - 26  ->  W8
*                        p_stk - 28  ->  W7
*                        p_stk - 30  ->  W6
*                        p_stk - 32  ->  W5
*                        p_stk - 34  ->  W4
*                        p_stk - 36  ->  W3
*                        p_stk - 38  ->  W2
*                        p_stk - 40  ->  W1
*                        p_stk - 44  ->  p_arg
*                        p_stk - 46  ->   SR (7..0) | PC (22..16)      Simulate ISR
*                        p_stk - 48  ->  PC (15..0)
*                        p_stk - 50  ->  PC (22..16)                              Simulate function call
*                        p_stk - 52  ->  PC (15..0)
*********************************************************************************************************
*/

CPU_STK *OSTaskStkInit (OS_TASK_PTR   p_task,
                        void         *p_arg,
                        CPU_STK      *p_stk_base,
                        CPU_STK      *p_stk_limit,
                        CPU_STK_SIZE  stk_size,
                        OS_OPT        opt)
 {
    CPU_INT16U   x;
    CPU_INT08U   pc_high;
    CPU_STK     *p_stk;

   (void)opt;                                                           /* Prevent compiler warning                                 */
   (void)p_stk_limit;


 	pc_high  = 0;                                                       /* Upper byte of PC always 0. Pointers are 16 bit unsigned  */

    p_stk    = &p_stk_base[0];                                          /* Load stack pointer                                       */

   *p_stk++  = (CPU_STK)p_task;                                         /* Simulate a call to the task by putting 32 bits of data   */
   *p_stk++  = (CPU_STK)pc_high;                                        /* data on the stack.                                       */

                                                                        /* Simulate an interrupt                                    */
   *p_stk++  = (CPU_STK)p_task;                                         /* Put the address of this task on the stack (PC)           */

    x        =  0;                                                      /* Set the SR to enable ALL interrupts                      */
    if (CORCONbits.IPL3) {                                              /* Check the CPU's current interrupt level 3 bit            */
         x  |= 0x0080;                                                  /* If set, then save the priority level bit in x bit [7]    */
    }
   *p_stk++  = (CPU_STK)(x | (CPU_INT16U)pc_high);                      /* Push the SR Low, CORCON IPL3 and PC (22..16)             */


   *p_stk++  = (CPU_STK)p_arg;                                          /* Initialize register W0                                   */
   *p_stk++  = 0x1111;                                                  /* Initialize register W1                                   */
   *p_stk++  = 0x2222;                                                  /* Initialize register W2                                   */
   *p_stk++  = 0x3333;                                                  /* Initialize register W3                                   */
   *p_stk++  = 0x4444;                                                  /* Initialize register W4                                   */
   *p_stk++  = 0x5555;                                                  /* Initialize register W5                                   */
   *p_stk++  = 0x6666;                                                  /* Initialize register W6                                   */
   *p_stk++  = 0x7777;                                                  /* Initialize register W7                                   */
   *p_stk++  = 0x8888;                                                  /* Initialize register W8                                   */
   *p_stk++  = 0x9999;                                                  /* Initialize register W9                                   */
   *p_stk++  = 0xAAAA;                                                  /* Initialize register W10                                  */
   *p_stk++  = 0xBBBB;                                                  /* Initialize register W11                                  */
   *p_stk++  = 0xCCCC;                                                  /* Initialize register W12                                  */
   *p_stk++  = 0xDDDD;                                                  /* Initialize register W13                                  */
   *p_stk++  = 0xEEEE;                                                  /* Initialize register W14                                  */

   *p_stk++  = TBLPAG;                                                  /* Push the Data Table Page Address on to the stack         */
   *p_stk++  = PSVPAG;                                                  /* Push the Program Space Visability Register on the stack  */
   *p_stk++  = RCOUNT;                                                  /* Push the Repeat Loop Counter Register on to the stack    */

   *p_stk++  = 0;                                                       /* Force the SR to enable all interrupt, clear flags        */
   *p_stk++  = CORCON;                                                  /* Push the Core Control Register on to the stack           */

    return (p_stk);                                                     /* Return the stack pointer to the new tasks stack          */
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
