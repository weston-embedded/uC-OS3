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
*                                            CORE FUNCTIONS
*
* File    : os_core.c
* Version : V3.08.00
*********************************************************************************************************
*/

#define  MICRIUM_SOURCE
#include "os.h"

#ifdef VSC_INCLUDE_SOURCE_FILE_NAMES
const  CPU_CHAR  *os_core__c = "$Id: $";
#endif

/*
************************************************************************************************************************
*                                                    INITIALIZATION
*
* Description: This function is used to initialize the internals of uC/OS-III and MUST be called prior to
*              creating any uC/OS-III object and, prior to calling OSStart().
*
* Arguments  : p_err         is a pointer to a variable that will contain an error code returned by this function.
*
*                                OS_ERR_NONE    Initialization was successful
*                                Other          Other OS_ERR_xxx depending on the sub-functions called by OSInit().
* Returns    : none
************************************************************************************************************************
*/

void  OSInit (OS_ERR  *p_err)
{
#if (OS_CFG_ISR_STK_SIZE > 0u)
    CPU_STK      *p_stk;
    CPU_STK_SIZE  size;
#endif



#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return;
    }
#endif

    OSInitHook();                                               /* Call port specific initialization code               */

    OSIntNestingCtr       =           0u;                       /* Clear the interrupt nesting counter                  */

    OSRunning             =  OS_STATE_OS_STOPPED;               /* Indicate that multitasking has not started           */

    OSSchedLockNestingCtr =           0u;                       /* Clear the scheduling lock counter                    */

    OSTCBCurPtr           = (OS_TCB *)0;                        /* Initialize OS_TCB pointers to a known state          */
    OSTCBHighRdyPtr       = (OS_TCB *)0;

    OSPrioCur             =           0u;                       /* Initialize priority variables to a known state       */
    OSPrioHighRdy         =           0u;

#if (OS_CFG_SCHED_LOCK_TIME_MEAS_EN > 0u)
    OSSchedLockTimeBegin  =           0u;
    OSSchedLockTimeMax    =           0u;
    OSSchedLockTimeMaxCur =           0u;
#endif

#ifdef OS_SAFETY_CRITICAL_IEC61508
    OSSafetyCriticalStartFlag = OS_FALSE;
#endif

#if (OS_CFG_SCHED_ROUND_ROBIN_EN > 0u)
    OSSchedRoundRobinEn             = OS_FALSE;
    OSSchedRoundRobinDfltTimeQuanta = OSCfg_TickRate_Hz / 10u;
#endif

#if (OS_CFG_ISR_STK_SIZE > 0u)
    p_stk = OSCfg_ISRStkBasePtr;                                /* Clear exception stack for stack checking.            */
    if (p_stk != (CPU_STK *)0) {
        size  = OSCfg_ISRStkSize;
        while (size > 0u) {
            size--;
           *p_stk = 0u;
            p_stk++;
        }
    }
#if (OS_CFG_TASK_STK_REDZONE_EN > 0u)                           /* Initialize Redzoned ISR stack                        */
    OS_TaskStkRedzoneInit(OSCfg_ISRStkBasePtr, OSCfg_ISRStkSize);
#endif
#endif

#if (OS_CFG_APP_HOOKS_EN > 0u)                                  /* Clear application hook pointers                      */
#if (OS_CFG_TASK_STK_REDZONE_EN > 0u)
    OS_AppRedzoneHitHookPtr = (OS_APP_HOOK_TCB )0;
#endif
    OS_AppTaskCreateHookPtr = (OS_APP_HOOK_TCB )0;
    OS_AppTaskDelHookPtr    = (OS_APP_HOOK_TCB )0;
    OS_AppTaskReturnHookPtr = (OS_APP_HOOK_TCB )0;

    OS_AppIdleTaskHookPtr   = (OS_APP_HOOK_VOID)0;
    OS_AppStatTaskHookPtr   = (OS_APP_HOOK_VOID)0;
    OS_AppTaskSwHookPtr     = (OS_APP_HOOK_VOID)0;
    OS_AppTimeTickHookPtr   = (OS_APP_HOOK_VOID)0;
#endif

#if (OS_CFG_TASK_REG_TBL_SIZE > 0u)
    OSTaskRegNextAvailID = 0u;
#endif

    OS_PrioInit();                                              /* Initialize the priority bitmap table                 */

    OS_RdyListInit();                                           /* Initialize the Ready List                            */


#if (OS_CFG_FLAG_EN > 0u)                                       /* Initialize the Event Flag module                     */
#if (OS_CFG_DBG_EN > 0u)
    OSFlagDbgListPtr = (OS_FLAG_GRP *)0;
    OSFlagQty        =                0u;
#endif
#endif

#if (OS_CFG_MEM_EN > 0u)                                        /* Initialize the Memory Manager module                 */
    OS_MemInit(p_err);
    if (*p_err != OS_ERR_NONE) {
        return;
    }
#endif


#if (OS_MSG_EN > 0u)                                            /* Initialize the free list of OS_MSGs                  */
    OS_MsgPoolInit(p_err);
    if (*p_err != OS_ERR_NONE) {
        return;
    }
#endif


#if (OS_CFG_MUTEX_EN > 0u)                                      /* Initialize the Mutex Manager module                  */
#if (OS_CFG_DBG_EN > 0u)
    OSMutexDbgListPtr = (OS_MUTEX *)0;
    OSMutexQty        =             0u;
#endif
#endif


#if (OS_CFG_Q_EN > 0u)                                          /* Initialize the Message Queue Manager module          */
#if (OS_CFG_DBG_EN > 0u)
    OSQDbgListPtr = (OS_Q *)0;
    OSQQty        =         0u;
#endif
#endif


#if (OS_CFG_SEM_EN > 0u)                                        /* Initialize the Semaphore Manager module              */
#if (OS_CFG_DBG_EN > 0u)
    OSSemDbgListPtr = (OS_SEM *)0;
    OSSemQty        =           0u;
#endif
#endif


#if defined(OS_CFG_TLS_TBL_SIZE) && (OS_CFG_TLS_TBL_SIZE > 0u)
    OS_TLS_Init(p_err);                                         /* Initialize Task Local Storage, before creating tasks */
    if (*p_err != OS_ERR_NONE) {
        return;
    }
#endif


    OS_TaskInit(p_err);                                         /* Initialize the task manager                          */
    if (*p_err != OS_ERR_NONE) {
        return;
    }


#if (OS_CFG_TASK_IDLE_EN > 0u)
    OS_IdleTaskInit(p_err);                                     /* Initialize the Idle Task                             */
    if (*p_err != OS_ERR_NONE) {
        return;
    }
#endif


#if (OS_CFG_TICK_EN > 0u)
    OS_TickInit(p_err);
    if (*p_err != OS_ERR_NONE) {
        return;
    }
#endif


#if (OS_CFG_STAT_TASK_EN > 0u)                                  /* Initialize the Statistic Task                        */
    OS_StatTaskInit(p_err);
    if (*p_err != OS_ERR_NONE) {
        return;
    }
#endif


#if (OS_CFG_TMR_EN > 0u)                                        /* Initialize the Timer Manager module                  */
    OS_TmrInit(p_err);
    if (*p_err != OS_ERR_NONE) {
        return;
    }
#endif


#if (OS_CFG_DBG_EN > 0u)
    OS_Dbg_Init();
#endif


    OSCfg_Init();

    OSInitialized = OS_TRUE;                                    /* Kernel is initialized                                */
}


/*
************************************************************************************************************************
*                                                      ENTER ISR
*
* Description: This function is used to notify uC/OS-III that you are about to service an interrupt service routine
*              (ISR).  This allows uC/OS-III to keep track of interrupt nesting and thus only perform rescheduling at
*              the last nested ISR.
*
* Arguments  : none
*
* Returns    : none
*
* Note(s)    : 1) This function MUST be called with interrupts already disabled
*
*              2) Your ISR can directly increment 'OSIntNestingCtr' without calling this function because OSIntNestingCtr has
*                 been declared 'global', the port is actually considered part of the OS and thus is allowed to access
*                 uC/OS-III variables.
*
*              3) You MUST still call OSIntExit() even though you increment 'OSIntNestingCtr' directly.
*
*              4) You MUST invoke OSIntEnter() and OSIntExit() in pair.  In other words, for every call to OSIntEnter()
*                 (or direct increment to OSIntNestingCtr) at the beginning of the ISR you MUST have a call to OSIntExit()
*                 at the end of the ISR.
*
*              5) You are allowed to nest interrupts up to 250 levels deep.
************************************************************************************************************************
*/

void  OSIntEnter (void)
{
    OS_TRACE_ISR_ENTER();

    if (OSRunning != OS_STATE_OS_RUNNING) {                     /* Is OS running?                                       */
        return;                                                 /* No                                                   */
    }

    if (OSIntNestingCtr >= 250u) {                              /* Have we nested past 250 levels?                      */
        return;                                                 /* Yes                                                  */
    }

    OSIntNestingCtr++;                                          /* Increment ISR nesting level                          */
}


/*
************************************************************************************************************************
*                                                       EXIT ISR
*
* Description: This function is used to notify uC/OS-III that you have completed servicing an ISR.  When the last nested
*              ISR has completed, uC/OS-III will call the scheduler to determine whether a new, high-priority task, is
*              ready to run.
*
* Arguments  : none
*
* Returns    : none
*
* Note(s)    : 1) You MUST invoke OSIntEnter() and OSIntExit() in pair.  In other words, for every call to OSIntEnter()
*                 (or direct increment to OSIntNestingCtr) at the beginning of the ISR you MUST have a call to OSIntExit()
*                 at the end of the ISR.
*
*              2) Rescheduling is prevented when the scheduler is locked (see OSSchedLock())
************************************************************************************************************************
*/

