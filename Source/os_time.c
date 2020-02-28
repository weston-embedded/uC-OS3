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
*                                            TIME MANAGEMENT
*
* File    : os_time.c
* Version : V3.08.00
*********************************************************************************************************
*/

#define  MICRIUM_SOURCE
#include "os.h"

#ifdef VSC_INCLUDE_SOURCE_FILE_NAMES
const  CPU_CHAR  *os_time__c = "$Id: $";
#endif

/*
************************************************************************************************************************
*                                                  DELAY TASK 'n' TICKS
*
* Description: This function is called to delay execution of the currently running task until the specified number of
*              system ticks expires.  This, of course, directly equates to delaying the current task for some time to
*              expire.  No delay will result if the specified delay is 0.  If the specified delay is greater than 0
*              then, a context switch will result.
*
* Arguments  : dly       is a value in 'clock ticks' that the task will either delay for or, the target match value
*                        of the tick counter (OSTickCtr).  Note that specifying 0 means the task is not to delay.
*
*                        depending on the option argument, the task will wake up when OSTickCtr reaches:
*
*                            OS_OPT_TIME_DLY      : OSTickCtr + dly
*                            OS_OPT_TIME_TIMEOUT  : OSTickCtr + dly
*                            OS_OPT_TIME_MATCH    : dly
*                            OS_OPT_TIME_PERIODIC : OSTCBCurPtr->TickCtrPrev + dly
*
*              opt       specifies whether 'dly' represents absolute or relative time; default option marked with *** :
*
*                        *** OS_OPT_TIME_DLY        specifies a relative time from the current value of OSTickCtr.
*                            OS_OPT_TIME_TIMEOUT    same as OS_OPT_TIME_DLY.
*                            OS_OPT_TIME_MATCH      indicates that 'dly' specifies the absolute value that OSTickCtr
*                                                   must reach before the task will be resumed.
*                            OS_OPT_TIME_PERIODIC   indicates that 'dly' specifies the periodic value that OSTickCtr
*                                                   must reach before the task will be resumed.
*
*              p_err     is a pointer to a variable that will contain an error code from this call.
*
*                            OS_ERR_NONE              The call was successful and the delay occurred
*                            OS_ERR_OPT_INVALID       If you specified an invalid option for this function
*                            OS_ERR_OS_NOT_RUNNING    If uC/OS-III is not running yet
*                            OS_ERR_SCHED_LOCKED      Can't delay when the scheduler is locked
*                            OS_ERR_TIME_DLY_ISR      If you called this function from an ISR
*                            OS_ERR_TIME_ZERO_DLY     If the effective delay is zero
*                            OS_ERR_TICK_DISABLED     If kernel ticks are disabled
*
* Returns    : none
*
* Note(s)    : none
************************************************************************************************************************
*/

void  OSTimeDly (OS_TICK   dly,
                 OS_OPT    opt,
                 OS_ERR   *p_err)
{
#if (OS_CFG_TICK_EN > 0u)
    CPU_SR_ALLOC();
#endif


#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return;
    }
#endif

#if (OS_CFG_TICK_EN == 0u)
    *p_err = OS_ERR_TICK_DISABLED;
    return;
#else

#if (OS_CFG_CALLED_FROM_ISR_CHK_EN > 0u)
    if (OSIntNestingCtr > 0u) {                                 /* Not allowed to call from an ISR                      */
       *p_err = OS_ERR_TIME_DLY_ISR;
        return;
    }
#endif

#if (OS_CFG_INVALID_OS_CALLS_CHK_EN > 0u)
    if (OSRunning != OS_STATE_OS_RUNNING) {                     /* Is the kernel running?                               */
       *p_err = OS_ERR_OS_NOT_RUNNING;
        return;
    }
#endif

    if (OSSchedLockNestingCtr > 0u) {                           /* Can't delay when the scheduler is locked             */
       *p_err = OS_ERR_SCHED_LOCKED;
        return;
    }

    switch (opt) {
        case OS_OPT_TIME_DLY:
        case OS_OPT_TIME_TIMEOUT:
        case OS_OPT_TIME_PERIODIC:
        case OS_OPT_TIME_MATCH:
             break;

        default:
            *p_err = OS_ERR_OPT_INVALID;
             return;
    }

