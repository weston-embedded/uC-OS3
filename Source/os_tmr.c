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
*                                           TIMER MANAGEMENT
*
* File    : os_tmr.c
* Version : V3.08.00
*********************************************************************************************************
*/

#define  MICRIUM_SOURCE
#include "os.h"

#ifdef VSC_INCLUDE_SOURCE_FILE_NAMES
const  CPU_CHAR  *os_tmr__c = "$Id: $";
#endif


#if (OS_CFG_TMR_EN > 0u)
/*
************************************************************************************************************************
*                                               LOCAL FUNCTION PROTOTYPES
************************************************************************************************************************
*/

static  void  OS_TmrLock      (void);
static  void  OS_TmrUnlock    (void);

static  void  OS_TmrCondCreate(void);
static  void  OS_TmrCondSignal(void);
static  void  OS_TmrCondWait  (OS_TICK  timeout);


/*
************************************************************************************************************************
*                                                   CREATE A TIMER
*
* Description: This function is called by your application code to create a timer.
*
* Arguments  : p_tmr           Is a pointer to a timer control block
*
*              p_name          Is a pointer to an ASCII string that is used to name the timer.  Names are useful for
*                              debugging.
*
*              dly             Initial delay.
*                              If the timer is configured for ONE-SHOT mode, this is the timeout used
*                              If the timer is configured for PERIODIC mode, this is the first timeout to wait for
*                              before the timer starts entering periodic mode
*
*              period          The 'period' being repeated for the timer.
*                              If you specified 'OS_OPT_TMR_PERIODIC' as an option, when the timer expires, it will
*                              automatically restart with the same period.
*
*              opt             Specifies either:
*
*                                  OS_OPT_TMR_ONE_SHOT       The timer counts down only once
*                                  OS_OPT_TMR_PERIODIC       The timer counts down and then reloads itself
*
*              p_callback      Is a pointer to a callback function that will be called when the timer expires.  The
*                              callback function must be declared as follows:
*
*                                  void  MyCallback (OS_TMR *p_tmr, void *p_arg);
*
*              p_callback_arg  Is an argument (a pointer) that is passed to the callback function when it is called.
*
*              p_err           Is a pointer to an error code.  '*p_err' will contain one of the following:
*
*                                 OS_ERR_NONE                    The call succeeded
*                                 OS_ERR_ILLEGAL_CREATE_RUN_TIME If you are trying to create the timer after you called
*                                                                  OSSafetyCriticalStart()
*                                 OS_ERR_OBJ_PTR_NULL            Is 'p_tmr' is a NULL pointer
*                                 OS_ERR_OPT_INVALID             You specified an invalid option
*                                 OS_ERR_TMR_INVALID_CALLBACK    You specified an invalid callback for a periodic timer
*                                 OS_ERR_TMR_INVALID_DLY         You specified an invalid delay
*                                 OS_ERR_TMR_INVALID_PERIOD      You specified an invalid period
*                                 OS_ERR_TMR_ISR                 If the call was made from an ISR
*                                 OS_ERR_OBJ_CREATED             If the timer was already created
*
* Returns    : none
*
* Note(s)    : 1) This function only creates the timer.  In other words, the timer is not started when created.  To
*                 start the timer, call OSTmrStart().
************************************************************************************************************************
*/

void  OSTmrCreate (OS_TMR               *p_tmr,
                   CPU_CHAR             *p_name,
                   OS_TICK               dly,
                   OS_TICK               period,
                   OS_OPT                opt,
                   OS_TMR_CALLBACK_PTR   p_callback,
                   void                 *p_callback_arg,
                   OS_ERR               *p_err)
{
#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return;
    }
#endif

#ifdef OS_SAFETY_CRITICAL_IEC61508
    if (OSSafetyCriticalStartFlag == OS_TRUE) {
       *p_err = OS_ERR_ILLEGAL_CREATE_RUN_TIME;
        return;
    }
#endif

#if (OS_CFG_CALLED_FROM_ISR_CHK_EN > 0u)
    if (OSIntNestingCtr > 0u) {                                 /* See if trying to call from an ISR                    */
       *p_err = OS_ERR_TMR_ISR;
        return;
    }
#endif

#if (OS_CFG_ARG_CHK_EN > 0u)
    if (p_tmr == (OS_TMR *)0) {                                 /* Validate 'p_tmr'                                     */
       *p_err = OS_ERR_OBJ_PTR_NULL;
        return;
    }

    switch (opt) {
        case OS_OPT_TMR_PERIODIC:
             if (period == 0u) {
                *p_err = OS_ERR_TMR_INVALID_PERIOD;
                 return;
             }

             if (p_callback == (OS_TMR_CALLBACK_PTR)0) {        /* No point in a periodic timer without a callback      */
                *p_err = OS_ERR_TMR_INVALID_CALLBACK;
                 return;
             }
             break;

        case OS_OPT_TMR_ONE_SHOT:
             if (dly == 0u) {
                *p_err = OS_ERR_TMR_INVALID_DLY;
                 return;
             }
             break;

        default:
            *p_err = OS_ERR_OPT_INVALID;
             return;
    }
#endif

    if (OSRunning == OS_STATE_OS_RUNNING) {                     /* Only lock when the kernel is running                 */
        OS_TmrLock();
    }

    p_tmr->State          = OS_TMR_STATE_STOPPED;               /* Initialize the timer fields                          */
#if (OS_OBJ_TYPE_REQ > 0u)
    if (p_tmr->Type == OS_OBJ_TYPE_TMR) {
        if (OSRunning == OS_STATE_OS_RUNNING) {
            OS_TmrUnlock();
        }
        *p_err = OS_ERR_OBJ_CREATED;
        return;
    }
    p_tmr->Type           = OS_OBJ_TYPE_TMR;
#endif
#if (OS_CFG_DBG_EN > 0u)
    p_tmr->NamePtr        = p_name;
#else
    (void)p_name;
#endif
    p_tmr->Dly            =  dly    * OSTmrToTicksMult;         /* Convert to Timer Start Delay to ticks                */
    p_tmr->Remain         =  0u;
    p_tmr->Period         =  period * OSTmrToTicksMult;         /* Convert to Timer Period      to ticks                */
    p_tmr->Opt            =  opt;
    p_tmr->CallbackPtr    =  p_callback;
    p_tmr->CallbackPtrArg =  p_callback_arg;
    p_tmr->NextPtr        = (OS_TMR *)0;
    p_tmr->PrevPtr        = (OS_TMR *)0;

#if (OS_CFG_DBG_EN > 0u)
    OS_TmrDbgListAdd(p_tmr);
#endif
#if (OS_CFG_DBG_EN > 0u)
    OSTmrQty++;                                                 /* Keep track of the number of timers created           */
#endif

    if (OSRunning == OS_STATE_OS_RUNNING) {
        OS_TmrUnlock();
    }

   *p_err = OS_ERR_NONE;
}


/*
************************************************************************************************************************
*                                                   DELETE A TIMER
*
* Description: This function is called by your application code to delete a timer.
*
* Arguments  : p_tmr          Is a pointer to the timer to stop and delete.
*
*              p_err          Is a pointer to an error code.  '*p_err' will contain one of the following:
*
*                                 OS_ERR_NONE                    The call succeeded
*                                 OS_ERR_ILLEGAL_DEL_RUN_TIME    If you are trying to delete the timer after you called
*                                                                  OSStart()
*                                 OS_ERR_OBJ_TYPE                If 'p_tmr' is not pointing to a timer
*                                 OS_ERR_OS_NOT_RUNNING          If uC/OS-III is not running yet
*                                 OS_ERR_TMR_INACTIVE            If the timer was not created
*                                 OS_ERR_TMR_INVALID             If 'p_tmr' is a NULL pointer
*                                 OS_ERR_TMR_INVALID_STATE       The timer is in an invalid state
*                                 OS_ERR_TMR_ISR                 If the function was called from an ISR
*
* Returns    : OS_TRUE   if the timer was deleted
*              OS_FALSE  if not or upon an error
*
* Note(s)    : none
************************************************************************************************************************
*/