void  OSIntExit (void)
{
#if (OS_CFG_TASK_STK_REDZONE_EN > 0u)
    CPU_BOOLEAN  stk_status;
#endif
    CPU_SR_ALLOC();



    if (OSRunning != OS_STATE_OS_RUNNING) {                     /* Has the OS started?                                  */
        OS_TRACE_ISR_EXIT();
        return;                                                 /* No                                                   */
    }

    CPU_INT_DIS();
    if (OSIntNestingCtr == 0u) {                                /* Prevent OSIntNestingCtr from wrapping                */
        OS_TRACE_ISR_EXIT();
        CPU_INT_EN();
        return;
    }
    OSIntNestingCtr--;
    if (OSIntNestingCtr > 0u) {                                 /* ISRs still nested?                                   */
        OS_TRACE_ISR_EXIT();
        CPU_INT_EN();                                           /* Yes                                                  */
        return;
    }

    if (OSSchedLockNestingCtr > 0u) {                           /* Scheduler still locked?                              */
        OS_TRACE_ISR_EXIT();
        CPU_INT_EN();                                           /* Yes                                                  */
        return;
    }

                                                                /* Verify ISR Stack                                     */
#if (OS_CFG_ISR_STK_SIZE > 0u)
#if (OS_CFG_TASK_STK_REDZONE_EN > 0u)
    stk_status = OS_TaskStkRedzoneChk(OSCfg_ISRStkBasePtr, OSCfg_ISRStkSize);
    if (stk_status != OS_TRUE) {
        OSRedzoneHitHook((OS_TCB *)0);
    }
#endif
#endif

    OSPrioHighRdy   = OS_PrioGetHighest();                      /* Find highest priority                                */
#if (OS_CFG_TASK_IDLE_EN > 0u)
    OSTCBHighRdyPtr = OSRdyList[OSPrioHighRdy].HeadPtr;         /* Get highest priority task ready-to-run               */
    if (OSTCBHighRdyPtr == OSTCBCurPtr) {                       /* Current task still the highest priority?             */
                                                                /* Yes                                                  */
#if (OS_CFG_TASK_STK_REDZONE_EN > 0u)
        stk_status = OSTaskStkRedzoneChk((OS_TCB *)0);
        if (stk_status != OS_TRUE) {
            OSRedzoneHitHook(OSTCBCurPtr);
        }
#endif
        OS_TRACE_ISR_EXIT();
        CPU_INT_EN();
        OS_TRACE_TASK_SWITCHED_IN(OSTCBHighRdyPtr);             /* Do this here because we don't execute OSIntCtxSw().  */
        return;
    }
#else
    if (OSPrioHighRdy != (OS_CFG_PRIO_MAX - 1u)) {              /* Are we returning to idle?                            */
        OSTCBHighRdyPtr = OSRdyList[OSPrioHighRdy].HeadPtr;     /* No ... get highest priority task ready-to-run        */
        if (OSTCBHighRdyPtr == OSTCBCurPtr) {                   /* Current task still the highest priority?             */
                                                                /* Yes                                                  */
            OS_TRACE_ISR_EXIT();
            CPU_INT_EN();
            OS_TRACE_TASK_SWITCHED_IN(OSTCBHighRdyPtr);         /* Do this here because we don't execute OSIntCtxSw().  */
            return;
        }
    }
#endif

#if (OS_CFG_TASK_PROFILE_EN > 0u)
    OSTCBHighRdyPtr->CtxSwCtr++;                                /* Inc. # of context switches for this new task         */
#endif
#if ((OS_CFG_TASK_PROFILE_EN > 0u) || (OS_CFG_DBG_EN > 0u))
    OSTaskCtxSwCtr++;                                           /* Keep track of the total number of ctx switches       */
#endif

#if defined(OS_CFG_TLS_TBL_SIZE) && (OS_CFG_TLS_TBL_SIZE > 0u)
    OS_TLS_TaskSw();
#endif

    OS_TRACE_ISR_EXIT_TO_SCHEDULER();

    OSIntCtxSw();                                               /* Perform interrupt level ctx switch                   */

    CPU_INT_EN();
}


/*
************************************************************************************************************************
*                                    INDICATE THAT IT'S NO LONGER SAFE TO CREATE OBJECTS
*
* Description: This function is called by the application code to indicate that all initialization has been completed
*              and that kernel objects are no longer allowed to be created.
*
* Arguments  : none
*
* Returns    : none
*
* Note(s)    : none
************************************************************************************************************************
*/

#ifdef OS_SAFETY_CRITICAL_IEC61508
void  OSSafetyCriticalStart (void)
{
    OSSafetyCriticalStartFlag = OS_TRUE;
}

#endif


/*
************************************************************************************************************************
*                                                      SCHEDULER
*
* Description: This function is called by other uC/OS-III services to determine whether a new, high priority task has
*              been made ready to run.  This function is invoked by TASK level code and is not used to reschedule tasks
*              from ISRs (see OSIntExit() for ISR rescheduling).
*
* Arguments  : none
*
* Returns    : none
*
* Note(s)    : 1) Rescheduling is prevented when the scheduler is locked (see OSSchedLock())
************************************************************************************************************************
*/

void  OSSched (void)
{
    CPU_SR_ALLOC();


#if (OS_CFG_INVALID_OS_CALLS_CHK_EN > 0u)                       /* Can't schedule when the kernel is stopped.           */
    if (OSRunning != OS_STATE_OS_RUNNING) {
        return;
    }
#endif

    if (OSIntNestingCtr > 0u) {                                 /* ISRs still nested?                                   */
        return;                                                 /* Yes ... only schedule when no nested ISRs            */
    }

    if (OSSchedLockNestingCtr > 0u) {                           /* Scheduler locked?                                    */
        return;                                                 /* Yes                                                  */
    }

    CPU_INT_DIS();
    OSPrioHighRdy   = OS_PrioGetHighest();                      /* Find the highest priority ready                      */
#if (OS_CFG_TASK_IDLE_EN > 0u)
    OSTCBHighRdyPtr = OSRdyList[OSPrioHighRdy].HeadPtr;         /* Get highest priority task ready-to-run               */
    if (OSTCBHighRdyPtr == OSTCBCurPtr) {                       /* Current task still the highest priority?             */
        CPU_INT_EN();                                           /* Yes                                                  */
        return;
    }
#else
    if (OSPrioHighRdy != (OS_CFG_PRIO_MAX - 1u)) {              /* Are we returning to idle?                              */
        OSTCBHighRdyPtr = OSRdyList[OSPrioHighRdy].HeadPtr;     /* No ... get highest priority task ready-to-run          */
        if (OSTCBHighRdyPtr == OSTCBCurPtr) {                   /* Current task still the highest priority?               */
            CPU_INT_EN();                                       /* Yes                                                    */
            return;
        }
    }
#endif

    OS_TRACE_TASK_PREEMPT(OSTCBCurPtr);

#if (OS_CFG_TASK_PROFILE_EN > 0u)
    OSTCBHighRdyPtr->CtxSwCtr++;                                /* Inc. # of context switches to this task              */
#endif

#if ((OS_CFG_TASK_PROFILE_EN > 0u) || (OS_CFG_DBG_EN > 0u))
    OSTaskCtxSwCtr++;                                           /* Increment context switch counter                     */
#endif

#if defined(OS_CFG_TLS_TBL_SIZE) && (OS_CFG_TLS_TBL_SIZE > 0u)
    OS_TLS_TaskSw();
#endif

#if (OS_CFG_TASK_IDLE_EN > 0u)
    OS_TASK_SW();                                               /* Perform a task level context switch                  */
    CPU_INT_EN();
#else
    if ((OSPrioHighRdy != (OS_CFG_PRIO_MAX - 1u))) {
        OS_TASK_SW();                                           /* Perform a task level context switch                  */
        CPU_INT_EN();
    } else {
        OSTCBHighRdyPtr = OSTCBCurPtr;
        CPU_INT_EN();
        for (;;) {
#if ((OS_CFG_DBG_EN > 0u) || (OS_CFG_STAT_TASK_EN > 0u))
            CPU_CRITICAL_ENTER();
#if (OS_CFG_DBG_EN > 0u)
            OSIdleTaskCtr++;
#endif
#if (OS_CFG_STAT_TASK_EN > 0u)
            OSStatTaskCtr++;
#endif
            CPU_CRITICAL_EXIT();
#endif

#if (OS_CFG_APP_HOOKS_EN > 0u)
            OSIdleTaskHook();                                   /* Call user definable HOOK                             */
#endif
            if ((*((volatile OS_PRIO *)&OSPrioHighRdy) != (OS_CFG_PRIO_MAX - 1u))) {
                break;
            }
        }
    }
#endif

#ifdef OS_TASK_SW_SYNC
    OS_TASK_SW_SYNC();
#endif
}


/*
************************************************************************************************************************
*                                                 PREVENT SCHEDULING
*
* Description: This function is used to prevent rescheduling from taking place.  This allows your application to prevent
*              context switches until you are ready to permit context switching.
*
* Arguments  : p_err     is a pointer to a variable that will receive an error code:
*
*                            OS_ERR_NONE                 The scheduler is locked
*                            OS_ERR_LOCK_NESTING_OVF     If you attempted to nest call to this function > 250 levels
*                            OS_ERR_OS_NOT_RUNNING       If uC/OS-III is not running yet
*                            OS_ERR_SCHED_LOCK_ISR       If you called this function from an ISR
*
* Returns    : none
*
* Note(s)    : 1) You MUST invoke OSSchedLock() and OSSchedUnlock() in pair.  In other words, for every
*                 call to OSSchedLock() you MUST have a call to OSSchedUnlock().
************************************************************************************************************************
*/

void  OSSchedLock (OS_ERR  *p_err)
{
    CPU_SR_ALLOC();



#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return;
    }
#endif

#if (OS_CFG_CALLED_FROM_ISR_CHK_EN > 0u)
    if (OSIntNestingCtr > 0u) {                                 /* Not allowed to call from an ISR                      */
       *p_err = OS_ERR_SCHED_LOCK_ISR;
        return;
    }
#endif

    if (OSRunning != OS_STATE_OS_RUNNING) {                     /* Make sure multitasking is running                    */
       *p_err = OS_ERR_OS_NOT_RUNNING;
        return;
    }

    if (OSSchedLockNestingCtr >= 250u) {                        /* Prevent OSSchedLockNestingCtr overflowing            */
       *p_err = OS_ERR_LOCK_NESTING_OVF;
        return;
    }

    CPU_CRITICAL_ENTER();
    OSSchedLockNestingCtr++;                                    /* Increment lock nesting level                         */
#if (OS_CFG_SCHED_LOCK_TIME_MEAS_EN > 0u)
    OS_SchedLockTimeMeasStart();
#endif
    CPU_CRITICAL_EXIT();
   *p_err = OS_ERR_NONE;
}