#if (OS_CFG_TICK_EN > 0u)
    CPU_CRITICAL_ENTER();
    OS_TickListInsertDly(OSTCBCurPtr,
                         dly,
                         opt,
                         p_err);
    if (*p_err != OS_ERR_NONE) {
         CPU_CRITICAL_EXIT();
         return;
    }

    OS_RdyListRemove(OSTCBCurPtr);                              /* Remove current task from ready list                  */
    CPU_CRITICAL_EXIT();
    OSSched();                                                  /* Find next task to run!                               */
#endif
#endif
}


/*
************************************************************************************************************************
*                                             DELAY TASK FOR SPECIFIED TIME
*
* Description: This function is called to delay execution of the currently running task until some time expires.  This
*              call allows you to specify the delay time in HOURS, MINUTES, SECONDS and MILLISECONDS instead of ticks.
*
* Arguments  : hours     specifies the number of hours that the task will be delayed (max. is 999 if the tick rate is
*                        1000 Hz or less otherwise, a higher value would overflow a 32-bit unsigned counter).
*
*              minutes   specifies the number of minutes (max. 59 if 'opt' is OS_OPT_TIME_HMSM_STRICT)
*
*              seconds   specifies the number of seconds (max. 59 if 'opt' is OS_OPT_TIME_HMSM_STRICT)
*
*              milli     specifies the number of milliseconds (max. 999 if 'opt' is OS_OPT_TIME_HMSM_STRICT)
*
*              opt       specifies time delay bit-field options logically OR'd; default options marked with *** :
*
*                        *** OS_OPT_TIME_DLY        specifies a relative time from the current value of OSTickCtr.
*                            OS_OPT_TIME_TIMEOUT    same as OS_OPT_TIME_DLY.
*                            OS_OPT_TIME_MATCH      indicates that the delay specifies the absolute value that OSTickCtr
*                                                   must reach before the task will be resumed.
*                            OS_OPT_TIME_PERIODIC   indicates that the delay specifies the periodic value that OSTickCtr
*                                                   must reach before the task will be resumed.
*
*                        *** OS_OPT_TIME_HMSM_STRICT            strictly allow only hours        (0...99)
*                                                                                   minutes      (0...59)
*                                                                                   seconds      (0...59)
*                                                                                   milliseconds (0...999)
*                            OS_OPT_TIME_HMSM_NON_STRICT        allow any value of  hours        (0...999)
*                                                                                   minutes      (0...9999)
*                                                                                   seconds      (0...65535)
*                                                                                   milliseconds (0...4294967295)
*
*              p_err     is a pointer to a variable that will receive an error code from this call.
*
*                            OS_ERR_NONE                        If the function returns from the desired delay
*                            OS_ERR_OPT_INVALID                 If you specified an invalid option for 'opt'
*                            OS_ERR_OS_NOT_RUNNING              If uC/OS-III is not running yet
*                            OS_ERR_SCHED_LOCKED                Can't delay when the scheduler is locked
*                            OS_ERR_TIME_DLY_ISR                If called from an ISR
*                            OS_ERR_TIME_INVALID_HOURS          If you didn't specify a valid value for 'hours'
*                            OS_ERR_TIME_INVALID_MINUTES        If you didn't specify a valid value for 'minutes'
*                            OS_ERR_TIME_INVALID_SECONDS        If you didn't specify a valid value for 'seconds'
*                            OS_ERR_TIME_INVALID_MILLISECONDS   If you didn't specify a valid value for 'milli'
*                            OS_ERR_TIME_ZERO_DLY               If the effective delay is zero
*                            OS_ERR_TICK_DISABLED               If kernel ticks are disabled
*
* Returns    : none
*
* Note(s)    : 1) The resolution on the milliseconds depends on the tick rate.  For example, you can't do a 10 mS delay
*                 if the ticker interrupts every 100 mS.  In this case, the delay would be set to 0.  The actual delay
*                 is rounded to the nearest tick.
*
*              2) Although this function allows you to delay a task for many, many hours, it's not recommended to put
*                 a task to sleep for that long.
************************************************************************************************************************
*/