#if (OS_CFG_TMR_DEL_EN > 0u)
CPU_BOOLEAN  OSTmrDel (OS_TMR  *p_tmr,
                       OS_ERR  *p_err)
{
    CPU_BOOLEAN  success;
    OS_TICK      time;
    CPU_SR_ALLOC();


#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return (OS_FALSE);
    }
#endif

#ifdef OS_SAFETY_CRITICAL_IEC61508
    if (OSSafetyCriticalStartFlag == OS_TRUE) {
       *p_err = OS_ERR_ILLEGAL_DEL_RUN_TIME;
        return (OS_FALSE);
    }
#endif

#if (OS_CFG_CALLED_FROM_ISR_CHK_EN > 0u)
    if (OSIntNestingCtr > 0u) {                                 /* See if trying to call from an ISR                    */
       *p_err  = OS_ERR_TMR_ISR;
        return (OS_FALSE);
    }
#endif

#if (OS_CFG_INVALID_OS_CALLS_CHK_EN > 0u)
    if (OSRunning != OS_STATE_OS_RUNNING) {                     /* Is the kernel running?                               */
       *p_err = OS_ERR_OS_NOT_RUNNING;
        return (OS_FALSE);
    }
#endif

#if (OS_CFG_ARG_CHK_EN > 0u)
    if (p_tmr == (OS_TMR *)0) {
       *p_err = OS_ERR_TMR_INVALID;
        return (OS_FALSE);
    }
#endif

#if (OS_CFG_OBJ_TYPE_CHK_EN > 0u)
    if (p_tmr->Type != OS_OBJ_TYPE_TMR) {                       /* Make sure timer was created                          */
       *p_err = OS_ERR_OBJ_TYPE;
        return (OS_FALSE);
    }
#endif

    OS_TmrLock();

    CPU_CRITICAL_ENTER();
    if (OSTCBCurPtr == &OSTmrTaskTCB) {                         /* Callbacks operate on the Tmr Task's tick base.       */
        time = OSTmrTaskTickBase;
    } else {
#if (OS_CFG_DYN_TICK_EN > 0u)
        time = OSTickCtr + OS_DynTickGet();
#else
        time = OSTickCtr;
#endif
    }
    CPU_CRITICAL_EXIT();

#if (OS_CFG_DBG_EN > 0u)
    OS_TmrDbgListRemove(p_tmr);
#endif

    switch (p_tmr->State) {
        case OS_TMR_STATE_RUNNING:
        case OS_TMR_STATE_TIMEOUT:
             OS_TmrUnlink(p_tmr, time);                         /* Remove from the list                                 */
             OS_TmrClr(p_tmr);
#if (OS_CFG_DBG_EN > 0u)
             OSTmrQty--;                                        /* One less timer                                       */
#endif
            *p_err   = OS_ERR_NONE;
             success = OS_TRUE;
             break;

        case OS_TMR_STATE_STOPPED:                              /* Timer has not started or ...                         */
        case OS_TMR_STATE_COMPLETED:                            /* ... timer has completed the ONE-SHOT time            */
             OS_TmrClr(p_tmr);                                  /* Clear timer fields                                   */
#if (OS_CFG_DBG_EN > 0u)
             OSTmrQty--;                                        /* One less timer                                       */
#endif
            *p_err   = OS_ERR_NONE;
             success = OS_TRUE;
             break;

        case OS_TMR_STATE_UNUSED:                               /* Already deleted                                      */
            *p_err   = OS_ERR_TMR_INACTIVE;
             success = OS_FALSE;
             break;

        default:
            *p_err   = OS_ERR_TMR_INVALID_STATE;
             success = OS_FALSE;
             break;
    }

    OS_TmrUnlock();

    return (success);
}
#endif


/*
************************************************************************************************************************
*                                    GET HOW MUCH TIME IS LEFT BEFORE A TIMER EXPIRES
*
* Description: This function is called to get the number of timer increments before a timer times out.
*
* Arguments  : p_tmr    Is a pointer to the timer to obtain the remaining time from.
*
*              p_err    Is a pointer to an error code.  '*p_err' will contain one of the following:
*
*                           OS_ERR_NONE               The call succeeded
*                           OS_ERR_OBJ_TYPE           If 'p_tmr' is not pointing to a timer
*                           OS_ERR_OS_NOT_RUNNING     If uC/OS-III is not running yet
*                           OS_ERR_TMR_INACTIVE       If 'p_tmr' points to a timer that is not active
*                           OS_ERR_TMR_INVALID        If 'p_tmr' is a NULL pointer
*                           OS_ERR_TMR_INVALID_STATE  The timer is in an invalid state
*                           OS_ERR_TMR_ISR            If the call was made from an ISR
*
* Returns    : The time remaining for the timer to expire.  The time represents 'timer' increments (typically 1/10 sec).
*
* Note(s)    : none
************************************************************************************************************************
*/

OS_TICK  OSTmrRemainGet (OS_TMR  *p_tmr,
                         OS_ERR  *p_err)
{
    OS_TMR   *p_tmr1;
    OS_TICK   remain;


#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return (0u);
    }
#endif

#if (OS_CFG_CALLED_FROM_ISR_CHK_EN > 0u)
    if (OSIntNestingCtr > 0u) {                                 /* See if trying to call from an ISR                    */
       *p_err = OS_ERR_TMR_ISR;
        return (0u);
    }
#endif

#if (OS_CFG_INVALID_OS_CALLS_CHK_EN > 0u)
    if (OSRunning != OS_STATE_OS_RUNNING) {                     /* Is the kernel running?                               */
       *p_err = OS_ERR_OS_NOT_RUNNING;
        return (0u);
    }
#endif

#if (OS_CFG_ARG_CHK_EN > 0u)
    if (p_tmr == (OS_TMR *)0) {
       *p_err = OS_ERR_TMR_INVALID;
        return (0u);
    }
#endif

#if (OS_CFG_OBJ_TYPE_CHK_EN > 0u)
    if (p_tmr->Type != OS_OBJ_TYPE_TMR) {                       /* Make sure timer was created                          */
       *p_err = OS_ERR_OBJ_TYPE;
        return (0u);
    }