/*
************************************************************************************************************************
*                                                  ENABLE SCHEDULING
*
* Description: This function is used to re-allow rescheduling.
*
* Arguments  : p_err     is a pointer to a variable that will contain an error code returned by this function.
*
*                            OS_ERR_NONE                 The scheduler has been enabled
*                            OS_ERR_OS_NOT_RUNNING       If uC/OS-III is not running yet
*                            OS_ERR_SCHED_LOCKED         The scheduler is still locked, still nested
*                            OS_ERR_SCHED_NOT_LOCKED     The scheduler was not locked
*                            OS_ERR_SCHED_UNLOCK_ISR     If you called this function from an ISR
*
* Returns    : none
*
* Note(s)    : 1) You MUST invoke OSSchedLock() and OSSchedUnlock() in pair.  In other words, for every call to
*                 OSSchedLock() you MUST have a call to OSSchedUnlock().
************************************************************************************************************************
*/

void  OSSchedUnlock (OS_ERR  *p_err)
{
    CPU_SR_ALLOC();



#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return;
    }
#endif

#if (OS_CFG_CALLED_FROM_ISR_CHK_EN > 0u)
    if (OSIntNestingCtr > 0u) {                                 /* Not allowed to call from an ISR                      */
       *p_err = OS_ERR_SCHED_UNLOCK_ISR;
        return;
    }
#endif

    if (OSRunning != OS_STATE_OS_RUNNING) {                     /* Make sure multitasking is running                    */
       *p_err = OS_ERR_OS_NOT_RUNNING;
        return;
    }

    if (OSSchedLockNestingCtr == 0u) {                          /* See if the scheduler is locked                       */
       *p_err = OS_ERR_SCHED_NOT_LOCKED;
        return;
    }

    CPU_CRITICAL_ENTER();
    OSSchedLockNestingCtr--;                                    /* Decrement lock nesting level                         */
    if (OSSchedLockNestingCtr > 0u) {
        CPU_CRITICAL_EXIT();                                    /* Scheduler is still locked                            */
       *p_err = OS_ERR_SCHED_LOCKED;
        return;
    }

#if (OS_CFG_SCHED_LOCK_TIME_MEAS_EN > 0u)
    OS_SchedLockTimeMeasStop();
#endif

    CPU_CRITICAL_EXIT();                                        /* Scheduler should be re-enabled                       */
    OSSched();                                                  /* Run the scheduler                                    */
   *p_err = OS_ERR_NONE;
}


/*
************************************************************************************************************************
*                                      CONFIGURE ROUND-ROBIN SCHEDULING PARAMETERS
*
* Description: This function is called to change the round-robin scheduling parameters.
*
* Arguments  : en                determines whether round-robin will be enabled (when OS_TRUE) or not (when OS_FALSE)
*
*              dflt_time_quanta  default number of ticks between time slices.  0 means OSCfg_TickRate_Hz / 10.
*
*              p_err             is a pointer to a variable that will contain an error code returned by this function.
*
*                                    OS_ERR_NONE    The call was successful
*
* Returns    : none
*
* Note(s)    : none
************************************************************************************************************************
*/

#if (OS_CFG_SCHED_ROUND_ROBIN_EN > 0u)
void  OSSchedRoundRobinCfg (CPU_BOOLEAN   en,
                            OS_TICK       dflt_time_quanta,
                            OS_ERR       *p_err)
{
    CPU_SR_ALLOC();



#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return;
    }
#endif

    CPU_CRITICAL_ENTER();
    if (en == 0u) {
        OSSchedRoundRobinEn = OS_FALSE;
    } else {
        OSSchedRoundRobinEn = OS_TRUE;
    }

    if (dflt_time_quanta > 0u) {
        OSSchedRoundRobinDfltTimeQuanta = dflt_time_quanta;
    } else {
        OSSchedRoundRobinDfltTimeQuanta = (OS_TICK)(OSCfg_TickRate_Hz / 10u);
    }
    CPU_CRITICAL_EXIT();
   *p_err = OS_ERR_NONE;
}
#endif


/*
************************************************************************************************************************
*                                    YIELD CPU WHEN TASK NO LONGER NEEDS THE TIME SLICE
*
* Description: This function is called to give up the CPU when a task is done executing before its time slice expires.
*
* Argument(s): p_err      is a pointer to a variable that will contain an error code returned by this function.
*
*                             OS_ERR_NONE                   The call was successful
*                             OS_ERR_ROUND_ROBIN_1          Only 1 task at this priority, nothing to yield to
*                             OS_ERR_ROUND_ROBIN_DISABLED   Round Robin is not enabled
*                             OS_ERR_SCHED_LOCKED           The scheduler has been locked
*                             OS_ERR_YIELD_ISR              Can't be called from an ISR
*
* Returns    : none
*
* Note(s)    : 1) This function MUST be called from a task.
************************************************************************************************************************
*/

#if (OS_CFG_SCHED_ROUND_ROBIN_EN > 0u)
void  OSSchedRoundRobinYield (OS_ERR  *p_err)
{
    OS_RDY_LIST  *p_rdy_list;
    OS_TCB       *p_tcb;
    CPU_SR_ALLOC();



#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return;
    }
#endif

#if (OS_CFG_CALLED_FROM_ISR_CHK_EN > 0u)
    if (OSIntNestingCtr > 0u) {                                 /* Can't call this function from an ISR                 */
       *p_err = OS_ERR_YIELD_ISR;
        return;
    }
#endif

    if (OSSchedLockNestingCtr > 0u) {                           /* Can't yield if the scheduler is locked               */
       *p_err = OS_ERR_SCHED_LOCKED;
        return;
    }

    if (OSSchedRoundRobinEn != OS_TRUE) {                       /* Make sure round-robin has been enabled               */
       *p_err = OS_ERR_ROUND_ROBIN_DISABLED;
        return;
    }

    CPU_CRITICAL_ENTER();
    p_rdy_list = &OSRdyList[OSPrioCur];                         /* Can't yield if it's the only task at that priority   */
    if (p_rdy_list->HeadPtr == p_rdy_list->TailPtr) {
        CPU_CRITICAL_EXIT();
       *p_err = OS_ERR_ROUND_ROBIN_1;
        return;
    }

    OS_RdyListMoveHeadToTail(p_rdy_list);                       /* Move current OS_TCB to the end of the list           */
    p_tcb = p_rdy_list->HeadPtr;                                /* Point to new OS_TCB at head of the list              */
    if (p_tcb->TimeQuanta == 0u) {                              /* See if we need to use the default time slice         */
        p_tcb->TimeQuantaCtr = OSSchedRoundRobinDfltTimeQuanta;
    } else {
        p_tcb->TimeQuantaCtr = p_tcb->TimeQuanta;               /* Load time slice counter with new time                */
    }

    CPU_CRITICAL_EXIT();

    OSSched();                                                  /* Run new task                                         */
   *p_err = OS_ERR_NONE;
}
#endif


/*
************************************************************************************************************************
*                                                 START MULTITASKING
*
* Description: This function is used to start the multitasking process which lets uC/OS-III manage the task that you
*              created.  Before you can call OSStart(), you MUST have called OSInit() and you MUST have created at least
*              one application task.
*
* Argument(s): p_err      is a pointer to a variable that will contain an error code returned by this function.
*
*                             OS_ERR_FATAL_RETURN    OS was running and OSStart() returned
*                             OS_ERR_OS_NOT_INIT     OS is not initialized, OSStart() has no effect
*                             OS_ERR_OS_NO_APP_TASK  No application task created, OSStart() has no effect
*                             OS_ERR_OS_RUNNING      OS is already running, OSStart() has no effect
*
* Returns    : none
*
* Note(s)    : 1) OSStartHighRdy() MUST:
*                 a) Call OSTaskSwHook() then,
*                 b) Load the context of the task pointed to by OSTCBHighRdyPtr.
*                 c) Execute the task.
*
*              2) OSStart() is not supposed to return.  If it does, that would be considered a fatal error.
************************************************************************************************************************
*/

void  OSStart (OS_ERR  *p_err)
{
    OS_OBJ_QTY  kernel_task_cnt;


#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return;
    }
#endif

    if (OSInitialized != OS_TRUE) {
       *p_err = OS_ERR_OS_NOT_INIT;
        return;
    }

    kernel_task_cnt = 0u;                                       /* Calculate the number of kernel tasks                 */
#if (OS_CFG_STAT_TASK_EN > 0u)
    kernel_task_cnt++;
#endif
#if (OS_CFG_TMR_EN > 0u)
    kernel_task_cnt++;
#endif
#if (OS_CFG_TASK_IDLE_EN > 0u)
    kernel_task_cnt++;
#endif

    if (OSTaskQty <= kernel_task_cnt) {                         /* No application task created                          */
        *p_err = OS_ERR_OS_NO_APP_TASK;
         return;
    }

    if (OSRunning == OS_STATE_OS_STOPPED) {
        OSPrioHighRdy   = OS_PrioGetHighest();                  /* Find the highest priority                            */
        OSPrioCur       = OSPrioHighRdy;
        OSTCBHighRdyPtr = OSRdyList[OSPrioHighRdy].HeadPtr;
        OSTCBCurPtr     = OSTCBHighRdyPtr;
        OSRunning       = OS_STATE_OS_RUNNING;
        OSStartHighRdy();                                       /* Execute target specific code to start task           */
       *p_err           = OS_ERR_FATAL_RETURN;                  /* OSStart() is not supposed to return                  */
    } else {
       *p_err           = OS_ERR_OS_RUNNING;                    /* OS is already running                                */
    }
}


/*
************************************************************************************************************************
*                                                    GET VERSION
*
* Description: This function is used to return the version number of uC/OS-III.  The returned value corresponds to
*              uC/OS-III's version number multiplied by 10000.  In other words, version 3.01.02 would be returned as 30102.
*
* Arguments  : p_err   is a pointer to a variable that will receive an error code.  However, OSVersion() set this
*                      variable to
*
*                         OS_ERR_NONE
*
* Returns    : The version number of uC/OS-III multiplied by 10000.
*
* Note(s)    : none
************************************************************************************************************************
*/

CPU_INT16U  OSVersion (OS_ERR  *p_err)
{
#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return (0u);
    }
#endif

   *p_err = OS_ERR_NONE;
    return (OS_VERSION);
}