#if (OS_CFG_TIME_DLY_HMSM_EN > 0u)
void  OSTimeDlyHMSM (CPU_INT16U   hours,
                     CPU_INT16U   minutes,
                     CPU_INT16U   seconds,
                     CPU_INT32U   milli,
                     OS_OPT       opt,
                     OS_ERR      *p_err)
{
#if (OS_CFG_TICK_EN > 0u)
#if (OS_CFG_ARG_CHK_EN > 0u)
    CPU_BOOLEAN  opt_invalid;
    CPU_BOOLEAN  opt_non_strict;
#endif
    OS_OPT       opt_time;
    OS_RATE_HZ   tick_rate;
    OS_TICK      ticks;
    CPU_SR_ALLOC();
#endif



#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return;
    }
#endif

#if (OS_CFG_TICK_EN == 0u)
    *p_err = OS_ERR_TICK_DISABLED;
    return;
#else
#if (OS_CFG_CALLED_FROM_ISR_CHK_EN > 0u)
    if (OSIntNestingCtr > 0u) {                                 /* Not allowed to call from an ISR                      */
       *p_err = OS_ERR_TIME_DLY_ISR;
        return;
    }
#endif

#if (OS_CFG_INVALID_OS_CALLS_CHK_EN > 0u)
    if (OSRunning != OS_STATE_OS_RUNNING) {                     /* Is the kernel running?                               */
       *p_err = OS_ERR_OS_NOT_RUNNING;
        return;
    }
#endif

    if (OSSchedLockNestingCtr > 0u) {                           /* Can't delay when the scheduler is locked             */
       *p_err = OS_ERR_SCHED_LOCKED;
        return;
    }

    opt_time = opt & OS_OPT_TIME_MASK;                          /* Retrieve time options only.                          */
    switch (opt_time) {
        case OS_OPT_TIME_DLY:
        case OS_OPT_TIME_TIMEOUT:
        case OS_OPT_TIME_PERIODIC:
        case OS_OPT_TIME_MATCH:
             break;

        default:
            *p_err = OS_ERR_OPT_INVALID;
             return;
    }

#if (OS_CFG_ARG_CHK_EN > 0u)                                    /* Validate arguments to be within range                */
    opt_invalid = ((((opt) & (~OS_OPT_TIME_OPTS_MASK)) == 0u) ? (OS_FALSE) : (OS_TRUE));
    if (opt_invalid == OS_TRUE) {
       *p_err = OS_ERR_OPT_INVALID;
        return;
    }

    opt_non_strict = ((((opt) & (OS_OPT_TIME_HMSM_NON_STRICT)) == 0u) ? (OS_FALSE) : (OS_TRUE));

    if (opt_non_strict != OS_TRUE) {
         if (milli   > 999u) {
            *p_err = OS_ERR_TIME_INVALID_MILLISECONDS;
             return;
         }
         if (seconds > 59u) {
            *p_err = OS_ERR_TIME_INVALID_SECONDS;
             return;
         }
         if (minutes > 59u) {
            *p_err = OS_ERR_TIME_INVALID_MINUTES;
             return;
         }
         if (hours   > 99u) {
            *p_err = OS_ERR_TIME_INVALID_HOURS;
             return;
         }
    } else {
         if (minutes > 9999u) {
            *p_err = OS_ERR_TIME_INVALID_MINUTES;
             return;
         }
         if (hours   > 999u) {
            *p_err = OS_ERR_TIME_INVALID_HOURS;
             return;
         }
    }
#endif

                                                                /* Compute the total number of clock ticks required..   */
                                                                /* .. (rounded to the nearest tick)                     */
    tick_rate = OSCfg_TickRate_Hz;
    ticks     = ((((OS_TICK)hours * (OS_TICK)3600u) + ((OS_TICK)minutes * (OS_TICK)60u) + (OS_TICK)seconds) * tick_rate)
              + ((tick_rate * ((OS_TICK)milli + ((OS_TICK)500u / tick_rate))) / (OS_TICK)1000u);


    CPU_CRITICAL_ENTER();
    OS_TickListInsertDly(OSTCBCurPtr,
                         ticks,
                         opt_time,
                         p_err);
    if (*p_err != OS_ERR_NONE) {
         CPU_CRITICAL_EXIT();
         return;
    }

    OS_RdyListRemove(OSTCBCurPtr);                              /* Remove current task from ready list                  */
    CPU_CRITICAL_EXIT();
    OSSched();                                                  /* Find next task to run!                               */