#endif

    OS_TmrLock();

    switch (p_tmr->State) {
        case OS_TMR_STATE_RUNNING:
             p_tmr1 = OSTmrListPtr;
             remain = 0u;
             while (p_tmr1 != (OS_TMR *)0) {                    /* Add up all the deltas up until the current timer     */
                 remain += p_tmr1->Remain;
                 if (p_tmr1 == p_tmr) {
                     break;
                 }
                 p_tmr1 = p_tmr1->NextPtr;
             }
             remain /= OSTmrToTicksMult;
            *p_err   = OS_ERR_NONE;
             break;

        case OS_TMR_STATE_STOPPED:                              /* It's assumed that the timer has not started yet      */
             if (p_tmr->Opt == OS_OPT_TMR_PERIODIC) {
                 if (p_tmr->Dly == 0u) {
                     remain = p_tmr->Period / OSTmrToTicksMult;
                 } else {
                     remain = p_tmr->Dly / OSTmrToTicksMult;
                 }
             } else {
                 remain = p_tmr->Dly / OSTmrToTicksMult;
             }
            *p_err = OS_ERR_NONE;
             break;

        case OS_TMR_STATE_TIMEOUT:                              /* Within a callback, timers are in the TIMEOUT state   */
        case OS_TMR_STATE_COMPLETED:                            /* Only ONE-SHOT timers can be in the COMPLETED state   */
            *p_err  = OS_ERR_NONE;
             remain = 0u;
             break;

        case OS_TMR_STATE_UNUSED:
            *p_err  = OS_ERR_TMR_INACTIVE;
             remain = 0u;
             break;

        default:
            *p_err = OS_ERR_TMR_INVALID_STATE;
             remain = 0u;
             break;
    }

    OS_TmrUnlock();

    return (remain);
}


/*
************************************************************************************************************************
*                                                    SET A TIMER
*
* Description: This function is called by your application code to set a timer.
*
* Arguments  : p_tmr           Is a pointer to a timer control block
*
*              dly             Initial delay.
*                              If the timer is configured for ONE-SHOT mode, this is the timeout used
*                              If the timer is configured for PERIODIC mode, this is the first timeout to wait for
*                              before the timer starts entering periodic mode
*
*              period          The 'period' being repeated for the timer.
*                              If you specified 'OS_OPT_TMR_PERIODIC' as an option, when the timer expires, it will
*                              automatically restart with the same period.
*
*              p_callback      Is a pointer to a callback function that will be called when the timer expires.  The
*                              callback function must be declared as follows:
*
*                                  void  MyCallback (OS_TMR *p_tmr, void *p_arg);
*
*              p_callback_arg  Is an argument (a pointer) that is passed to the callback function when it is called.
*
*              p_err           Is a pointer to an error code.  '*p_err' will contain one of the following:
*
*                                 OS_ERR_NONE                    The timer was configured as expected
*                                 OS_ERR_OBJ_TYPE                If the object type is invalid
*                                 OS_ERR_OS_NOT_RUNNING          If uC/OS-III is not running yet
*                                 OS_ERR_TMR_INVALID             If 'p_tmr' is a NULL pointer or invalid option
*                                 OS_ERR_TMR_INVALID_CALLBACK    you specified an invalid callback for a periodic timer
*                                 OS_ERR_TMR_INVALID_DLY         You specified an invalid delay
*                                 OS_ERR_TMR_INVALID_PERIOD      You specified an invalid period
*                                 OS_ERR_TMR_ISR                 If the call was made from an ISR
*
* Returns    : none
*
* Note(s)    : 1) This function can be called on a running timer. The change to the delay and period will only
*                 take effect after the current period or delay has passed. Change to the callback will take
*                 effect immediately.
************************************************************************************************************************
*/

void  OSTmrSet (OS_TMR               *p_tmr,
                OS_TICK               dly,
                OS_TICK               period,
                OS_TMR_CALLBACK_PTR   p_callback,
                void                 *p_callback_arg,
                OS_ERR               *p_err)
{
#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return;
    }
#endif

#if (OS_CFG_CALLED_FROM_ISR_CHK_EN > 0u)
    if (OSIntNestingCtr > 0u) {                                 /* See if trying to call from an ISR                    */
       *p_err = OS_ERR_TMR_ISR;
        return;
    }
#endif

#if (OS_CFG_INVALID_OS_CALLS_CHK_EN > 0u)
    if (OSRunning != OS_STATE_OS_RUNNING) {                     /* Is the kernel running?                               */
       *p_err = OS_ERR_OS_NOT_RUNNING;
        return;
    }
#endif

#if (OS_CFG_ARG_CHK_EN > 0u)
    if (p_tmr == (OS_TMR *)0) {                                 /* Validate 'p_tmr'                                     */
       *p_err = OS_ERR_TMR_INVALID;
        return;
    }
#endif

#if (OS_CFG_OBJ_TYPE_CHK_EN > 0u)
    if (p_tmr->Type != OS_OBJ_TYPE_TMR) {                       /* Make sure timer was created                          */
       *p_err = OS_ERR_OBJ_TYPE;
        return;
    }
#endif

#if (OS_CFG_ARG_CHK_EN > 0u)
    switch (p_tmr->Opt) {
        case OS_OPT_TMR_PERIODIC:
             if (period == 0u) {
                *p_err = OS_ERR_TMR_INVALID_PERIOD;
                 return;
             }

             if (p_callback == (OS_TMR_CALLBACK_PTR)0) {        /* No point in a periodic timer without a callback      */
                *p_err = OS_ERR_TMR_INVALID_CALLBACK;
                 return;
             }
             break;

        case OS_OPT_TMR_ONE_SHOT:
             if (dly == 0u) {
                *p_err = OS_ERR_TMR_INVALID_DLY;
                 return;
             }
             break;

        default:
            *p_err = OS_ERR_TMR_INVALID;
             return;
    }
#endif

    OS_TmrLock();

    p_tmr->Dly            = dly    * OSTmrToTicksMult;             /* Convert Timer Delay  to ticks                     */
    p_tmr->Period         = period * OSTmrToTicksMult;             /* Convert Timer Period to ticks                     */
    p_tmr->CallbackPtr    = p_callback;
    p_tmr->CallbackPtrArg = p_callback_arg;

   *p_err                 = OS_ERR_NONE;

    OS_TmrUnlock();
}


/*
************************************************************************************************************************
*                                                   START A TIMER
*
* Description: This function is called by your application code to start a timer.
*
* Arguments  : p_tmr    Is a pointer to an OS_TMR
*
*              p_err    Is a pointer to an error code.  '*p_err' will contain one of the following:
*
*                           OS_ERR_NONE                The timer was started
*                           OS_ERR_OBJ_TYPE            If 'p_tmr' is not pointing to a timer
*                           OS_ERR_OS_NOT_RUNNING      If uC/OS-III is not running yet
*                           OS_ERR_TMR_INACTIVE        If the timer was not created
*                           OS_ERR_TMR_INVALID         If 'p_tmr' is a NULL pointer
*                           OS_ERR_TMR_INVALID_STATE   The timer is in an invalid state
*                           OS_ERR_TMR_ISR             If the call was made from an ISR
*
* Returns    : OS_TRUE   is the timer was started
*              OS_FALSE  if not or upon an error
*
* Note(s)    : 1) When starting/restarting a timer, regardless if it is in PERIODIC or ONE-SHOT mode, the timer is
*                 linked to the timer list with the OS_OPT_LINK_DLY option. This option sets the initial expiration
*                 time for the timer. For timers in PERIODIC mode, subsequent expiration times are handled by
*                 the OS_TmrTask().
************************************************************************************************************************
*/

CPU_BOOLEAN  OSTmrStart (OS_TMR  *p_tmr,
                         OS_ERR  *p_err)
{
    CPU_BOOLEAN  success;
    OS_TICK      time;
    CPU_SR_ALLOC();


#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return (OS_FALSE);
    }
#endif

#if (OS_CFG_CALLED_FROM_ISR_CHK_EN > 0u)
    if (OSIntNestingCtr > 0u) {                                 /* See if trying to call from an ISR                    */
       *p_err = OS_ERR_TMR_ISR;
        return (OS_FALSE);
    }
#endif

#if (OS_CFG_INVALID_OS_CALLS_CHK_EN > 0u)
    if (OSRunning != OS_STATE_OS_RUNNING) {                     /* Is the kernel running?                               */
       *p_err = OS_ERR_OS_NOT_RUNNING;
        return (OS_FALSE);
    }