/*
************************************************************************************************************************
*                                                      IDLE TASK
*
* Description: This task is internal to uC/OS-III and executes whenever no other higher priority tasks executes because
*              they are ALL waiting for event(s) to occur.
*
* Arguments  : p_arg    is an argument passed to the task when the task is created.
*
* Returns    : none
*
* Note(s)    : 1) This function is INTERNAL to uC/OS-III and your application MUST NOT call it.
*
*              2) OSIdleTaskHook() is called after the critical section to ensure that interrupts will be enabled for at
*                 least a few instructions.  On some processors (ex. Philips XA), enabling and then disabling interrupts
*                 doesn't allow the processor enough time to have interrupts enabled before they were disabled again.
*                 uC/OS-III would thus never recognize interrupts.
*
*              3) This hook has been added to allow you to do such things as STOP the CPU to conserve power.
************************************************************************************************************************
*/
#if (OS_CFG_TASK_IDLE_EN > 0u)
void  OS_IdleTask (void  *p_arg)
{
#if ((OS_CFG_DBG_EN > 0u) || (OS_CFG_STAT_TASK_EN > 0u))
    CPU_SR_ALLOC();
#endif


    (void)p_arg;                                                /* Prevent compiler warning for not using 'p_arg'       */

    for (;;) {
#if ((OS_CFG_DBG_EN > 0u) || (OS_CFG_STAT_TASK_EN > 0u))
        CPU_CRITICAL_ENTER();
#if (OS_CFG_DBG_EN > 0u)
        OSIdleTaskCtr++;
#endif
#if (OS_CFG_STAT_TASK_EN > 0u)
        OSStatTaskCtr++;
#endif
        CPU_CRITICAL_EXIT();
#endif

#if (OS_CFG_APP_HOOKS_EN > 0u)
        OSIdleTaskHook();                                       /* Call user definable HOOK                             */
#endif
    }
}
#endif

/*
************************************************************************************************************************
*                                               INITIALIZE THE IDLE TASK
*
* Description: This function initializes the idle task
*
* Arguments  : p_err    is a pointer to a variable that will contain an error code returned by this function.
*
* Returns    : none
*
* Note(s)    : 1) This function is INTERNAL to uC/OS-III and your application MUST NOT call it.
************************************************************************************************************************
*/
#if (OS_CFG_TASK_IDLE_EN > 0u)
void  OS_IdleTaskInit (OS_ERR  *p_err)
{
#if (OS_CFG_DBG_EN > 0u)
    OSIdleTaskCtr = 0u;
#endif
                                                                /* --------------- CREATE THE IDLE TASK --------------- */
    OSTaskCreate(&OSIdleTaskTCB,
#if  (OS_CFG_DBG_EN == 0u)
                 (CPU_CHAR   *)0,
#else
                 (CPU_CHAR   *)"uC/OS-III Idle Task",
#endif
                  OS_IdleTask,
                 (void       *)0,
                 (OS_PRIO     )(OS_CFG_PRIO_MAX - 1u),
                  OSCfg_IdleTaskStkBasePtr,
                  OSCfg_IdleTaskStkLimit,
                  OSCfg_IdleTaskStkSize,
                  0u,
                  0u,
                 (void       *)0,
                 (OS_OPT_TASK_STK_CHK | (OS_OPT)(OS_OPT_TASK_STK_CLR | OS_OPT_TASK_NO_TLS)),
                  p_err);
}
#endif

/*
************************************************************************************************************************
*                                             BLOCK A TASK PENDING ON EVENT
*
* Description: This function is called to place a task in the blocked state waiting for an event to occur. This function
*              exists because it is common to a number of OSxxxPend() services.
*
* Arguments  : p_obj          is a pointer to the object to pend on.  If there are no object used to pend on then
*              -----          the caller must pass a NULL pointer.
*
*              p_tcb          is the task that will be blocked.
*
*              pending_on     Specifies what the task will be pending on:
*
*                                 OS_TASK_PEND_ON_FLAG
*                                 OS_TASK_PEND_ON_TASK_Q     <- No object (pending for a message sent to the task)
*                                 OS_TASK_PEND_ON_MUTEX
*                                 OS_TASK_PEND_ON_COND
*                                 OS_TASK_PEND_ON_Q
*                                 OS_TASK_PEND_ON_SEM
*                                 OS_TASK_PEND_ON_TASK_SEM   <- No object (pending on a signal sent to the task)
*
*              timeout        Is the amount of time the task will wait for the event to occur.
*
* Returns    : none
*
* Note(s)    : 1) This function is INTERNAL to uC/OS-III and your application MUST NOT call it.
************************************************************************************************************************
*/

void  OS_Pend (OS_PEND_OBJ  *p_obj,
               OS_TCB       *p_tcb,
               OS_STATE      pending_on,
               OS_TICK       timeout)
{
    OS_PEND_LIST  *p_pend_list;


    p_tcb->PendOn     = pending_on;                             /* Resource not available, wait until it is             */
    p_tcb->PendStatus = OS_STATUS_PEND_OK;

    OS_TaskBlock(p_tcb,                                         /* Block the task and add it to the tick list if needed */
                 timeout);

    if (p_obj != (OS_PEND_OBJ *)0) {                            /* Add the current task to the pend list ...            */
        p_pend_list             = &p_obj->PendList;             /* ... if there is an object to pend on                 */
        p_tcb->PendObjPtr =  p_obj;                             /* Save the pointer to the object pending on            */
        OS_PendListInsertPrio(p_pend_list,                      /* Insert in the pend list in priority order            */
                              p_tcb);

    } else {
        p_tcb->PendObjPtr = (OS_PEND_OBJ *)0;                   /* If no object being pended on, clear the pend object  */
    }
#if (OS_CFG_DBG_EN > 0u)
    OS_PendDbgNameAdd(p_obj,
                      p_tcb);
#endif
}


/*
************************************************************************************************************************
*                                                    CANCEL PENDING
*
* Description: This function is called by the OSxxxPendAbort() and OSxxxDel() functions to cancel pending on an event.
*
* Arguments  : p_tcb          Is a pointer to the OS_TCB of the task that we'll abort the pend for
*              -----
*
*              ts             Is a timestamp as to when the pend was cancelled
*
*              reason         Indicates how the task was readied:
*
*                                 OS_STATUS_PEND_DEL       Object pended on was deleted.
*                                 OS_STATUS_PEND_ABORT     Pend was aborted.
*
* Returns    : none
*
* Note(s)    : 1) This function is INTERNAL to uC/OS-III and your application MUST NOT call it.
************************************************************************************************************************
*/

void  OS_PendAbort (OS_TCB     *p_tcb,
                    CPU_TS      ts,
                    OS_STATUS   reason)
{
#if (OS_CFG_TS_EN == 0u)
    (void)ts;                                                   /* Prevent compiler warning for not using 'ts'          */
#endif

    switch (p_tcb->TaskState) {
        case OS_TASK_STATE_PEND:
        case OS_TASK_STATE_PEND_TIMEOUT:
#if (OS_MSG_EN > 0u)
             p_tcb->MsgPtr     = (void *)0;
             p_tcb->MsgSize    =         0u;
#endif
#if (OS_CFG_TS_EN > 0u)
             p_tcb->TS         = ts;
#endif
             OS_PendListRemove(p_tcb);                          /* Remove task from the pend list                       */

#if (OS_CFG_TICK_EN > 0u)
             if (p_tcb->TaskState == OS_TASK_STATE_PEND_TIMEOUT) {
                 OS_TickListRemove(p_tcb);                      /* Cancel the timeout                                   */
             }
#endif
             OS_RdyListInsert(p_tcb);                           /* Insert the task in the ready list                    */
             p_tcb->TaskState  = OS_TASK_STATE_RDY;             /* Task will be ready                                   */
             p_tcb->PendStatus = reason;                        /* Indicate how the task became ready                   */
             p_tcb->PendOn     = OS_TASK_PEND_ON_NOTHING;       /* Indicate no longer pending                           */
             break;

        case OS_TASK_STATE_PEND_SUSPENDED:
        case OS_TASK_STATE_PEND_TIMEOUT_SUSPENDED:
#if (OS_MSG_EN > 0u)
             p_tcb->MsgPtr     = (void *)0;
             p_tcb->MsgSize    =         0u;
#endif
#if (OS_CFG_TS_EN > 0u)
             p_tcb->TS         = ts;
#endif
             OS_PendListRemove(p_tcb);                          /* Remove task from the pend list                       */

#if (OS_CFG_TICK_EN > 0u)
             if (p_tcb->TaskState == OS_TASK_STATE_PEND_TIMEOUT_SUSPENDED) {
                 OS_TickListRemove(p_tcb);                      /* Cancel the timeout                                   */
             }
#endif
             p_tcb->TaskState  = OS_TASK_STATE_SUSPENDED;       /* Task needs to remain suspended                       */
             p_tcb->PendStatus = reason;                        /* Indicate how the task became ready                   */
             p_tcb->PendOn     = OS_TASK_PEND_ON_NOTHING;       /* Indicate no longer pending                           */
             break;

        case OS_TASK_STATE_RDY:                                 /* Cannot cancel a pend when a task is in these states. */
        case OS_TASK_STATE_DLY:
        case OS_TASK_STATE_SUSPENDED:
        case OS_TASK_STATE_DLY_SUSPENDED:
        default:
                                                                /* Default case.                                        */
             break;
    }
}


/*
************************************************************************************************************************
*                                     ADD/REMOVE DEBUG NAMES TO PENDED OBJECT AND OS_TCB
*
* Description: These functions are used to add pointers to ASCII 'names' of objects so they can easily be displayed
*              using a kernel aware tool.
*
* Arguments  : p_obj              is a pointer to the object being pended on
*
*              p_tcb              is a pointer to the OS_TCB of the task pending on the object
*
* Returns    : none
*
* Note(s)    : 1) These functions are INTERNAL to uC/OS-III and your application must not call it.
************************************************************************************************************************
*/