#endif
}
#endif

/*
************************************************************************************************************************
*                                                RESUME A DELAYED TASK
*
* Description: This function is used resume a task that has been delayed through a call to either OSTimeDly() or
*              OSTimeDlyHMSM().  Note that you cannot call this function to resume a task that is waiting for an event
*              with timeout.
*
* Arguments  : p_tcb    is a pointer to the TCB of the task to resume.
*
*              p_err    is a pointer to a variable that will receive an error code
*
*                           OS_ERR_NONE                  Task has been resumed
*                           OS_ERR_OS_NOT_RUNNING        If uC/OS-III is not running yet
*                           OS_ERR_STATE_INVALID         Task is in an invalid state
*                           OS_ERR_TASK_NOT_DLY          Task is not waiting for time to expire
*                           OS_ERR_TASK_SUSPENDED        Task cannot be resumed, it was suspended by OSTaskSuspend()
*                           OS_ERR_TCB_INVALID           If 'p_tcb' is a NULL pointer
*                           OS_ERR_TIME_DLY_RESUME_ISR   If called from an ISR
*
* Returns    : none
*
* Note(s)    : none
************************************************************************************************************************
*/

#if (OS_CFG_TIME_DLY_RESUME_EN > 0u)
void  OSTimeDlyResume (OS_TCB  *p_tcb,
                       OS_ERR  *p_err)
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
       *p_err = OS_ERR_TIME_DLY_RESUME_ISR;
        return;
    }
#endif

#if (OS_CFG_ARG_CHK_EN > 0u)                                    /* ---------------- VALIDATE ARGUMENTS ---------------- */
    if (p_tcb == (OS_TCB *)0) {                                 /* User must supply a valid OS_TCB                      */
       *p_err = OS_ERR_TCB_INVALID;
        return;
    }
#endif

#if (OS_CFG_INVALID_OS_CALLS_CHK_EN > 0u)
    if (OSRunning != OS_STATE_OS_RUNNING) {                     /* Is the kernel running?                               */
       *p_err = OS_ERR_OS_NOT_RUNNING;
        return;
    }
#endif

    CPU_CRITICAL_ENTER();
    switch (p_tcb->TaskState) {
        case OS_TASK_STATE_RDY:                                 /* Cannot Abort delay if task is ready                  */
        case OS_TASK_STATE_PEND:
        case OS_TASK_STATE_PEND_TIMEOUT:
        case OS_TASK_STATE_SUSPENDED:
        case OS_TASK_STATE_PEND_SUSPENDED:
        case OS_TASK_STATE_PEND_TIMEOUT_SUSPENDED:
             CPU_CRITICAL_EXIT();
            *p_err = OS_ERR_TASK_NOT_DLY;
             break;

        case OS_TASK_STATE_DLY:
             p_tcb->TaskState = OS_TASK_STATE_RDY;
#if (OS_CFG_TICK_EN > 0u)
             OS_TickListRemove(p_tcb);                          /* Remove task from tick list                           */
             OS_RdyListInsert(p_tcb);                           /* Add to ready list                                    */
#endif
             CPU_CRITICAL_EXIT();
            *p_err = OS_ERR_NONE;
             break;

        case OS_TASK_STATE_DLY_SUSPENDED:
             p_tcb->TaskState = OS_TASK_STATE_SUSPENDED;
#if (OS_CFG_TICK_EN > 0u)
             OS_TickListRemove(p_tcb);                          /* Remove task from tick list                           */
#endif
             CPU_CRITICAL_EXIT();
            *p_err            = OS_ERR_TASK_SUSPENDED;
             break;

        default:
             CPU_CRITICAL_EXIT();
            *p_err = OS_ERR_STATE_INVALID;
             break;
    }

    OSSched();
}
#endif