#endif

#if (OS_CFG_ARG_CHK_EN > 0u)
    if (p_tmr == (OS_TMR *)0) {
       *p_err = OS_ERR_TMR_INVALID;
        return (OS_FALSE);
    }
#endif

#if (OS_CFG_OBJ_TYPE_CHK_EN > 0u)
    if (p_tmr->Type != OS_OBJ_TYPE_TMR) {                       /* Make sure timer was created                          */
       *p_err = OS_ERR_OBJ_TYPE;
        return (OS_FALSE);
    }
#endif

    OS_TmrLock();

    CPU_CRITICAL_ENTER();
    if (OSTCBCurPtr == &OSTmrTaskTCB) {                         /* Callbacks operate on the Tmr Task's tick base.       */
        time = OSTmrTaskTickBase;
    } else {
#if (OS_CFG_DYN_TICK_EN > 0u)
        time = OSTickCtr + OS_DynTickGet();
#else
        time = OSTickCtr;
#endif
    }
    CPU_CRITICAL_EXIT();


    switch (p_tmr->State) {
        case OS_TMR_STATE_RUNNING:                              /* Restart the timer                                    */
        case OS_TMR_STATE_TIMEOUT:
             p_tmr->State = OS_TMR_STATE_RUNNING;
             OS_TmrUnlink(p_tmr, time);                         /* Remove from current position in List                 */
             if (p_tmr->Dly == 0u) {
                 p_tmr->Remain = p_tmr->Period;
             } else {
                 p_tmr->Remain = p_tmr->Dly;
             }
             OS_TmrLink(p_tmr, time);                           /* Add timer to List                                    */
            *p_err   = OS_ERR_NONE;
             success = OS_TRUE;
             break;

        case OS_TMR_STATE_STOPPED:                              /* Start the timer                                      */
        case OS_TMR_STATE_COMPLETED:
             p_tmr->State = OS_TMR_STATE_RUNNING;
             if (p_tmr->Dly == 0u) {
                 p_tmr->Remain = p_tmr->Period;
             } else {
                 p_tmr->Remain = p_tmr->Dly;
             }
             OS_TmrLink(p_tmr, time);                           /* Add timer to List                                    */
            *p_err   = OS_ERR_NONE;
             success = OS_TRUE;
             break;

        case OS_TMR_STATE_UNUSED:                               /* Timer not created                                    */
            *p_err   = OS_ERR_TMR_INACTIVE;
             success = OS_FALSE;
             break;

        default:
            *p_err   = OS_ERR_TMR_INVALID_STATE;
             success = OS_FALSE;
             break;
    }

    OS_TmrUnlock();

    return (success);
}


/*
************************************************************************************************************************
*                                           FIND OUT WHAT STATE A TIMER IS IN
*
* Description: This function is called to determine what state the timer is in:
*
*                  OS_TMR_STATE_UNUSED     the timer has not been created
*                  OS_TMR_STATE_STOPPED    the timer has been created but has not been started or has been stopped
*                  OS_TMR_STATE_COMPLETED  the timer is in ONE-SHOT mode and has completed it's timeout
*                  OS_TMR_SATE_RUNNING     the timer is currently running
*
* Arguments  : p_tmr    Is a pointer to the desired timer
*
*              p_err    Is a pointer to an error code.  '*p_err' will contain one of the following:
*
*                           OS_ERR_NONE               The return value reflects the state of the timer
*                           OS_ERR_OBJ_TYPE           If 'p_tmr' is not pointing to a timer
*                           OS_ERR_OS_NOT_RUNNING     If uC/OS-III is not running yet
*                           OS_ERR_TMR_INVALID        If 'p_tmr' is a NULL pointer
*                           OS_ERR_TMR_INVALID_STATE  If the timer is not in a valid state
*                           OS_ERR_TMR_ISR            If the call was made from an ISR
*
* Returns    : The current state of the timer (see description).
*
* Note(s)    : none
************************************************************************************************************************
*/

OS_STATE  OSTmrStateGet (OS_TMR  *p_tmr,
                         OS_ERR  *p_err)
{
    OS_STATE  state;



#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return (OS_TMR_STATE_UNUSED);
    }
#endif

#if (OS_CFG_CALLED_FROM_ISR_CHK_EN > 0u)
    if (OSIntNestingCtr > 0u) {                                 /* See if trying to call from an ISR                    */
       *p_err = OS_ERR_TMR_ISR;
        return (OS_TMR_STATE_UNUSED);
    }
#endif

#if (OS_CFG_INVALID_OS_CALLS_CHK_EN > 0u)
    if (OSRunning != OS_STATE_OS_RUNNING) {                     /* Is the kernel running?                               */
       *p_err = OS_ERR_OS_NOT_RUNNING;
        return (OS_TMR_STATE_UNUSED);
    }
#endif

#if (OS_CFG_ARG_CHK_EN > 0u)
    if (p_tmr == (OS_TMR *)0) {
       *p_err = OS_ERR_TMR_INVALID;
        return (OS_TMR_STATE_UNUSED);
    }
#endif

#if (OS_CFG_OBJ_TYPE_CHK_EN > 0u)
    if (p_tmr->Type != OS_OBJ_TYPE_TMR) {                       /* Make sure timer was created                          */
       *p_err = OS_ERR_OBJ_TYPE;
        return (OS_TMR_STATE_UNUSED);
    }
#endif

    OS_TmrLock();

    state = p_tmr->State;
    switch (state) {
        case OS_TMR_STATE_UNUSED:
        case OS_TMR_STATE_STOPPED:
        case OS_TMR_STATE_COMPLETED:
        case OS_TMR_STATE_RUNNING:
        case OS_TMR_STATE_TIMEOUT:
            *p_err = OS_ERR_NONE;
             break;

        default:
            *p_err = OS_ERR_TMR_INVALID_STATE;
             break;
    }

    OS_TmrUnlock();

    return (state);
}


/*
************************************************************************************************************************
*                                                    STOP A TIMER
*
* Description: This function is called by your application code to stop a timer.
*
* Arguments  : p_tmr          Is a pointer to the timer to stop.
*
*              opt            Allows you to specify an option to this functions which can be:
*
*                               OS_OPT_TMR_NONE            Do nothing special but stop the timer
*                               OS_OPT_TMR_CALLBACK        Execute the callback function, pass it the callback argument
*                                                          specified when the timer was created.
*                               OS_OPT_TMR_CALLBACK_ARG    Execute the callback function, pass it the callback argument
*                                                          specified in THIS function call
*
*              callback_arg   Is a pointer to a 'new' callback argument that can be passed to the callback function
*                               instead of the timer's callback argument.  In other words, use 'callback_arg' passed in
*                               THIS function INSTEAD of p_tmr->OSTmrCallbackArg
*
*              p_err          Is a pointer to an error code.  '*p_err' will contain one of the following:
*
*                               OS_ERR_NONE                The timer has stopped
*                               OS_ERR_OBJ_TYPE            If 'p_tmr' is not pointing to a timer
*                               OS_ERR_OPT_INVALID         If you specified an invalid option for 'opt'
*                               OS_ERR_OS_NOT_RUNNING      If uC/OS-III is not running yet
*                               OS_ERR_TMR_INACTIVE        If the timer was not created
*                               OS_ERR_TMR_INVALID         If 'p_tmr' is a NULL pointer
*                               OS_ERR_TMR_INVALID_STATE   The timer is in an invalid state
*                               OS_ERR_TMR_ISR             If the function was called from an ISR
*                               OS_ERR_TMR_NO_CALLBACK     If the timer does not have a callback function defined
*                               OS_ERR_TMR_STOPPED         If the timer was already stopped
*
* Returns    : OS_TRUE   If we stopped the timer (if the timer is already stopped, we also return OS_TRUE)
*              OS_FALSE  If not
*
* Note(s)    : none
************************************************************************************************************************
*/