#if (OS_CFG_DBG_EN > 0u)
void  OS_PendDbgNameAdd (OS_PEND_OBJ  *p_obj,
                         OS_TCB       *p_tcb)
{
    OS_PEND_LIST  *p_pend_list;
    OS_TCB        *p_tcb1;


    if (p_obj != (OS_PEND_OBJ *)0) {
        p_tcb->DbgNamePtr =  p_obj->NamePtr;                    /* Task pending on this object ... save name in TCB     */
        p_pend_list       = &p_obj->PendList;                   /* Find name of HP task pending on this object ...      */
        p_tcb1            =  p_pend_list->HeadPtr;
        p_obj->DbgNamePtr =  p_tcb1->NamePtr;                   /* ... Save in object                                   */
    } else {
        switch (p_tcb->PendOn) {
            case OS_TASK_PEND_ON_TASK_Q:
                 p_tcb->DbgNamePtr = (CPU_CHAR *)((void *)"Task Q");
                 break;

            case OS_TASK_PEND_ON_TASK_SEM:
                 p_tcb->DbgNamePtr = (CPU_CHAR *)((void *)"Task Sem");
                 break;

            default:
                 p_tcb->DbgNamePtr = (CPU_CHAR *)((void *)" ");
                 break;
        }
    }
}


void  OS_PendDbgNameRemove (OS_PEND_OBJ  *p_obj,
                            OS_TCB       *p_tcb)
{
    OS_PEND_LIST  *p_pend_list;
    OS_TCB        *p_tcb1;


    p_tcb->DbgNamePtr = (CPU_CHAR *)((void *)" ");              /* Remove name of object pended on for readied task     */

    if (p_obj != (OS_PEND_OBJ *)0) {
        p_pend_list = &p_obj->PendList;
        p_tcb1      =  p_pend_list->HeadPtr;
        if (p_tcb1 != (OS_TCB *)0) {                            /* Find name of HP task pending on this object ...      */
            p_obj->DbgNamePtr = p_tcb1->NamePtr;                /* ... Save in object                                   */
        } else {
            p_obj->DbgNamePtr = (CPU_CHAR *)((void *)" ");      /* Or no other task is pending on object                */
        }
    }
}
#endif


/*
************************************************************************************************************************
*                                 CHANGE THE PRIORITY OF A TASK WAITING IN A PEND LIST
*
* Description: This function is called to change the position of a task waiting in a pend list. The strategy used is to
*              remove the task from the pend list and add it again using its changed priority.
*
* Arguments  : p_tcb       is a pointer to the TCB of the task to move
*              -----
*
* Returns    : none
*
* Note(s)    : 1) This function is INTERNAL to uC/OS-III and your application MUST NOT call it.
*
*              2) It's assumed that the TCB contains the NEW priority in its .Prio field.
************************************************************************************************************************
*/

void  OS_PendListChangePrio (OS_TCB  *p_tcb)
{
    OS_PEND_LIST  *p_pend_list;
    OS_PEND_OBJ   *p_obj;

    p_obj       =  p_tcb->PendObjPtr;                           /* Get pointer to pend list                             */
    p_pend_list = &p_obj->PendList;

    if (p_pend_list->HeadPtr->PendNextPtr != (OS_TCB *)0) {     /* Only move if multiple entries in the list            */
            OS_PendListRemove(p_tcb);                           /* Remove entry from current position                   */
            p_tcb->PendObjPtr = p_obj;
            OS_PendListInsertPrio(p_pend_list,                  /* INSERT it back in the list                           */
                                  p_tcb);
    }
}


/*
************************************************************************************************************************
*                                                INITIALIZE A WAIT LIST
*
* Description: This function is called to initialize the fields of an OS_PEND_LIST.
*
* Arguments  : p_pend_list   is a pointer to an OS_PEND_LIST
*              -----------
*
* Returns    : none
*
* Note(s)    : 1) This function is INTERNAL to uC/OS-III and your application must not call it.
************************************************************************************************************************
*/

void  OS_PendListInit (OS_PEND_LIST  *p_pend_list)
{
    p_pend_list->HeadPtr    = (OS_TCB *)0;
    p_pend_list->TailPtr    = (OS_TCB *)0;
#if (OS_CFG_DBG_EN > 0u)
    p_pend_list->NbrEntries =           0u;
#endif
}


/*
************************************************************************************************************************
*                                  INSERT A TASK BASED ON IT'S PRIORITY IN A PEND LIST
*
* Description: This function is called to place an OS_TCB entry in a linked list based on its priority.  The
*              highest priority being placed at the head of the list. The TCB is assumed to contain the priority
*              of the task in its .Prio field.
*
*              CASE 0: Insert in an empty list.
*
*                     OS_PEND_LIST
*                     +---------------+
*                     | TailPtr       |-> 0
*                     +---------------+
*                     | HeadPtr       |-> 0
*                     +---------------+
*                     | NbrEntries=0  |
*                     +---------------+
*
*
*
*              CASE 1: Insert BEFORE or AFTER an OS_TCB
*
*                     OS_PEND_LIST
*                     +--------------+         OS_TCB
*                     | TailPtr      |--+---> +--------------+
*                     +--------------+  |     | PendNextPtr  |->0
*                     | HeadPtr      |--/     +--------------+
*                     +--------------+     0<-| PendPrevPtr  |
*                     | NbrEntries=1 |        +--------------+
*                     +--------------+        |              |
*                                             +--------------+
*                                             |              |
*                                             +--------------+
*
*
*                     OS_PEND_LIST
*                     +--------------+
*                     | TailPtr      |---------------------------------------------------+
*                     +--------------+         OS_TCB                 OS_TCB             |    OS_TCB
*                     | HeadPtr      |------> +--------------+       +--------------+    +-> +--------------+
*                     +--------------+        | PendNextPtr  |<------| PendNextPtr  | ...... | PendNextPtr  |->0
*                     | NbrEntries=N |        +--------------+       +--------------+        +--------------+
*                     +--------------+     0<-| PendPrevPtr  |<------| PendPrevPtr  | ...... | PendPrevPtr  |
*                                             +--------------+       +--------------+        +--------------+
*                                             |              |       |              |        |              |
*                                             +--------------+       +--------------+        +--------------+
*                                             |              |       |              |        |              |
*                                             +--------------+       +--------------+        +--------------+
*
*
* Arguments  : p_pend_list    is a pointer to the OS_PEND_LIST where the OS_TCB entry will be inserted
*              -----------
*
*              p_tcb          is the OS_TCB to insert in the list
*              -----
*
* Returns    : none
*
* Note(s)    : 1) This function is INTERNAL to uC/OS-III and your application MUST NOT call it.
************************************************************************************************************************
*/

void  OS_PendListInsertPrio (OS_PEND_LIST  *p_pend_list,
                             OS_TCB        *p_tcb)
{
    OS_PRIO   prio;
    OS_TCB   *p_tcb_next;


    prio  = p_tcb->Prio;                                        /* Obtain the priority of the task to insert            */

    if (p_pend_list->HeadPtr == (OS_TCB *)0) {                  /* CASE 0: Insert when there are no entries             */
#if (OS_CFG_DBG_EN > 0u)
        p_pend_list->NbrEntries = 1u;                           /* This is the first entry                              */
#endif
        p_tcb->PendNextPtr   = (OS_TCB *)0;                     /* No other OS_TCBs in the list                         */
        p_tcb->PendPrevPtr   = (OS_TCB *)0;
        p_pend_list->HeadPtr =  p_tcb;
        p_pend_list->TailPtr =  p_tcb;
    } else {
#if (OS_CFG_DBG_EN > 0u)
        p_pend_list->NbrEntries++;                              /* CASE 1: One more OS_TCBs in the list                 */
#endif
        p_tcb_next = p_pend_list->HeadPtr;
        while (p_tcb_next != (OS_TCB *)0) {                     /* Find the position where to insert                    */
            if (prio < p_tcb_next->Prio) {
                break;                                          /* Found! ... insert BEFORE current                     */
            } else {
                p_tcb_next = p_tcb_next->PendNextPtr;           /* Not Found, follow the list                           */
            }
        }
        if (p_tcb_next == (OS_TCB *)0) {                        /* TCB to insert is lowest in priority                  */
            p_tcb->PendNextPtr              = (OS_TCB *)0;      /* ... insert at the tail.                              */
            p_tcb->PendPrevPtr              =  p_pend_list->TailPtr;
            p_tcb->PendPrevPtr->PendNextPtr =  p_tcb;
            p_pend_list->TailPtr            =  p_tcb;
        } else {
            if (p_tcb_next->PendPrevPtr == (OS_TCB *)0) {       /* Is new TCB highest priority?                         */
                p_tcb->PendNextPtr      =  p_tcb_next;          /* Yes, insert as new Head of list                      */
                p_tcb->PendPrevPtr      = (OS_TCB *)0;
                p_tcb_next->PendPrevPtr =  p_tcb;
                p_pend_list->HeadPtr    =  p_tcb;
            } else {                                            /* No,  insert in between two entries                   */
                p_tcb->PendNextPtr              = p_tcb_next;
                p_tcb->PendPrevPtr              = p_tcb_next->PendPrevPtr;
                p_tcb->PendPrevPtr->PendNextPtr = p_tcb;
                p_tcb_next->PendPrevPtr         = p_tcb;
            }
        }
    }
}


/*
************************************************************************************************************************
*                           REMOVE TASK FROM A PEND LIST KNOWING ONLY WHICH TCB TO REMOVE
*
* Description: This function is called to remove a task from a pend list knowing the TCB of the task to remove.
*
*              CASE 0: OS_PEND_LIST list is empty, nothing to do.
*
*              CASE 1: Only 1 OS_TCB in the list.
*
*                     OS_PEND_LIST
*                     +--------------+         OS_TCB
*                     | TailPtr      |--+---> +--------------+
*                     +--------------+  |     | PendNextPtr  |->0
*                     | HeadPtr      |--/     +--------------+
*                     +--------------+     0<-| PendPrevPtr  |
*                     | NbrEntries=1 |        +--------------+
*                     +--------------+        |              |
*                                             +--------------+
*                                             |              |
*                                             +--------------+
*
*              CASE N: Two or more OS_TCBs in the list.
*
*                     OS_PEND_LIST
*                     +--------------+
*                     | TailPtr      |---------------------------------------------------+
*                     +--------------+         OS_TCB                 OS_TCB             |    OS_TCB
*                     | HeadPtr      |------> +--------------+       +--------------+    +-> +--------------+
*                     +--------------+        | PendNextPtr  |<------| PendNextPtr  | ...... | PendNextPtr  |->0
*                     | NbrEntries=N |        +--------------+       +--------------+        +--------------+
*                     +--------------+     0<-| PendPrevPtr  |<------| PendPrevPtr  | ...... | PendPrevPtr  |
*                                             +--------------+       +--------------+        +--------------+
*                                             |              |       |              |        |              |
*                                             +--------------+       +--------------+        +--------------+
*                                             |              |       |              |        |              |
*                                             +--------------+       +--------------+        +--------------+
*
*
* Arguments  : p_tcb          is a pointer to the TCB of the task to remove from the pend list
*              -----
*
* Returns    : none
*
* Note(s)    : 1) This function is INTERNAL to uC/OS-III and your application MUST NOT call it.
************************************************************************************************************************
*/