/*
************************************************************************************************************************
*                                               GET CURRENT SYSTEM TIME
*
* Description: This function is used by your application to obtain the current value of the counter which keeps track of
*              the number of clock ticks.
*
* Arguments  : p_err    is a pointer to a variable that will receive an error code
*
*                           OS_ERR_NONE              If the call was successful
*                           OS_ERR_TICK_DISABLED     If kernel ticks are disabled
*
* Returns    : The current value of OSTickCtr
*
* Note(s)    : none
************************************************************************************************************************
*/

OS_TICK  OSTimeGet (OS_ERR  *p_err)
{
    OS_TICK  ticks;
#if (OS_CFG_TICK_EN > 0u)
    CPU_SR_ALLOC();
#endif


#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return (0u);
    }
#endif


#if (OS_CFG_TICK_EN > 0u)
    CPU_CRITICAL_ENTER();
#if (OS_CFG_DYN_TICK_EN > 0u)
    if (OSRunning == OS_STATE_OS_RUNNING) {
        ticks = OSTickCtr + OS_DynTickGet();
    } else {
        ticks = OSTickCtr;
    }
#else
    ticks = OSTickCtr;
#endif
    CPU_CRITICAL_EXIT();
   *p_err = OS_ERR_NONE;
#else
    ticks = 0u;
   *p_err = OS_ERR_TICK_DISABLED;
#endif

    return (ticks);
}

/*
************************************************************************************************************************
*                                                   SET SYSTEM CLOCK
*
* Description: This function sets the counter which keeps track of the number of clock ticks.
*
* Arguments  : ticks    is the desired tick value
*
*              p_err    is a pointer to a variable that will receive an error code
*
*                           OS_ERR_NONE              If the call was successful
*                           OS_ERR_TICK_DISABLED     If kernel ticks are disabled
*
* Returns    : none
*
* Note(s)    : none
************************************************************************************************************************
*/

void  OSTimeSet (OS_TICK   ticks,
                 OS_ERR   *p_err)
{
#if (OS_CFG_TICK_EN > 0u)
    CPU_SR_ALLOC();

#else
    (void)ticks;
#endif


#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return;
    }
#endif

#if (OS_CFG_TICK_EN > 0u)
    CPU_CRITICAL_ENTER();
    OSTickCtr = ticks;
    OS_TRACE_TICK_INCREMENT(OSTickCtr);
    CPU_CRITICAL_EXIT();
   *p_err = OS_ERR_NONE;
#else
   *p_err = OS_ERR_TICK_DISABLED;
#endif

}


/*
************************************************************************************************************************
*                                                 PROCESS SYSTEM TICK
*
* Description: This function is used to signal to uC/OS-III the occurrence of a 'system tick' (also known as a
*              'clock tick').  This function should be called by the tick ISR.
*
* Arguments  : none
*
* Returns    : none
*
* Note(s)    : none
************************************************************************************************************************
*/

void  OSTimeTick (void)
{
    if (OSRunning != OS_STATE_OS_RUNNING) {
        return;
    }

    OSTimeTickHook();                                           /* Call user definable hook                             */

#if (OS_CFG_SCHED_ROUND_ROBIN_EN > 0u)
    OS_SchedRoundRobin(&OSRdyList[OSPrioCur]);                  /* Update quanta ctr for the task which just ran        */
#endif

#if (OS_CFG_TICK_EN > 0u)
    OS_TickUpdate(1u);                                          /* Update from the ISR                                  */
#endif
}


/*
************************************************************************************************************************
*                                             PROCESS SYSTEM TICK (DYNAMIC)
*
* Description: This function is used to signal to uC/OS-III the occurrence of a 'system tick' (also known as a
*              'clock tick').  This function should be called by the tick ISR.
*
* Arguments  : none
*
* Returns    : none
*
* Note(s)    : none
************************************************************************************************************************
*/

#if (OS_CFG_DYN_TICK_EN > 0u)
void  OSTimeDynTick (OS_TICK  ticks)
{
    if (OSRunning != OS_STATE_OS_RUNNING) {
        return;
    }

    OSTimeTickHook();

    OS_TickUpdate(ticks);                                       /* Update from the ISR                                  */
}
#endif