CPU_BOOLEAN  OSTmrStop (OS_TMR  *p_tmr,
                        OS_OPT   opt,
                        void    *p_callback_arg,
                        OS_ERR  *p_err)
{
    OS_TMR_CALLBACK_PTR  p_fnct;
    CPU_BOOLEAN          success;
    OS_TICK              time;
    CPU_SR_ALLOC();


#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return (OS_FALSE);
    }
#endif

#if (OS_CFG_CALLED_FROM_ISR_CHK_EN > 0u)
    if (OSIntNestingCtr > 0u) {                                 /* See if trying to call from an ISR                    */
       *p_err = OS_ERR_TMR_ISR;
        return (OS_FALSE);
    }
#endif

#if (OS_CFG_INVALID_OS_CALLS_CHK_EN > 0u)
    if (OSRunning != OS_STATE_OS_RUNNING) {                     /* Is the kernel running?                               */
       *p_err = OS_ERR_OS_NOT_RUNNING;
        return (OS_FALSE);
    }
#endif

#if (OS_CFG_ARG_CHK_EN > 0u)
    if (p_tmr == (OS_TMR *)0) {
       *p_err = OS_ERR_TMR_INVALID;
        return (OS_FALSE);
    }
#endif

#if (OS_CFG_OBJ_TYPE_CHK_EN > 0u)
    if (p_tmr->Type != OS_OBJ_TYPE_TMR) {                       /* Make sure timer was created                          */
       *p_err = OS_ERR_OBJ_TYPE;
        return (OS_FALSE);
    }
#endif

    OS_TmrLock();

    CPU_CRITICAL_ENTER();
    if (OSTCBCurPtr == &OSTmrTaskTCB) {                         /* Callbacks operate on the Tmr Task's tick base.       */
        time = OSTmrTaskTickBase;
    } else {
#if (OS_CFG_DYN_TICK_EN > 0u)
        time = OSTickCtr + OS_DynTickGet();
#else
        time = OSTickCtr;
#endif
    }
    CPU_CRITICAL_EXIT();

    switch (p_tmr->State) {
        case OS_TMR_STATE_RUNNING:
        case OS_TMR_STATE_TIMEOUT:
             p_tmr->State = OS_TMR_STATE_STOPPED;               /* Ensure that any callbacks see the stop state         */
             switch (opt) {
                 case OS_OPT_TMR_CALLBACK:
                      OS_TmrUnlink(p_tmr, time);                /* Remove from timer list                               */
                      p_fnct = p_tmr->CallbackPtr;              /* Execute callback function ...                        */
                      if (p_fnct != (OS_TMR_CALLBACK_PTR)0) {   /* ... if available                                     */
                        (*p_fnct)(p_tmr, p_tmr->CallbackPtrArg);/* Use callback arg when timer was created              */
                      } else {
                         *p_err = OS_ERR_TMR_NO_CALLBACK;
                      }
                      break;

                 case OS_OPT_TMR_CALLBACK_ARG:
                      OS_TmrUnlink(p_tmr, time);                /* Remove from timer list                               */
                      p_fnct = p_tmr->CallbackPtr;              /* Execute callback function if available ...           */
                      if (p_fnct != (OS_TMR_CALLBACK_PTR)0) {
                        (*p_fnct)(p_tmr, p_callback_arg);       /* .. using the 'callback_arg' provided in call         */
                      } else {
                         *p_err = OS_ERR_TMR_NO_CALLBACK;
                      }
                      break;

                 case OS_OPT_TMR_NONE:
                      OS_TmrUnlink(p_tmr, time);                /* Remove from timer list                               */
                      break;

                 default:
                      OS_TmrUnlock();
                     *p_err = OS_ERR_OPT_INVALID;
                      return (OS_FALSE);
             }
            *p_err        = OS_ERR_NONE;
             success      = OS_TRUE;
             break;

        case OS_TMR_STATE_COMPLETED:                            /* Timer has already completed the ONE-SHOT or          */
        case OS_TMR_STATE_STOPPED:                              /* ... timer has not started yet.                       */
             p_tmr->State = OS_TMR_STATE_STOPPED;
            *p_err        = OS_ERR_TMR_STOPPED;
             success      = OS_TRUE;
             break;

        case OS_TMR_STATE_UNUSED:                               /* Timer was not created                                */
            *p_err        = OS_ERR_TMR_INACTIVE;
             success      = OS_FALSE;
             break;

        default:
            *p_err        = OS_ERR_TMR_INVALID_STATE;
             success      = OS_FALSE;
             break;
    }

    OS_TmrUnlock();

    return (success);
}


/*
************************************************************************************************************************
*                                                 CLEAR TIMER FIELDS
*
* Description: This function is called to clear all timer fields.
*
* Argument(s): p_tmr    Is a pointer to the timer to clear
*              -----
*
* Returns    : none
*
* Note(s)    : 1) This function is INTERNAL to uC/OS-III and your application MUST NOT call it.
************************************************************************************************************************
*/

void  OS_TmrClr (OS_TMR  *p_tmr)
{
    p_tmr->State          = OS_TMR_STATE_UNUSED;                /* Clear timer fields                                   */
#if (OS_OBJ_TYPE_REQ > 0u)
    p_tmr->Type           = OS_OBJ_TYPE_NONE;
#endif
#if (OS_CFG_DBG_EN > 0u)
    p_tmr->NamePtr        = (CPU_CHAR *)((void *)"?TMR");
#endif
    p_tmr->Dly            =                      0u;
    p_tmr->Remain         =                      0u;
    p_tmr->Period         =                      0u;
    p_tmr->Opt            =                      0u;
    p_tmr->CallbackPtr    = (OS_TMR_CALLBACK_PTR)0;
    p_tmr->CallbackPtrArg = (void              *)0;
    p_tmr->NextPtr        = (OS_TMR            *)0;
    p_tmr->PrevPtr        = (OS_TMR            *)0;
}


/*
************************************************************************************************************************
*                                         ADD/REMOVE TIMER TO/FROM DEBUG TABLE
*
* Description: These functions are called by uC/OS-III to add or remove a timer to/from a timer debug table.
*
* Arguments  : p_tmr     is a pointer to the timer to add/remove
*
* Returns    : none
*
* Note(s)    : These functions are INTERNAL to uC/OS-III and your application should not call it.
************************************************************************************************************************
*/


#if (OS_CFG_DBG_EN > 0u)
void  OS_TmrDbgListAdd (OS_TMR  *p_tmr)
{
    p_tmr->DbgPrevPtr               = (OS_TMR *)0;
    if (OSTmrDbgListPtr == (OS_TMR *)0) {
        p_tmr->DbgNextPtr           = (OS_TMR *)0;
    } else {
        p_tmr->DbgNextPtr           =  OSTmrDbgListPtr;
        OSTmrDbgListPtr->DbgPrevPtr =  p_tmr;
    }
    OSTmrDbgListPtr                 =  p_tmr;
}