void  OS_PendListRemove (OS_TCB  *p_tcb)
{
    OS_PEND_LIST  *p_pend_list;
    OS_TCB        *p_next;
    OS_TCB        *p_prev;


    if (p_tcb->PendObjPtr != (OS_PEND_OBJ *)0) {                /* Only remove if object has a pend list.               */
        p_pend_list = &p_tcb->PendObjPtr->PendList;             /* Get pointer to pend list                             */

                                                                /* Remove TCB from the pend list.                       */
        if (p_pend_list->HeadPtr->PendNextPtr == (OS_TCB *)0) {
            p_pend_list->HeadPtr = (OS_TCB *)0;                 /* Only one entry in the pend list                      */
            p_pend_list->TailPtr = (OS_TCB *)0;
        } else if (p_tcb->PendPrevPtr == (OS_TCB *)0) {         /* See if entry is at the head of the list              */
            p_next               =  p_tcb->PendNextPtr;         /* Yes                                                  */
            p_next->PendPrevPtr  = (OS_TCB *)0;
            p_pend_list->HeadPtr =  p_next;

        } else if (p_tcb->PendNextPtr == (OS_TCB *)0) {         /* See if entry is at the tail of the list              */
            p_prev               =  p_tcb->PendPrevPtr;         /* Yes                                                  */
            p_prev->PendNextPtr  = (OS_TCB *)0;
            p_pend_list->TailPtr =  p_prev;

        } else {
            p_prev               = p_tcb->PendPrevPtr;          /* Remove from inside the list                          */
            p_next               = p_tcb->PendNextPtr;
            p_prev->PendNextPtr  = p_next;
            p_next->PendPrevPtr  = p_prev;
        }
#if (OS_CFG_DBG_EN > 0u)
        p_pend_list->NbrEntries--;                              /* One less entry in the list                           */
#endif
        p_tcb->PendNextPtr = (OS_TCB      *)0;
        p_tcb->PendPrevPtr = (OS_TCB      *)0;
        p_tcb->PendObjPtr  = (OS_PEND_OBJ *)0;
    }
}


/*
************************************************************************************************************************
*                                                   POST TO A TASK
*
* Description: This function is called to post to a task.  This function exist because it is common to a number of
*              OSxxxPost() services.
*
* Arguments  : p_obj          Is a pointer to the object being posted to or NULL pointer if there is no object
*              -----
*
*              p_tcb          Is a pointer to the OS_TCB that will receive the 'post'
*              -----
*
*              p_void         If we are posting a message to a task, this is the message that the task will receive
*
*              msg_size       If we are posting a message to a task, this is the size of the message
*
*              ts             The timestamp as to when the post occurred
*
* Returns    : none
*
* Note(s)    : This function is INTERNAL to uC/OS-III and your application should not call it.
************************************************************************************************************************
*/

void  OS_Post (OS_PEND_OBJ  *p_obj,
               OS_TCB       *p_tcb,
               void         *p_void,
               OS_MSG_SIZE   msg_size,
               CPU_TS        ts)
{
#if (OS_CFG_TS_EN == 0u)
    (void)ts;                                                   /* Prevent compiler warning for not using 'ts'          */
#endif
#if (OS_MSG_EN == 0u)
    (void)p_void;
    (void)msg_size;
#endif

    switch (p_tcb->TaskState) {
        case OS_TASK_STATE_RDY:                                 /* Cannot Post a task that is ready                     */
        case OS_TASK_STATE_DLY:                                 /* Cannot Post a task that is delayed                   */
        case OS_TASK_STATE_SUSPENDED:                           /* Cannot Post a suspended task                         */
        case OS_TASK_STATE_DLY_SUSPENDED:                       /* Cannot Post a suspended task that was also dly'd     */
             break;

        case OS_TASK_STATE_PEND:
        case OS_TASK_STATE_PEND_TIMEOUT:
#if (OS_MSG_EN > 0u)
             p_tcb->MsgPtr  = p_void;                           /* Deposit message in OS_TCB of task waiting            */
             p_tcb->MsgSize = msg_size;                         /* ... assuming posting a message                       */
#endif
#if (OS_CFG_TS_EN > 0u)
                 p_tcb->TS      = ts;
#endif
             if (p_obj != (OS_PEND_OBJ *)0) {
                 OS_PendListRemove(p_tcb);                      /* Remove task from pend list                           */
             }
#if (OS_CFG_DBG_EN > 0u)
             OS_PendDbgNameRemove(p_obj,
                                  p_tcb);
#endif
#if (OS_CFG_TICK_EN > 0u)
             if (p_tcb->TaskState == OS_TASK_STATE_PEND_TIMEOUT) {
                 OS_TickListRemove(p_tcb);                      /* Remove from tick list                                */
             }
#endif
             OS_RdyListInsert(p_tcb);                           /* Insert the task in the ready list                    */
             p_tcb->TaskState  = OS_TASK_STATE_RDY;
             p_tcb->PendStatus = OS_STATUS_PEND_OK;             /* Clear pend status                                    */
             p_tcb->PendOn     = OS_TASK_PEND_ON_NOTHING;       /* Indicate no longer pending                           */
             break;

        case OS_TASK_STATE_PEND_SUSPENDED:
        case OS_TASK_STATE_PEND_TIMEOUT_SUSPENDED:
#if (OS_MSG_EN > 0u)
             p_tcb->MsgPtr  = p_void;                           /* Deposit message in OS_TCB of task waiting            */
             p_tcb->MsgSize = msg_size;                         /* ... assuming posting a message                       */
#endif
#if (OS_CFG_TS_EN > 0u)
             p_tcb->TS      = ts;
#endif
             if (p_obj != (OS_PEND_OBJ *)0) {
                 OS_PendListRemove(p_tcb);                      /* Remove from pend list                                */
             }
#if (OS_CFG_DBG_EN > 0u)
             OS_PendDbgNameRemove(p_obj,
                                  p_tcb);
#endif
#if (OS_CFG_TICK_EN > 0u)
             if (p_tcb->TaskState == OS_TASK_STATE_PEND_TIMEOUT_SUSPENDED) {
                 OS_TickListRemove(p_tcb);                      /* Cancel any timeout                                   */
             }
#endif
             p_tcb->TaskState  = OS_TASK_STATE_SUSPENDED;
             p_tcb->PendStatus = OS_STATUS_PEND_OK;             /* Clear pend status                                    */
             p_tcb->PendOn     = OS_TASK_PEND_ON_NOTHING;       /* Indicate no longer pending                           */
             break;

        default:
                                                                /* Default case.                                        */
             break;
    }
}


/*
************************************************************************************************************************
*                                                    INITIALIZATION
*                                               READY LIST INITIALIZATION
*
* Description: This function is called by OSInit() to initialize the ready list.  The ready list contains a list of all
*              the tasks that are ready to run.  The list is actually an array of OS_RDY_LIST.  An OS_RDY_LIST contains
*              three fields.  The number of OS_TCBs in the list (i.e. .NbrEntries), a pointer to the first OS_TCB in the
*              OS_RDY_LIST (i.e. .HeadPtr) and a pointer to the last OS_TCB in the OS_RDY_LIST (i.e. .TailPtr).
*
*              OS_TCBs are doubly linked in the OS_RDY_LIST and each OS_TCB points pack to the OS_RDY_LIST it belongs
*              to.
*
*              'OS_RDY_LIST  OSRdyTbl[OS_CFG_PRIO_MAX]'  looks like this once initialized:
*
*                               +---------------+--------------+
*                               |               | TailPtr      |-----> 0
*                          [0]  | NbrEntries=0  +--------------+
*                               |               | HeadPtr      |-----> 0
*                               +---------------+--------------+
*                               |               | TailPtr      |-----> 0
*                          [1]  | NbrEntries=0  +--------------+
*                               |               | HeadPtr      |-----> 0
*                               +---------------+--------------+
*                                       :              :
*                                       :              :
*                                       :              :
*                               +---------------+--------------+
*                               |               | TailPtr      |-----> 0
*          [OS_CFG_PRIO_MAX-1]  | NbrEntries=0  +--------------+
*                               |               | HeadPtr      |-----> 0
*                               +---------------+--------------+
*
*
* Arguments  : none
*
* Returns    : none
*
* Note(s)    : This function is INTERNAL to uC/OS-III and your application should not call it.
************************************************************************************************************************
*/

void  OS_RdyListInit (void)
{
    CPU_INT32U    i;
    OS_RDY_LIST  *p_rdy_list;



    for (i = 0u; i < OS_CFG_PRIO_MAX; i++) {                    /* Initialize the array of OS_RDY_LIST at each priority */
        p_rdy_list = &OSRdyList[i];
#if (OS_CFG_DBG_EN > 0u)
        p_rdy_list->NbrEntries =           0u;
#endif
        p_rdy_list->HeadPtr    = (OS_TCB *)0;
        p_rdy_list->TailPtr    = (OS_TCB *)0;
    }
}


/*
************************************************************************************************************************
*                                             INSERT TCB IN THE READY LIST
*
* Description: This function is called to insert a TCB in the ready list.
*
*              The TCB is inserted at the tail of the list if the priority of the TCB is the same as the priority of the
*              current task.  The TCB is inserted at the head of the list if not.
*
* Arguments  : p_tcb     is a pointer to the TCB to insert into the ready list
*              -----
*
* Returns    : none
*
* Note(s)    : This function is INTERNAL to uC/OS-III and your application should not call it.
************************************************************************************************************************
*/

void  OS_RdyListInsert (OS_TCB  *p_tcb)
{
    OS_PrioInsert(p_tcb->Prio);
    if (p_tcb->Prio == OSPrioCur) {                             /* Are we readying a task at the same prio?             */
        OS_RdyListInsertTail(p_tcb);                            /* Yes, insert readied task at the end of the list      */
    } else {
        OS_RdyListInsertHead(p_tcb);                            /* No,  insert readied task at the beginning of the list*/
    }

    OS_TRACE_TASK_READY(p_tcb);
}