void  OS_TmrDbgListRemove (OS_TMR  *p_tmr)
{
    OS_TMR  *p_tmr_next;
    OS_TMR  *p_tmr_prev;


    p_tmr_prev = p_tmr->DbgPrevPtr;
    p_tmr_next = p_tmr->DbgNextPtr;

    if (p_tmr_prev == (OS_TMR *)0) {
        OSTmrDbgListPtr = p_tmr_next;
        if (p_tmr_next != (OS_TMR *)0) {
            p_tmr_next->DbgPrevPtr = (OS_TMR *)0;
        }
        p_tmr->DbgNextPtr = (OS_TMR *)0;

    } else if (p_tmr_next == (OS_TMR *)0) {
        p_tmr_prev->DbgNextPtr = (OS_TMR *)0;
        p_tmr->DbgPrevPtr      = (OS_TMR *)0;

    } else {
        p_tmr_prev->DbgNextPtr =  p_tmr_next;
        p_tmr_next->DbgPrevPtr =  p_tmr_prev;
        p_tmr->DbgNextPtr      = (OS_TMR *)0;
        p_tmr->DbgPrevPtr      = (OS_TMR *)0;
    }
}
#endif


/*
************************************************************************************************************************
*                                             INITIALIZE THE TIMER MANAGER
*
* Description: This function is called by OSInit() to initialize the timer manager module.
*
* Argument(s): p_err    is a pointer to a variable that will contain an error code returned by this function.
*
*                           OS_ERR_NONE
*                           OS_ERR_TMR_STK_INVALID       if you didn't specify a stack for the timer task
*                           OS_ERR_TMR_STK_SIZE_INVALID  if you didn't allocate enough space for the timer stack
*                           OS_ERR_PRIO_INVALID          if you specified the same priority as the idle task
*                           OS_ERR_xxx                   any error code returned by OSTaskCreate()
*
* Returns    : none
*
* Note(s)    : 1) This function is INTERNAL to uC/OS-III and your application MUST NOT call it.
************************************************************************************************************************
*/

void  OS_TmrInit (OS_ERR  *p_err)
{
#if (OS_CFG_DBG_EN > 0u)
    OSTmrQty             =           0u;                        /* Keep track of the number of timers created           */
    OSTmrDbgListPtr      = (OS_TMR *)0;
#endif

    OSTmrListPtr         = (OS_TMR *)0;                         /* Create an empty timer list                           */
#if (OS_CFG_DBG_EN > 0u)
    OSTmrListEntries     =           0u;
#endif
                                                                /* Calculate Timer to Ticks multiplier                  */
    OSTmrToTicksMult = OSCfg_TickRate_Hz / OSCfg_TmrTaskRate_Hz;

#if (OS_CFG_TS_EN > 0u)
    OSTmrTaskTime        =           0u;
    OSTmrTaskTimeMax     =           0u;
#endif

    OSMutexCreate(&OSTmrMutex,                                  /* Use a mutex to protect the timers                    */
#if  (OS_CFG_DBG_EN == 0u)
                  (CPU_CHAR *)0,
#else
                  (CPU_CHAR *)"OS Tmr Mutex",
#endif
                  p_err);
    if (*p_err != OS_ERR_NONE) {
        return;
    }

    OS_TmrCondCreate();
                                                                /* -------------- CREATE THE TIMER TASK --------------- */
    if (OSCfg_TmrTaskStkBasePtr == (CPU_STK *)0) {
       *p_err = OS_ERR_TMR_STK_INVALID;
        return;
    }

    if (OSCfg_TmrTaskStkSize < OSCfg_StkSizeMin) {
       *p_err = OS_ERR_TMR_STK_SIZE_INVALID;
        return;
    }

    if (OSCfg_TmrTaskPrio >= (OS_CFG_PRIO_MAX - 1u)) {
       *p_err = OS_ERR_TMR_PRIO_INVALID;
        return;
    }

    OSTaskCreate(&OSTmrTaskTCB,
#if  (OS_CFG_DBG_EN == 0u)
                 (CPU_CHAR *)0,
#else
                 (CPU_CHAR *)"uC/OS-III Timer Task",
#endif
                  OS_TmrTask,
                 (void     *)0,
                  OSCfg_TmrTaskPrio,
                  OSCfg_TmrTaskStkBasePtr,
                  OSCfg_TmrTaskStkLimit,
                  OSCfg_TmrTaskStkSize,
                  0u,
                  0u,
                 (void     *)0,
                 (OS_OPT_TASK_STK_CHK | (OS_OPT)(OS_OPT_TASK_STK_CLR | OS_OPT_TASK_NO_TLS)),
                  p_err);
}


/*
************************************************************************************************************************
*                                         ADD A TIMER TO THE TIMER LIST
*
* Description: This function is called to add a timer to the timer list.
*
* Arguments  : p_tmr          Is a pointer to the timer to add.
*
*              time           Is the system time when this timer was linked.
*              -----
*
* Returns    : none
*
* Note(s)    : 1) This function is INTERNAL to uC/OS-III and your application MUST NOT call it.
************************************************************************************************************************
*/

void OS_TmrLink (OS_TMR   *p_tmr,
                 OS_TICK   time)
{
    OS_TMR   *p_tmr1;
    OS_TMR   *p_tmr2;
    OS_TICK   remain;
    OS_TICK   delta;


    if (OSTmrListPtr == (OS_TMR *)0) {                          /* Is the list empty?                                   */
        p_tmr->NextPtr    = (OS_TMR *)0;                        /* Yes, this is the first entry                         */
        p_tmr->PrevPtr    = (OS_TMR *)0;
        OSTmrListPtr      = p_tmr;
#if (OS_CFG_DBG_EN > 0u)
        OSTmrListEntries  = 1u;
#endif
        OSTmrTaskTickBase = time;
        OS_TmrCondSignal();

        return;
    }

#if (OS_CFG_DBG_EN > 0u)
    OSTmrListEntries++;
#endif

    delta = (time + p_tmr->Remain) - OSTmrTaskTickBase;

    p_tmr2 = OSTmrListPtr;                                      /* No,  Insert somewhere in the list in delta order     */
    remain = p_tmr2->Remain;

    if ((delta           <     remain) &&
        (p_tmr2->PrevPtr == (OS_TMR *)0)) {                     /* Are we the new head of the list?                     */
        p_tmr2->Remain    =  remain - delta;
        p_tmr->PrevPtr    = (OS_TMR *)0;
        p_tmr->NextPtr    =  p_tmr2;
        p_tmr2->PrevPtr   =  p_tmr;
        OSTmrListPtr      =  p_tmr;

        OSTmrTaskTickBase = time;
        OS_TmrCondSignal();

        return;
    }

                                                                /* No                                                   */
    delta  -= remain;                                           /* Make delta relative to the current head.             */
    p_tmr1  = p_tmr2;
    p_tmr2  = p_tmr1->NextPtr;


    while ((p_tmr2 !=        (OS_TMR *)0) &&                    /* Find the appropriate position in the delta list.     */
           (delta  >= p_tmr2->Remain)) {
        delta  -= p_tmr2->Remain;                               /* Update our delta as we traverse the list.            */
        p_tmr1  = p_tmr2;
        p_tmr2  = p_tmr2->NextPtr;
    }


    if (p_tmr2 != (OS_TMR *)0) {                                /* Our entry is not the last element in the list.       */
        p_tmr1           = p_tmr2->PrevPtr;
        p_tmr->Remain    = delta;                               /* Store remaining time                                 */
        p_tmr->PrevPtr   = p_tmr1;
        p_tmr->NextPtr   = p_tmr2;
        p_tmr2->Remain  -= delta;                               /* Reduce time of next entry in the list                */
        p_tmr2->PrevPtr  = p_tmr;
        p_tmr1->NextPtr  = p_tmr;

    } else {                                                    /* Our entry belongs at the end of the list.            */
        p_tmr->Remain    = delta;
        p_tmr->PrevPtr   = p_tmr1;
        p_tmr->NextPtr   = (OS_TMR *)0;
        p_tmr1->NextPtr  = p_tmr;
    }
}


/*
************************************************************************************************************************
*                                         REMOVE A TIMER FROM THE TIMER LIST
*
* Description: This function is called to remove the timer from the timer list.
*
* Arguments  : p_tmr          Is a pointer to the timer to remove.
*
*              time           Is the system time when this timer was unlinked.
*              -----
*
* Returns    : none
*
* Note(s)    : 1) This function is INTERNAL to uC/OS-III and your application MUST NOT call it.
************************************************************************************************************************
*/

void  OS_TmrUnlink (OS_TMR   *p_tmr,
                    OS_TICK   time)
{
    OS_TMR   *p_tmr1;
    OS_TMR   *p_tmr2;
    OS_TICK   elapsed;


    p_tmr1                          = p_tmr->PrevPtr;
    p_tmr2                          = p_tmr->NextPtr;
    if (p_tmr1 == (OS_TMR *)0) {
        if (p_tmr2 == (OS_TMR *)0) {                            /* Remove the ONLY entry in the list?                   */
            OSTmrListPtr            = (OS_TMR *)0;
#if (OS_CFG_DBG_EN > 0u)
            OSTmrListEntries        = 0u;
#endif
            p_tmr->Remain           = 0u;

            OSTmrTaskTickBase       = time;
            OS_TmrCondSignal();
        } else {
#if (OS_CFG_DBG_EN > 0u)
            OSTmrListEntries--;
#endif
            elapsed                 = time - OSTmrTaskTickBase;
            p_tmr2->PrevPtr         = (OS_TMR *)0;
            p_tmr2->Remain         += p_tmr->Remain;            /* Add back the ticks to the delta                      */
            OSTmrListPtr            = p_tmr2;

            while ((elapsed >           0u) &&
                   (p_tmr2  != (OS_TMR *)0)) {

                if (elapsed > p_tmr2->Remain) {
                    elapsed        -= p_tmr2->Remain;
                    p_tmr2->Remain  = 0u;
                } else {
                    p_tmr2->Remain -= elapsed;
                    elapsed         = 0u;
                }


                p_tmr1              = p_tmr2;
                p_tmr2              = p_tmr1->NextPtr;
            }

            if ((OSTmrListPtr->Remain != p_tmr->Remain) ||      /* Reload if new head has a different delay         ... */
                (OSTmrListPtr->Remain ==            0u)) {      /* ... or has already timed out.                        */
                OSTmrTaskTickBase   = time;
                OS_TmrCondSignal();
            }

            p_tmr->NextPtr          = (OS_TMR *)0;
            p_tmr->Remain           =           0u;
        }
    } else {
#if (OS_CFG_DBG_EN > 0u)
        OSTmrListEntries--;
#endif
        p_tmr1->NextPtr             = p_tmr2;
        if (p_tmr2 != (OS_TMR *)0) {
            p_tmr2->PrevPtr         = p_tmr1;
            p_tmr2->Remain         += p_tmr->Remain;            /* Add back the ticks to the delta list                 */
        }
        p_tmr->PrevPtr              = (OS_TMR *)0;
        p_tmr->NextPtr              = (OS_TMR *)0;
        p_tmr->Remain               =           0u;
    }
}


/*
************************************************************************************************************************
*                                                 TIMER MANAGEMENT TASK
*
* Description: This task is created by OS_TmrInit().
*
* Arguments  : none
*
* Returns    : none
*
* Note(s)    : 1) This function is INTERNAL to uC/OS-III and your application MUST NOT call it.
*
*              2) The timer list is processed in two stages.
*                   a) Subtract the expired time from the delta list, which leaves expired timers at the head.
*                   b) Process each of the expired timers by invoking its callback (if any) and removing it.
*                 This method allows timer callbacks to Link/Unlink timers while maintaining the correct delta values.
*
*              3) Timer callbacks are allowed to make calls to the Timer APIs.
************************************************************************************************************************
*/

void  OS_TmrTask (void  *p_arg)
{
    OS_TMR_CALLBACK_PTR   p_fnct;
    OS_TMR               *p_tmr;
    OS_TICK               timeout;
    OS_TICK               elapsed;
    OS_TICK               time;
#if (OS_CFG_TS_EN > 0u)
    CPU_TS                ts_start;
#endif
    CPU_SR_ALLOC();


    (void)p_arg;                                                /* Not using 'p_arg', prevent compiler warning          */

    OS_TmrLock();

    for (;;) {
        if (OSTmrListPtr == (OS_TMR *)0) {
            timeout                = 0u;
        } else {
            timeout                = OSTmrListPtr->Remain;
        }

        OS_TmrCondWait(timeout);                                /* Suspend the timer task until it needs to process ... */
                                                                /* ... the timer list again. Also release the mutex ... */
                                                                /* ... so that application tasks can add/remove timers. */

        if (OSTmrListPtr == (OS_TMR *)0) {                      /* Suppresses static analyzer warnings.                 */
            continue;
        }

#if (OS_CFG_TS_EN > 0u)
        ts_start = OS_TS_GET();
#endif

        CPU_CRITICAL_ENTER();
#if (OS_CFG_DYN_TICK_EN > 0u)
        time                       = OSTickCtr + OS_DynTickGet();
#else
        time                       = OSTickCtr;
#endif
        CPU_CRITICAL_EXIT();
        elapsed                    = time - OSTmrTaskTickBase;
        OSTmrTaskTickBase          = time;

                                                                /* Update the delta values.                             */
        p_tmr = OSTmrListPtr;
        while ((elapsed !=          0u) &&
               (p_tmr   != (OS_TMR *)0)) {

            if (elapsed > p_tmr->Remain) {
                elapsed           -= p_tmr->Remain;
                p_tmr->Remain      = 0u;
            } else {
                p_tmr->Remain     -= elapsed;
                elapsed            = 0u;
            }

            p_tmr                  = p_tmr->NextPtr;
        }

                                                                /* Process timers that have expired.                    */
        p_tmr                      = OSTmrListPtr;

        while ((p_tmr         != (OS_TMR *)0) &&
               (p_tmr->Remain ==          0u)) {
            p_tmr->State           = OS_TMR_STATE_TIMEOUT;
                                                                /* Execute callback function if available               */
            p_fnct                 = p_tmr->CallbackPtr;
            if (p_fnct != (OS_TMR_CALLBACK_PTR)0u) {
                (*p_fnct)(p_tmr, p_tmr->CallbackPtrArg);
            }

            if (p_tmr->State == OS_TMR_STATE_TIMEOUT) {
                OS_TmrUnlink(p_tmr, OSTmrTaskTickBase);

                if (p_tmr->Opt == OS_OPT_TMR_PERIODIC) {
                    p_tmr->State   = OS_TMR_STATE_RUNNING;
                    p_tmr->Remain  = p_tmr->Period;
                    OS_TmrLink(p_tmr, OSTmrTaskTickBase);
                } else {
                    p_tmr->PrevPtr = (OS_TMR *)0;
                    p_tmr->NextPtr = (OS_TMR *)0;
                    p_tmr->Remain  = 0u;
                    p_tmr->State   = OS_TMR_STATE_COMPLETED;
                }
            }

            p_tmr                  = OSTmrListPtr;
        }

#if (OS_CFG_TS_EN > 0u)
        OSTmrTaskTime = OS_TS_GET() - ts_start;                 /* Measure execution time of timer task                 */
        if (OSTmrTaskTimeMax < OSTmrTaskTime) {
            OSTmrTaskTimeMax       = OSTmrTaskTime;
        }
#endif
    }
}