/*
************************************************************************************************************************
*                                          INSERT TCB AT THE BEGINNING OF A LIST
*
* Description: This function is called to place an OS_TCB at the beginning of a linked list as follows:
*
*              CASE 0: Insert in an empty list.
*
*                     OS_RDY_LIST
*                     +--------------+
*                     | TailPtr      |-> 0
*                     +--------------+
*                     | HeadPtr      |-> 0
*                     +--------------+
*                     | NbrEntries=0 |
*                     +--------------+
*
*
*
*              CASE 1: Insert BEFORE the current head of list
*
*                     OS_RDY_LIST
*                     +--------------+          OS_TCB
*                     | TailPtr      |--+---> +------------+
*                     +--------------+  |     | NextPtr    |->0
*                     | HeadPtr      |--/     +------------+
*                     +--------------+     0<-| PrevPtr    |
*                     | NbrEntries=1 |        +------------+
*                     +--------------+        :            :
*                                             :            :
*                                             +------------+
*
*
*                     OS_RDY_LIST
*                     +--------------+
*                     | TailPtr      |-----------------------------------------------+
*                     +--------------+          OS_TCB               OS_TCB          |     OS_TCB
*                     | HeadPtr      |------> +------------+       +------------+    +-> +------------+
*                     +--------------+        | NextPtr    |------>| NextPtr    | ...... | NextPtr    |->0
*                     | NbrEntries=N |        +------------+       +------------+        +------------+
*                     +--------------+     0<-| PrevPtr    |<------| PrevPtr    | ...... | PrevPtr    |
*                                             +------------+       +------------+        +------------+
*                                             :            :       :            :        :            :
*                                             :            :       :            :        :            :
*                                             +------------+       +------------+        +------------+
*
*
* Arguments  : p_tcb     is the OS_TCB to insert in the list
*              -----
*
* Returns    : none
*
* Note(s)    : 1) This function is INTERNAL to uC/OS-III and your application MUST NOT call it.
************************************************************************************************************************
*/

void  OS_RdyListInsertHead (OS_TCB  *p_tcb)
{
    OS_RDY_LIST  *p_rdy_list;
    OS_TCB       *p_tcb2;



    p_rdy_list = &OSRdyList[p_tcb->Prio];
    if (p_rdy_list->HeadPtr == (OS_TCB *)0) {                   /* CASE 0: Insert when there are no entries             */
#if (OS_CFG_DBG_EN > 0u)
        p_rdy_list->NbrEntries =           1u;                  /* This is the first entry                              */
#endif
        p_tcb->NextPtr         = (OS_TCB *)0;                   /* No other OS_TCBs in the list                         */
        p_tcb->PrevPtr         = (OS_TCB *)0;
        p_rdy_list->HeadPtr    =  p_tcb;                        /* Both list pointers point to this OS_TCB              */
        p_rdy_list->TailPtr    =  p_tcb;
    } else {                                                    /* CASE 1: Insert BEFORE the current head of list       */
#if (OS_CFG_DBG_EN > 0u)
        p_rdy_list->NbrEntries++;                               /* One more OS_TCB in the list                          */
#endif
        p_tcb->NextPtr         =  p_rdy_list->HeadPtr;          /* Adjust new OS_TCBs links                             */
        p_tcb->PrevPtr         = (OS_TCB *)0;
        p_tcb2                 =  p_rdy_list->HeadPtr;          /* Adjust old head of list's links                      */
        p_tcb2->PrevPtr        =  p_tcb;
        p_rdy_list->HeadPtr    =  p_tcb;
    }
}


/*
************************************************************************************************************************
*                                           INSERT TCB AT THE END OF A LIST
*
* Description: This function is called to place an OS_TCB at the end of a linked list as follows:
*
*              CASE 0: Insert in an empty list.
*
*                     OS_RDY_LIST
*                     +--------------+
*                     | TailPtr      |-> 0
*                     +--------------+
*                     | HeadPtr      |-> 0
*                     +--------------+
*                     | NbrEntries=0 |
*                     +--------------+
*
*
*
*              CASE 1: Insert AFTER the current tail of list
*
*                     OS_RDY_LIST
*                     +--------------+          OS_TCB
*                     | TailPtr      |--+---> +------------+
*                     +--------------+  |     | NextPtr    |->0
*                     | HeadPtr      |--/     +------------+
*                     +--------------+     0<-| PrevPtr    |
*                     | NbrEntries=1 |        +------------+
*                     +--------------+        :            :
*                                             :            :
*                                             +------------+
*
*
*                     OS_RDY_LIST
*                     +--------------+
*                     | TailPtr      |-----------------------------------------------+
*                     +--------------+          OS_TCB               OS_TCB          |     OS_TCB
*                     | HeadPtr      |------> +------------+       +------------+    +-> +------------+
*                     +--------------+        | NextPtr    |------>| NextPtr    | ...... | NextPtr    |->0
*                     | NbrEntries=N |        +------------+       +------------+        +------------+
*                     +--------------+     0<-| PrevPtr    |<------| PrevPtr    | ...... | PrevPtr    |
*                                             +------------+       +------------+        +------------+
*                                             :            :       :            :        :            :
*                                             :            :       :            :        :            :
*                                             +------------+       +------------+        +------------+
*
*
* Arguments  : p_tcb     is the OS_TCB to insert in the list
*              -----
*
* Returns    : none
*
* Note(s)    : 1) This function is INTERNAL to uC/OS-III and your application MUST NOT call it.
************************************************************************************************************************
*/

void  OS_RdyListInsertTail (OS_TCB  *p_tcb)
{
    OS_RDY_LIST  *p_rdy_list;
    OS_TCB       *p_tcb2;



    p_rdy_list = &OSRdyList[p_tcb->Prio];
    if (p_rdy_list->HeadPtr == (OS_TCB *)0) {                   /* CASE 0: Insert when there are no entries             */
#if (OS_CFG_DBG_EN > 0u)
        p_rdy_list->NbrEntries  =           1u;                 /* This is the first entry                              */
#endif
        p_tcb->NextPtr          = (OS_TCB *)0;                  /* No other OS_TCBs in the list                         */
        p_tcb->PrevPtr          = (OS_TCB *)0;
        p_rdy_list->HeadPtr     =  p_tcb;                       /* Both list pointers point to this OS_TCB              */
        p_rdy_list->TailPtr     =  p_tcb;
    } else {                                                    /* CASE 1: Insert AFTER the current tail of list        */
#if (OS_CFG_DBG_EN > 0u)
        p_rdy_list->NbrEntries++;                               /* One more OS_TCB in the list                          */
#endif
        p_tcb->NextPtr          = (OS_TCB *)0;                  /* Adjust new OS_TCBs links                             */
        p_tcb2                  =  p_rdy_list->TailPtr;
        p_tcb->PrevPtr          =  p_tcb2;
        p_tcb2->NextPtr         =  p_tcb;                       /* Adjust old tail of list's links                      */
        p_rdy_list->TailPtr     =  p_tcb;
    }
}


/*
************************************************************************************************************************
*                                                MOVE TCB AT HEAD TO TAIL
*
* Description: This function is called to move the current head of a list to the tail of the list.
*
*
*              CASE 0: TCB list is empty, nothing to do.
*
*              CASE 1: Only 1 OS_TCB  in the list, nothing to do.
*
*              CASE 2: Only 2 OS_TCBs in the list.
*
*                     OS_RDY_LIST
*                     +--------------+
*                     | TailPtr      |--------------------------+
*                     +--------------+          OS_TCB          |     OS_TCB
*                     | HeadPtr      |------> +------------+    +-> +------------+
*                     +--------------+        | NextPtr    |------> | NextPtr    |->0
*                     | NbrEntries=2 |        +------------+        +------------+
*                     +--------------+     0<-| PrevPtr    | <------| PrevPtr    |
*                                             +------------+        +------------+
*                                             :            :        :            :
*                                             :            :        :            :
*                                             +------------+        +------------+
*
*
*              CASE N: More than 2 OS_TCBs in the list.
*
*                     OS_RDY_LIST
*                     +--------------+
*                     | TailPtr      |-----------------------------------------------+
*                     +--------------+          OS_TCB               OS_TCB          |     OS_TCB
*                     | HeadPtr      |------> +------------+       +------------+    +-> +------------+
*                     +--------------+        | NextPtr    |------>| NextPtr    | ...... | NextPtr    |->0
*                     | NbrEntries=N |        +------------+       +------------+        +------------+
*                     +--------------+     0<-| PrevPtr    |<------| PrevPtr    | ...... | PrevPtr    |
*                                             +------------+       +------------+        +------------+
*                                             :            :       :            :        :            :
*                                             :            :       :            :        :            :
*                                             +------------+       +------------+        +------------+
*
*
* Arguments  : p_rdy_list    is a pointer to the OS_RDY_LIST where the OS_TCB will be inserted
*              ------
*
* Returns    : none
*
* Note(s)    : 1) This function is INTERNAL to uC/OS-III and your application MUST NOT call it.
************************************************************************************************************************
*/

void  OS_RdyListMoveHeadToTail (OS_RDY_LIST  *p_rdy_list)
{
    OS_TCB  *p_tcb1;
    OS_TCB  *p_tcb2;
    OS_TCB  *p_tcb3;


     if (p_rdy_list->HeadPtr != p_rdy_list->TailPtr) {
         if (p_rdy_list->HeadPtr->NextPtr == p_rdy_list->TailPtr) { /* SWAP the TCBs                                    */
             p_tcb1              =  p_rdy_list->HeadPtr;        /* Point to current head                                */
             p_tcb2              =  p_rdy_list->TailPtr;        /* Point to current tail                                */
             p_tcb1->PrevPtr     =  p_tcb2;
             p_tcb1->NextPtr     = (OS_TCB *)0;
             p_tcb2->PrevPtr     = (OS_TCB *)0;
             p_tcb2->NextPtr     =  p_tcb1;
             p_rdy_list->HeadPtr =  p_tcb2;
             p_rdy_list->TailPtr =  p_tcb1;
         } else {
             p_tcb1              =  p_rdy_list->HeadPtr;        /* Point to current head                                */
             p_tcb2              =  p_rdy_list->TailPtr;        /* Point to current tail                                */
             p_tcb3              =  p_tcb1->NextPtr;            /* Point to new list head                               */
             p_tcb3->PrevPtr     = (OS_TCB *)0;                 /* Adjust back    link of new list head                 */
             p_tcb1->NextPtr     = (OS_TCB *)0;                 /* Adjust forward link of new list tail                 */
             p_tcb1->PrevPtr     =  p_tcb2;                     /* Adjust back    link of new list tail                 */
             p_tcb2->NextPtr     =  p_tcb1;                     /* Adjust forward link of old list tail                 */
             p_rdy_list->HeadPtr =  p_tcb3;                     /* Adjust new list head and tail pointers               */
             p_rdy_list->TailPtr =  p_tcb1;
         }
     }
}