/*
************************************************************************************************************************
*                                          TIMER MANAGEMENT LOCKING MECHANISM
*
* Description: These functions are used to handle timer critical sections.  The method uses a mutex
*              to protect access to the global timer list.
*
* Arguments  : none
*
* Returns    : none
*
* Note(s)    : 1) These functions are INTERNAL to uC/OS-III and your application MUST NOT call them.
************************************************************************************************************************
*/

static  void  OS_TmrLock (void)
{
    OS_ERR  err;


    OSMutexPend(&OSTmrMutex, 0u, OS_OPT_PEND_BLOCKING, (CPU_TS *)0, &err);
}


static  void  OS_TmrUnlock (void)
{
    OS_ERR  err;


    OSMutexPost(&OSTmrMutex, OS_OPT_POST_NONE, &err);
}


/*
************************************************************************************************************************
*                                         CREATE TIMER TASK CONDITION VARIABLE
*
* Description: Initializes a condition variable for INTERNAL use ONLY.
*
* Arguments  : none
*
* Returns    : none
*
* Note(s)    : 1) This function is INTERNAL to uC/OS-III and your application MUST NOT call it.
************************************************************************************************************************
*/

static  void  OS_TmrCondCreate (void)
{
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
#if (OS_OBJ_TYPE_REQ > 0u)
    OSTmrCond.Type  = OS_OBJ_TYPE_COND;                         /* Mark the data structure as a condition variable.     */
#endif
    OSTmrCond.Mutex = &OSTmrMutex;                              /* Bind the timer mutex to the condition variable.      */
    OS_PendListInit(&OSTmrCond.PendList);                       /* Initialize the waiting list                          */
    CPU_CRITICAL_EXIT();
}


/*
************************************************************************************************************************
*                                         WAIT ON TIMER TASK CONDITION VARIABLE
*
* Description: Allows the timer task to release the global mutex and pend atomically. This ensures that
*              timers are only added/removed after the timer task has processed the current list and pended
*              for the next timeout. The timer task will always acquire the mutex before returning from this function.
*
* Arguments  : timeout                   The number of ticks before the timer task will wake up.
*                                        A value of zero signifies an indefinite pend.
*
* Returns    : none
*
* Note(s)    : 1) This function is INTERNAL to uC/OS-III and your application MUST NOT call it.
************************************************************************************************************************
*/

static  void  OS_TmrCondWait (OS_TICK  timeout)
{
    OS_TCB        *p_tcb;
    OS_PEND_LIST  *p_pend_list;
    CPU_TS         ts;
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
#if (OS_CFG_TS_EN > 0u)
    ts             = OS_TS_GET();                               /* Get timestamp                                        */
    OSTmrMutex.TS  = ts;
#else
    ts             = 0u;
#endif
                                                                /* Release mutex to other tasks.                        */
    OS_MutexGrpRemove(&OSTmrTaskTCB, &OSTmrMutex);
    p_pend_list                    = &OSTmrMutex.PendList;

    if (OSTmrTaskTCB.Prio != OSTmrTaskTCB.BasePrio) {           /* Restore our original prio.                           */
        OS_TRACE_MUTEX_TASK_PRIO_DISINHERIT(&OSTmrTaskTCB, OSTmrTaskTCB.Prio);
        OSTmrTaskTCB.Prio          = OSTmrTaskTCB.BasePrio;
        OSPrioCur                  = OSTmrTaskTCB.BasePrio;
    }

    if (p_pend_list->HeadPtr == (OS_TCB *)0) {                  /* Any task waiting on mutex?                           */
        OSTmrMutex.OwnerTCBPtr     = (OS_TCB *)0;               /* No                                                   */
        OSTmrMutex.OwnerNestingCtr =           0u;
    } else {
        p_tcb                      = p_pend_list->HeadPtr;      /* Yes, give mutex to new owner                         */
        OSTmrMutex.OwnerTCBPtr     = p_tcb;
        OSTmrMutex.OwnerNestingCtr =           1u;
        OS_MutexGrpAdd(p_tcb, &OSTmrMutex);
                                                                /* Post to mutex                                        */
        OS_Post((OS_PEND_OBJ *)((void *)&OSTmrMutex),
                                         p_tcb,
                                (void *) 0,
                                         0u,
                                         ts);
    }

    OS_Pend((OS_PEND_OBJ *)((void *)&OSTmrCond),                /* Pend on the condition variable.                      */
                                   &OSTmrTaskTCB,
                                    OS_TASK_PEND_ON_COND,
                                    timeout);
    CPU_CRITICAL_EXIT();

    OSSched();

    CPU_CRITICAL_ENTER();                                       /* Either we timed out, or were signaled.               */

    if (OSTmrMutex.OwnerTCBPtr == (OS_TCB *)0) {                /* Can we grab the mutex?                               */
        OS_MutexGrpAdd(&OSTmrTaskTCB, &OSTmrMutex);             /* Yes, no-one else pending.                            */
        OSTmrMutex.OwnerTCBPtr     = &OSTmrTaskTCB;
        OSTmrMutex.OwnerNestingCtr = 1u;
        CPU_CRITICAL_EXIT();
    } else {
        p_tcb = OSTmrMutex.OwnerTCBPtr;                         /* No, we need to wait for it.                          */
        if (p_tcb->Prio > OSTmrTaskTCB.Prio) {                  /* See if mutex owner has a lower priority than TmrTask.*/
            OS_TaskChangePrio(p_tcb, OSTmrTaskTCB.Prio);
        }

        OS_Pend((OS_PEND_OBJ *)((void *)&OSTmrMutex),           /* Block TmrTask until it gets the Mutex.               */
                                        &OSTmrTaskTCB,
                                         OS_TASK_PEND_ON_MUTEX,
                                         0u);
        CPU_CRITICAL_EXIT();

        OSSched();
    }
}


/*
************************************************************************************************************************
*                                       SIGNAL THE TIMER TASK CONDITION VARIABLE
*
* Description: Used to signal the timer task when a timer is added/removed which requires the task to reload
*              its timeout. We ensure that this function is always called with the timer mutex locked.
*
* Arguments  : none.
*
* Returns    : none
*
* Note(s)    : 1) This function is INTERNAL to uC/OS-III and your application MUST NOT call it.
************************************************************************************************************************
*/

static  void  OS_TmrCondSignal (void)
{
    OS_PEND_LIST  *p_pend_list;
    CPU_TS         ts;
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
#if (OS_CFG_TS_EN > 0u)
    ts             = OS_TS_GET();                               /* Get timestamp                                        */
    OSTmrMutex.TS  = ts;
#else
    ts             = 0u;
#endif

    p_pend_list    = &OSTmrCond.PendList;

    if (p_pend_list->HeadPtr == (OS_TCB *)0) {                  /* Timer task waiting on cond?                          */
        CPU_CRITICAL_EXIT();
        return;                                                 /* No, nothing to signal.                               */
    } else {
                                                                /* Yes, signal the timer task.                          */
        OS_Post((OS_PEND_OBJ *)((void *)&OSTmrCond),
                                        &OSTmrTaskTCB,
                                (void *) 0,
                                         0u,
                                         ts);
    }

    CPU_CRITICAL_EXIT();
}
#endif