/*
************************************************************************************************************************
*                                REMOVE TCB FROM LIST KNOWING ONLY WHICH OS_TCB TO REMOVE
*
* Description: This function is called to remove an OS_TCB from an OS_RDY_LIST knowing the address of the OS_TCB to
*              remove.
*
*
*              CASE 0: TCB list is empty, nothing to do.
*
*              CASE 1: Only 1 OS_TCBs in the list.
*
*                     OS_RDY_LIST
*                     +--------------+          OS_TCB
*                     | TailPtr      |--+---> +------------+
*                     +--------------+  |     | NextPtr    |->0
*                     | HeadPtr      |--/     +------------+
*                     +--------------+     0<-| PrevPtr    |
*                     | NbrEntries=1 |        +------------+
*                     +--------------+        :            :
*                                             :            :
*                                             +------------+
*
*              CASE N: Two or more OS_TCBs in the list.
*
*                     OS_RDY_LIST
*                     +--------------+
*                     | TailPtr      |-----------------------------------------------+
*                     +--------------+          OS_TCB               OS_TCB          |     OS_TCB
*                     | HeadPtr      |------> +------------+       +------------+    +-> +------------+
*                     +--------------+        | NextPtr    |------>| NextPtr    | ...... | NextPtr    |->0
*                     | NbrEntries=N |        +------------+       +------------+        +------------+
*                     +--------------+     0<-| PrevPtr    |<------| PrevPtr    | ...... | PrevPtr    |
*                                             +------------+       +------------+        +------------+
*                                             :            :       :            :        :            :
*                                             :            :       :            :        :            :
*                                             +------------+       +------------+        +------------+
*
*
* Arguments  : p_tcb    is a pointer to the OS_TCB to remove
*              -----
*
* Returns    : A pointer to the OS_RDY_LIST where the OS_TCB was
*
* Note(s)    : 1) This function is INTERNAL to uC/OS-III and your application MUST NOT call it.
************************************************************************************************************************
*/

void  OS_RdyListRemove (OS_TCB  *p_tcb)
{
    OS_RDY_LIST  *p_rdy_list;
    OS_TCB       *p_tcb1;
    OS_TCB       *p_tcb2;



    p_rdy_list = &OSRdyList[p_tcb->Prio];
    p_tcb1     = p_tcb->PrevPtr;                                /* Point to next and previous OS_TCB in the list        */
    p_tcb2     = p_tcb->NextPtr;
    if (p_tcb1 == (OS_TCB *)0) {                                /* Was the OS_TCB to remove at the head?                */
        if (p_tcb2 == (OS_TCB *)0) {                            /* Yes, was it the only OS_TCB?                         */
#if (OS_CFG_DBG_EN > 0u)
            p_rdy_list->NbrEntries =           0u;              /* Yes, no more entries                                 */
#endif
            p_rdy_list->HeadPtr    = (OS_TCB *)0;
            p_rdy_list->TailPtr    = (OS_TCB *)0;
            OS_PrioRemove(p_tcb->Prio);
        } else {
#if (OS_CFG_DBG_EN > 0u)
            p_rdy_list->NbrEntries--;                           /* No,  one less entry                                  */
#endif
            p_tcb2->PrevPtr     = (OS_TCB *)0;                  /* adjust back link of new list head                    */
            p_rdy_list->HeadPtr =  p_tcb2;                      /* adjust OS_RDY_LIST's new head                        */
        }
    } else {
#if (OS_CFG_DBG_EN > 0u)
        p_rdy_list->NbrEntries--;                               /* No,  one less entry                                  */
#endif
        p_tcb1->NextPtr = p_tcb2;
        if (p_tcb2 == (OS_TCB *)0) {
            p_rdy_list->TailPtr = p_tcb1;                       /* Removing the TCB at the tail, adj the tail ptr       */
        } else {
            p_tcb2->PrevPtr     = p_tcb1;
        }
    }
    p_tcb->PrevPtr = (OS_TCB *)0;
    p_tcb->NextPtr = (OS_TCB *)0;

    OS_TRACE_TASK_SUSPENDED(p_tcb);
}


/*
************************************************************************************************************************
*                                               SCHEDULER LOCK TIME MEASUREMENT
*
* Description: These functions are used to measure the peak amount of time that the scheduler is locked
*
* Arguments  : none
*
* Returns    : none
*
* Note(s)    : 1) The are internal functions to uC/OS-III and MUST not be called by your application code.
*
*              2) It's assumed that these functions are called when interrupts are disabled.
*
*              3) We are reading the time stamp timer via OS_TS_GET() directly even if this is a 16-bit timer.  The
*                 reason is that we don't expect to have the scheduler locked for 65536 counts even at the rate the TS
*                 timer is updated.  In other words, locking the scheduler for longer than 65536 count would not be a
*                 good thing for a real-time system.
************************************************************************************************************************
*/

#if (OS_CFG_SCHED_LOCK_TIME_MEAS_EN > 0u)
void  OS_SchedLockTimeMeasStart (void)
{
    if (OSSchedLockNestingCtr == 1u) {
        OSSchedLockTimeBegin = OS_TS_GET();
    }
}




void  OS_SchedLockTimeMeasStop (void)
{
    CPU_TS_TMR  delta;


    if (OSSchedLockNestingCtr == 0u) {                          /* Make sure we fully un-nested scheduler lock          */
        delta = OS_TS_GET()                                     /* Compute the delta time between begin and end         */
              - OSSchedLockTimeBegin;
        if (OSSchedLockTimeMax    < delta) {                    /* Detect peak value                                    */
            OSSchedLockTimeMax    = delta;
        }
        if (OSSchedLockTimeMaxCur < delta) {                    /* Detect peak value (for resettable value)             */
            OSSchedLockTimeMaxCur = delta;
        }
    }
}
#endif


/*
************************************************************************************************************************
*                                        RUN ROUND-ROBIN SCHEDULING ALGORITHM
*
* Description: This function is called on every tick to determine if a new task at the same priority needs to execute.
*
*
* Arguments  : p_rdy_list    is a pointer to the OS_RDY_LIST entry of the ready list at the current priority
*              ----------
*
* Returns    : none
*
* Note(s)    : 1) This function is INTERNAL to uC/OS-III and your application MUST NOT call it.
************************************************************************************************************************
*/

#if (OS_CFG_SCHED_ROUND_ROBIN_EN > 0u)
void  OS_SchedRoundRobin (OS_RDY_LIST  *p_rdy_list)
{
    OS_TCB  *p_tcb;
    CPU_SR_ALLOC();


    if (OSSchedRoundRobinEn != OS_TRUE) {                       /* Make sure round-robin has been enabled               */
        return;
    }

    CPU_CRITICAL_ENTER();
    p_tcb = p_rdy_list->HeadPtr;                                /* Decrement time quanta counter                        */

    if (p_tcb == (OS_TCB *)0) {
        CPU_CRITICAL_EXIT();
        return;
    }

#if (OS_CFG_TASK_IDLE_EN > 0u)
    if (p_tcb == &OSIdleTaskTCB) {
        CPU_CRITICAL_EXIT();
        return;
    }
#endif

    if (p_tcb->TimeQuantaCtr > 0u) {
        p_tcb->TimeQuantaCtr--;
    }

    if (p_tcb->TimeQuantaCtr > 0u) {                            /* Task not done with its time quanta                   */
        CPU_CRITICAL_EXIT();
        return;
    }

    if (p_rdy_list->HeadPtr == p_rdy_list->TailPtr) {           /* See if it's time to time slice current task          */
        CPU_CRITICAL_EXIT();                                    /* ... only if multiple tasks at same priority          */
        return;
    }

    if (OSSchedLockNestingCtr > 0u) {                           /* Can't round-robin if the scheduler is locked         */
        CPU_CRITICAL_EXIT();
        return;
    }

    OS_RdyListMoveHeadToTail(p_rdy_list);                       /* Move current OS_TCB to the end of the list           */
    p_tcb = p_rdy_list->HeadPtr;                                /* Point to new OS_TCB at head of the list              */
    if (p_tcb->TimeQuanta == 0u) {                              /* See if we need to use the default time slice         */
        p_tcb->TimeQuantaCtr = OSSchedRoundRobinDfltTimeQuanta;
    } else {
        p_tcb->TimeQuantaCtr = p_tcb->TimeQuanta;               /* Load time slice counter with new time                */
    }
    CPU_CRITICAL_EXIT();
}
#endif


/*
************************************************************************************************************************
*                                                     BLOCK A TASK
*
* Description: This function is called to remove a task from the ready list and also insert it in the timer tick list if
*              the specified timeout is non-zero.
*
* Arguments  : p_tcb          is a pointer to the OS_TCB of the task block
*              -----
*
*              timeout        is the desired timeout
*
* Returns    : none
*
* Note(s)    : 1) This function is INTERNAL to uC/OS-III and your application MUST NOT call it.
************************************************************************************************************************
*/

void  OS_TaskBlock (OS_TCB   *p_tcb,
                    OS_TICK   timeout)
{
#if (OS_CFG_DYN_TICK_EN > 0u)
    OS_TICK  elapsed;


    elapsed = OS_DynTickGet();
#endif

#if (OS_CFG_TICK_EN > 0u)
    if (timeout > 0u) {                                         /* Add task to tick list if timeout non zero            */
#if (OS_CFG_DYN_TICK_EN > 0u)
        (void)OS_TickListInsert(p_tcb, elapsed, (OSTickCtr + elapsed), timeout);
#else
        (void)OS_TickListInsert(p_tcb,      0u,             OSTickCtr, timeout);
#endif
        p_tcb->TaskState = OS_TASK_STATE_PEND_TIMEOUT;
    } else {
        p_tcb->TaskState = OS_TASK_STATE_PEND;
    }
#else
    (void)timeout;
    p_tcb->TaskState = OS_TASK_STATE_PEND;
#endif
    OS_RdyListRemove(p_tcb);
}
