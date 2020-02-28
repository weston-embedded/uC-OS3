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
*                                         EVENT FLAG MANAGEMENT
*
* File    : os_flag.c
* Version : V3.08.00
*********************************************************************************************************
*/

#define  MICRIUM_SOURCE
#include "os.h"

#ifdef VSC_INCLUDE_SOURCE_FILE_NAMES
const  CPU_CHAR  *os_flag__c = "$Id: $";
#endif


#if (OS_CFG_FLAG_EN > 0u)

/*
************************************************************************************************************************
*                                                 CREATE AN EVENT FLAG
*
* Description: This function is called to create an event flag group.
*
* Arguments  : p_grp          is a pointer to the event flag group to create
*
*              p_name         is the name of the event flag group
*
*              flags          contains the initial value to store in the event flag group (typically 0).
*
*              p_err          is a pointer to an error code which will be returned to your application:
*
*                                 OS_ERR_NONE                    If the call was successful
*                                 OS_ERR_CREATE_ISR              If you attempted to create an Event Flag from an ISR
*                                 OS_ERR_ILLEGAL_CREATE_RUN_TIME If you are trying to create the Event Flag after you
*                                                                   called OSSafetyCriticalStart().
*                                 OS_ERR_OBJ_PTR_NULL            If 'p_grp' is a NULL pointer
*                                 OS_ERR_OBJ_CREATED             If the event flag was already created
*
* Returns    : none
************************************************************************************************************************
*/

void  OSFlagCreate (OS_FLAG_GRP  *p_grp,
                    CPU_CHAR     *p_name,
                    OS_FLAGS      flags,
                    OS_ERR       *p_err)
{
    CPU_SR_ALLOC();


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
    if (OSIntNestingCtr > 0u) {                                 /* See if called from ISR ...                           */
       *p_err = OS_ERR_CREATE_ISR;                              /* ... can't CREATE from an ISR                         */
        return;
    }
#endif

#if (OS_CFG_ARG_CHK_EN > 0u)
    if (p_grp == (OS_FLAG_GRP *)0) {                            /* Validate 'p_grp'                                     */
       *p_err = OS_ERR_OBJ_PTR_NULL;
        return;
    }
#endif

    CPU_CRITICAL_ENTER();
#if (OS_OBJ_TYPE_REQ > 0u)
    if (p_grp->Type == OS_OBJ_TYPE_FLAG) {
        CPU_CRITICAL_EXIT();
        *p_err = OS_ERR_OBJ_CREATED;
        return;
    }
    p_grp->Type    = OS_OBJ_TYPE_FLAG;                          /* Set to event flag group type                         */
#endif
#if (OS_CFG_DBG_EN > 0u)
    p_grp->NamePtr = p_name;
#else
    (void)p_name;
#endif
    p_grp->Flags   = flags;                                     /* Set to desired initial value                         */
#if (OS_CFG_TS_EN > 0u)
    p_grp->TS      = 0u;
#endif
    OS_PendListInit(&p_grp->PendList);

#if (OS_CFG_DBG_EN > 0u)
    OS_FlagDbgListAdd(p_grp);
    OSFlagQty++;
#endif

    OS_TRACE_FLAG_CREATE(p_grp, p_name);

    CPU_CRITICAL_EXIT();
   *p_err = OS_ERR_NONE;
}


/*
************************************************************************************************************************
*                                             DELETE AN EVENT FLAG GROUP
*
* Description: This function deletes an event flag group and readies all tasks pending on the event flag group.
*
* Arguments  : p_grp     is a pointer to the desired event flag group.
*
*              opt       determines delete options as follows:
*
*                            OS_OPT_DEL_NO_PEND           Deletes the event flag group ONLY if no task pending
*                            OS_OPT_DEL_ALWAYS            Deletes the event flag group even if tasks are waiting.
*                                                         In this case, all the tasks pending will be readied.
*
*              p_err     is a pointer to an error code that can contain one of the following values:
*
*                            OS_ERR_NONE                    The call was successful and the event flag group was deleted
*                            OS_ERR_DEL_ISR                 If you attempted to delete the event flag group from an ISR
*                            OS_ERR_ILLEGAL_DEL_RUN_TIME    If you are trying to delete the event flag group after you
*                                                             called OSStart()
*                            OS_ERR_OBJ_PTR_NULL            If 'p_grp' is a NULL pointer
*                            OS_ERR_OBJ_TYPE                If you didn't pass a pointer to an event flag group
*                            OS_ERR_OPT_INVALID             An invalid option was specified
*                            OS_ERR_OS_NOT_RUNNING          If uC/OS-III is not running yet
*                            OS_ERR_TASK_WAITING            One or more tasks were waiting on the event flag group
*
* Returns    : == 0          if no tasks were waiting on the event flag group, or upon error.
*              >  0          if one or more tasks waiting on the event flag group are now readied and informed.
*
* Note(s)    : 1) This function must be used with care.  Tasks that would normally expect the presence of the event flag
*                 group MUST check the return code of OSFlagPost and OSFlagPend().
************************************************************************************************************************
*/

#if (OS_CFG_FLAG_DEL_EN > 0u)
OS_OBJ_QTY  OSFlagDel (OS_FLAG_GRP  *p_grp,
                       OS_OPT        opt,
                       OS_ERR       *p_err)
{
    OS_OBJ_QTY     nbr_tasks;
    OS_PEND_LIST  *p_pend_list;
    OS_TCB        *p_tcb;
    CPU_TS         ts;
    CPU_SR_ALLOC();


#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return (0u);
    }
#endif

    OS_TRACE_FLAG_DEL_ENTER(p_grp, opt);

#ifdef OS_SAFETY_CRITICAL_IEC61508
    if (OSSafetyCriticalStartFlag == OS_TRUE) {
        OS_TRACE_FLAG_DEL_EXIT(OS_ERR_ILLEGAL_DEL_RUN_TIME);
       *p_err = OS_ERR_ILLEGAL_DEL_RUN_TIME;
        return (0u);
    }
#endif

#if (OS_CFG_CALLED_FROM_ISR_CHK_EN > 0u)
    if (OSIntNestingCtr > 0u) {                                 /* See if called from ISR ...                           */
       *p_err = OS_ERR_DEL_ISR;                                 /* ... can't DELETE from an ISR                         */
        OS_TRACE_FLAG_DEL_EXIT(OS_ERR_DEL_ISR);
        return (0u);
    }
#endif

#if (OS_CFG_INVALID_OS_CALLS_CHK_EN > 0u)
    if (OSRunning != OS_STATE_OS_RUNNING) {                     /* Is the kernel running?                               */
        OS_TRACE_FLAG_DEL_EXIT(OS_ERR_OS_NOT_RUNNING);
       *p_err = OS_ERR_OS_NOT_RUNNING;
        return (0u);
    }
#endif

#if (OS_CFG_ARG_CHK_EN > 0u)
    if (p_grp == (OS_FLAG_GRP *)0) {                            /* Validate 'p_grp'                                     */
        OS_TRACE_FLAG_DEL_EXIT(OS_ERR_OBJ_PTR_NULL);
       *p_err  = OS_ERR_OBJ_PTR_NULL;
        return (0u);
    }
#endif

#if (OS_CFG_OBJ_TYPE_CHK_EN > 0u)
    if (p_grp->Type != OS_OBJ_TYPE_FLAG) {                      /* Validate event group object                          */
        OS_TRACE_FLAG_DEL_EXIT(OS_ERR_OBJ_TYPE);
       *p_err = OS_ERR_OBJ_TYPE;
        return (0u);
    }
#endif
    CPU_CRITICAL_ENTER();
    p_pend_list = &p_grp->PendList;
    nbr_tasks   = 0u;
    switch (opt) {
        case OS_OPT_DEL_NO_PEND:                                /* Delete group if no task waiting                      */
             if (p_pend_list->HeadPtr == (OS_TCB *)0) {
#if (OS_CFG_DBG_EN > 0u)
                 OS_FlagDbgListRemove(p_grp);
                 OSFlagQty--;
#endif
                 OS_TRACE_FLAG_DEL(p_grp);
                 OS_FlagClr(p_grp);

                 CPU_CRITICAL_EXIT();

                *p_err = OS_ERR_NONE;
             } else {
                 CPU_CRITICAL_EXIT();
                *p_err = OS_ERR_TASK_WAITING;
             }
             break;

        case OS_OPT_DEL_ALWAYS:                                 /* Always delete the event flag group                   */
#if (OS_CFG_TS_EN > 0u)
             ts = OS_TS_GET();                                  /* Get local time stamp so all tasks get the same time  */
#else
             ts = 0u;
#endif
             while (p_pend_list->HeadPtr != (OS_TCB *)0) {      /* Remove all tasks from the pend list                  */
                 p_tcb = p_pend_list->HeadPtr;
                 OS_PendAbort(p_tcb,
                              ts,
                              OS_STATUS_PEND_DEL);
                 nbr_tasks++;
             }
#if (OS_CFG_DBG_EN > 0u)
             OS_FlagDbgListRemove(p_grp);
             OSFlagQty--;
#endif
             OS_TRACE_FLAG_DEL(p_grp);

             OS_FlagClr(p_grp);
             CPU_CRITICAL_EXIT();

             OSSched();                                         /* Find highest priority task ready to run              */
            *p_err = OS_ERR_NONE;
             break;

        default:
             CPU_CRITICAL_EXIT();
            *p_err = OS_ERR_OPT_INVALID;
             break;
    }

    OS_TRACE_FLAG_DEL_EXIT(*p_err);

    return (nbr_tasks);
}
#endif


/*
************************************************************************************************************************
*                                             WAIT ON AN EVENT FLAG GROUP
*
* Description: This function is called to wait for a combination of bits to be set in an event flag group.  Your
*              application can wait for ANY bit to be set or ALL bits to be set.
*
* Arguments  : p_grp         is a pointer to the desired event flag group.
*
*              flags         Is a bit pattern indicating which bit(s) (i.e. flags) you wish to wait for.
*                            The bits you want are specified by setting the corresponding bits in 'flags'.
*                            e.g. if your application wants to wait for bits 0 and 1 then 'flags' would contain 0x03.
*
*              timeout       is an optional timeout (in clock ticks) that your task will wait for the
*                            desired bit combination.  If you specify 0, however, your task will wait
*                            forever at the specified event flag group or, until a message arrives.
*
*              opt           specifies whether you want ALL bits to be set or ANY of the bits to be set.
*                            You can specify the 'ONE' of the following arguments:
*
*                                OS_OPT_PEND_FLAG_CLR_ALL   You will wait for ALL bits in 'flags' to be clear (0)
*                                OS_OPT_PEND_FLAG_CLR_ANY   You will wait for ANY bit  in 'flags' to be clear (0)
*                                OS_OPT_PEND_FLAG_SET_ALL   You will wait for ALL bits in 'flags' to be set   (1)
*                                OS_OPT_PEND_FLAG_SET_ANY   You will wait for ANY bit  in 'flags' to be set   (1)
*
*                            You can 'ADD' OS_OPT_PEND_FLAG_CONSUME if you want the event flag to be 'consumed' by
*                                      the call.  Example, to wait for any flag in a group AND then clear
*                                      the flags that are present, set 'wait_opt' to:
*
*                                      OS_OPT_PEND_FLAG_SET_ANY + OS_OPT_PEND_FLAG_CONSUME
*
*                            You can also 'ADD' the type of pend with 'ONE' of the two option:
*
*                                OS_OPT_PEND_NON_BLOCKING   Task will NOT block if flags are not available
*                                OS_OPT_PEND_BLOCKING       Task will     block if flags are not available
*
*              p_ts          is a pointer to a variable that will receive the timestamp of when the event flag group was
*                            posted, aborted or the event flag group deleted.  If you pass a NULL pointer (i.e. (CPU_TS *)0)
*                            then you will not get the timestamp.  In other words, passing a NULL pointer is valid and
*                            indicates that you don't need the timestamp.
*
*              p_err         is a pointer to an error code and can be:
*
*                                OS_ERR_NONE                The desired bits have been set within the specified 'timeout'
*                                OS_ERR_OBJ_DEL             If the event group was deleted
*                                OS_ERR_OBJ_PTR_NULL        If 'p_grp' is a NULL pointer.
*                                OS_ERR_OBJ_TYPE            You are not pointing to an event flag group
*                                OS_ERR_OPT_INVALID         You didn't specify a proper 'opt' argument
*                                OS_ERR_OS_NOT_RUNNING      If uC/OS-III is not running yet
*                                OS_ERR_PEND_ABORT          The wait on the flag was aborted
*                                OS_ERR_PEND_ISR            If you tried to PEND from an ISR
*                                OS_ERR_PEND_WOULD_BLOCK    If you specified non-blocking but the flags were not
*                                                           available
*                                OS_ERR_SCHED_LOCKED        If you called this function when the scheduler is locked
*                                OS_ERR_STATUS_INVALID      If the pend status has an invalid value
*                                OS_ERR_TIMEOUT             The bit(s) have not been set in the specified 'timeout'
*                                OS_ERR_TICK_DISABLED       If kernel ticks are disabled and a timeout is specified
*
* Returns    : The flags in the event flag group that made the task ready or, 0 if a timeout or an error
*              occurred.
*
* Note(s)    : This API 'MUST NOT' be called from a timer callback function.
************************************************************************************************************************
*/

OS_FLAGS  OSFlagPend (OS_FLAG_GRP  *p_grp,
                      OS_FLAGS      flags,
                      OS_TICK       timeout,
                      OS_OPT        opt,
                      CPU_TS       *p_ts,
                      OS_ERR       *p_err)
{
    CPU_BOOLEAN  consume;
    OS_FLAGS     flags_rdy;
    OS_OPT       mode;
    CPU_SR_ALLOC();


#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return (0u);
    }
#endif

    OS_TRACE_FLAG_PEND_ENTER(p_grp, flags, timeout, opt, p_ts);

#if (OS_CFG_TICK_EN == 0u)
    if (timeout != 0u) {
       *p_err = OS_ERR_TICK_DISABLED;
        OS_TRACE_FLAG_PEND_FAILED(p_grp);
        OS_TRACE_FLAG_PEND_EXIT(OS_ERR_TICK_DISABLED);
        return ((OS_FLAGS)0);
    }
#endif

#if (OS_CFG_CALLED_FROM_ISR_CHK_EN > 0u)
    if (OSIntNestingCtr > 0u) {                                 /* See if called from ISR ...                           */
        if ((opt & OS_OPT_PEND_NON_BLOCKING) != OS_OPT_PEND_NON_BLOCKING) {
           *p_err = OS_ERR_PEND_ISR;                            /* ... can't PEND from an ISR                           */
            OS_TRACE_FLAG_PEND_FAILED(p_grp);
            OS_TRACE_FLAG_PEND_EXIT(OS_ERR_PEND_ISR);
            return ((OS_FLAGS)0);
        }
    }
#endif

#if (OS_CFG_INVALID_OS_CALLS_CHK_EN > 0u)
    if (OSRunning != OS_STATE_OS_RUNNING) {                     /* Is the kernel running?                               */
        OS_TRACE_FLAG_PEND_EXIT(OS_ERR_OS_NOT_RUNNING);
       *p_err = OS_ERR_OS_NOT_RUNNING;
        return (0u);
    }
#endif

#if (OS_CFG_ARG_CHK_EN > 0u)
    if (p_grp == (OS_FLAG_GRP *)0) {                            /* Validate 'p_grp'                                     */
        OS_TRACE_FLAG_PEND_FAILED(p_grp);
        OS_TRACE_FLAG_PEND_EXIT(OS_ERR_OBJ_PTR_NULL);
       *p_err = OS_ERR_OBJ_PTR_NULL;
        return (0u);
    }
    switch (opt) {                                              /* Validate 'opt'                                       */
        case OS_OPT_PEND_FLAG_CLR_ALL:
        case OS_OPT_PEND_FLAG_CLR_ANY:
        case OS_OPT_PEND_FLAG_SET_ALL:
        case OS_OPT_PEND_FLAG_SET_ANY:
        case OS_OPT_PEND_FLAG_CLR_ALL | OS_OPT_PEND_FLAG_CONSUME:
        case OS_OPT_PEND_FLAG_CLR_ANY | OS_OPT_PEND_FLAG_CONSUME:
        case OS_OPT_PEND_FLAG_SET_ALL | OS_OPT_PEND_FLAG_CONSUME:
        case OS_OPT_PEND_FLAG_SET_ANY | OS_OPT_PEND_FLAG_CONSUME:
        case OS_OPT_PEND_FLAG_CLR_ALL | OS_OPT_PEND_NON_BLOCKING:
        case OS_OPT_PEND_FLAG_CLR_ANY | OS_OPT_PEND_NON_BLOCKING:
        case OS_OPT_PEND_FLAG_SET_ALL | OS_OPT_PEND_NON_BLOCKING:
        case OS_OPT_PEND_FLAG_SET_ANY | OS_OPT_PEND_NON_BLOCKING:
        case OS_OPT_PEND_FLAG_CLR_ALL | (OS_OPT)(OS_OPT_PEND_FLAG_CONSUME | OS_OPT_PEND_NON_BLOCKING):
        case OS_OPT_PEND_FLAG_CLR_ANY | (OS_OPT)(OS_OPT_PEND_FLAG_CONSUME | OS_OPT_PEND_NON_BLOCKING):
        case OS_OPT_PEND_FLAG_SET_ALL | (OS_OPT)(OS_OPT_PEND_FLAG_CONSUME | OS_OPT_PEND_NON_BLOCKING):
        case OS_OPT_PEND_FLAG_SET_ANY | (OS_OPT)(OS_OPT_PEND_FLAG_CONSUME | OS_OPT_PEND_NON_BLOCKING):
             break;

        default:
             OS_TRACE_FLAG_PEND_FAILED(p_grp);
             OS_TRACE_FLAG_PEND_EXIT(OS_ERR_OPT_INVALID);
            *p_err = OS_ERR_OPT_INVALID;
             return (0u);
    }
#endif

#if (OS_CFG_OBJ_TYPE_CHK_EN > 0u)
    if (p_grp->Type != OS_OBJ_TYPE_FLAG) {                      /* Validate that we are pointing at an event flag       */
        OS_TRACE_FLAG_PEND_FAILED(p_grp);
        OS_TRACE_FLAG_PEND_EXIT(OS_ERR_OBJ_TYPE);
       *p_err = OS_ERR_OBJ_TYPE;
        return (0u);
    }
#endif

    if ((opt & OS_OPT_PEND_FLAG_CONSUME) != 0u) {               /* See if we need to consume the flags                  */
        consume = OS_TRUE;
    } else {
        consume = OS_FALSE;
    }

    if (p_ts != (CPU_TS *)0) {
       *p_ts = 0u;                                              /* Initialize the returned timestamp                    */
    }

    mode = opt & OS_OPT_PEND_FLAG_MASK;
    CPU_CRITICAL_ENTER();
    switch (mode) {
        case OS_OPT_PEND_FLAG_SET_ALL:                          /* See if all required flags are set                    */
             flags_rdy = (p_grp->Flags & flags);                /* Extract only the bits we want                        */
             if (flags_rdy == flags) {                          /* Must match ALL the bits that we want                 */
                 if (consume == OS_TRUE) {                      /* See if we need to consume the flags                  */
                     p_grp->Flags &= ~flags_rdy;                /* Clear ONLY the flags that we wanted                  */
                 }
                 OSTCBCurPtr->FlagsRdy = flags_rdy;             /* Save flags that were ready                           */
#if (OS_CFG_TS_EN > 0u)
                 if (p_ts != (CPU_TS *)0) {
                    *p_ts = p_grp->TS;
                 }
#endif
                 CPU_CRITICAL_EXIT();                           /* Yes, condition met, return to caller                 */
                 OS_TRACE_FLAG_PEND(p_grp);
                 OS_TRACE_FLAG_PEND_EXIT(OS_ERR_NONE);
                *p_err = OS_ERR_NONE;
                 return (flags_rdy);
             } else {                                           /* Block task until events occur or timeout             */
                 if ((opt & OS_OPT_PEND_NON_BLOCKING) != 0u) {
                     CPU_CRITICAL_EXIT();
                     OS_TRACE_FLAG_PEND_FAILED(p_grp);
                     OS_TRACE_FLAG_PEND_EXIT(OS_ERR_PEND_WOULD_BLOCK);
                    *p_err = OS_ERR_PEND_WOULD_BLOCK;           /* Specified non-blocking so task would block           */
                     return ((OS_FLAGS)0);
                 } else {                                       /* Specified blocking so check is scheduler is locked   */
                     if (OSSchedLockNestingCtr > 0u) {          /* See if called with scheduler locked ...        */
                         CPU_CRITICAL_EXIT();
                         OS_TRACE_FLAG_PEND_FAILED(p_grp);
                         OS_TRACE_FLAG_PEND_EXIT(OS_ERR_SCHED_LOCKED);
                        *p_err = OS_ERR_SCHED_LOCKED;           /* ... can't PEND when locked                           */
                         return (0u);
                     }
                 }
                                                                /* Lock the scheduler/re-enable interrupts              */
                 OS_FlagBlock(p_grp,
                              flags,
                              opt,
                              timeout);
                 CPU_CRITICAL_EXIT();
             }
             break;

        case OS_OPT_PEND_FLAG_SET_ANY:
             flags_rdy = (p_grp->Flags & flags);                /* Extract only the bits we want                        */
             if (flags_rdy != 0u) {                             /* See if any flag set                                  */
                 if (consume == OS_TRUE) {                      /* See if we need to consume the flags                  */
                     p_grp->Flags &= ~flags_rdy;                /* Clear ONLY the flags that we got                     */
                 }
                 OSTCBCurPtr->FlagsRdy = flags_rdy;             /* Save flags that were ready                           */
#if (OS_CFG_TS_EN > 0u)
                 if (p_ts != (CPU_TS *)0) {
                    *p_ts  = p_grp->TS;
                 }
#endif
                 CPU_CRITICAL_EXIT();                           /* Yes, condition met, return to caller                 */
                 OS_TRACE_FLAG_PEND(p_grp);
                 OS_TRACE_FLAG_PEND_EXIT(OS_ERR_NONE);
                *p_err = OS_ERR_NONE;
                 return (flags_rdy);
             } else {                                           /* Block task until events occur or timeout             */
                 if ((opt & OS_OPT_PEND_NON_BLOCKING) != 0u) {
                     CPU_CRITICAL_EXIT();
                     OS_TRACE_FLAG_PEND_EXIT(OS_ERR_PEND_WOULD_BLOCK);
                    *p_err = OS_ERR_PEND_WOULD_BLOCK;           /* Specified non-blocking so task would block           */
                     return ((OS_FLAGS)0);
                 } else {                                       /* Specified blocking so check is scheduler is locked   */
                     if (OSSchedLockNestingCtr > 0u) {          /* See if called with scheduler locked ...        */
                         CPU_CRITICAL_EXIT();
                         OS_TRACE_FLAG_PEND_EXIT(OS_ERR_SCHED_LOCKED);
                        *p_err = OS_ERR_SCHED_LOCKED;           /* ... can't PEND when locked                           */
                         return ((OS_FLAGS)0);
                     }
                 }

                 OS_FlagBlock(p_grp,
                              flags,
                              opt,
                              timeout);
                 CPU_CRITICAL_EXIT();
             }
             break;

#if (OS_CFG_FLAG_MODE_CLR_EN > 0u)
        case OS_OPT_PEND_FLAG_CLR_ALL:                          /* See if all required flags are cleared                */
             flags_rdy = (OS_FLAGS)(~p_grp->Flags & flags);     /* Extract only the bits we want                        */
             if (flags_rdy == flags) {                          /* Must match ALL the bits that we want                 */
                 if (consume == OS_TRUE) {                      /* See if we need to consume the flags                  */
                     p_grp->Flags |= flags_rdy;                 /* Set ONLY the flags that we wanted                    */
                 }
                 OSTCBCurPtr->FlagsRdy = flags_rdy;             /* Save flags that were ready                           */
#if (OS_CFG_TS_EN > 0u)
                 if (p_ts != (CPU_TS *)0) {
                    *p_ts  = p_grp->TS;
                 }
#endif
                 CPU_CRITICAL_EXIT();                           /* Yes, condition met, return to caller                 */
                 OS_TRACE_FLAG_PEND(p_grp);
                 OS_TRACE_FLAG_PEND_EXIT(OS_ERR_NONE);
                *p_err = OS_ERR_NONE;
                 return (flags_rdy);
             } else {                                           /* Block task until events occur or timeout             */
                 if ((opt & OS_OPT_PEND_NON_BLOCKING) != 0u) {
                     CPU_CRITICAL_EXIT();
                     OS_TRACE_FLAG_PEND_EXIT(OS_ERR_PEND_WOULD_BLOCK);
                    *p_err = OS_ERR_PEND_WOULD_BLOCK;           /* Specified non-blocking so task would block           */
                     return ((OS_FLAGS)0);
                 } else {                                       /* Specified blocking so check is scheduler is locked   */
                     if (OSSchedLockNestingCtr > 0u) {          /* See if called with scheduler locked ...        */
                         CPU_CRITICAL_EXIT();
                         OS_TRACE_FLAG_PEND_EXIT(OS_ERR_SCHED_LOCKED);
                        *p_err = OS_ERR_SCHED_LOCKED;           /* ... can't PEND when locked                           */
                         return (0);
                     }
                 }

                 OS_FlagBlock(p_grp,
                              flags,
                              opt,
                              timeout);
                 CPU_CRITICAL_EXIT();
             }
             break;

        case OS_OPT_PEND_FLAG_CLR_ANY:
             flags_rdy = (~p_grp->Flags & flags);               /* Extract only the bits we want                        */
             if (flags_rdy != 0u) {                             /* See if any flag cleared                              */
                 if (consume == OS_TRUE) {                      /* See if we need to consume the flags                  */
                     p_grp->Flags |= flags_rdy;                 /* Set ONLY the flags that we got                       */
                 }
                 OSTCBCurPtr->FlagsRdy = flags_rdy;             /* Save flags that were ready                           */
#if (OS_CFG_TS_EN > 0u)
                 if (p_ts != (CPU_TS *)0) {
                    *p_ts  = p_grp->TS;
                 }
#endif
                 CPU_CRITICAL_EXIT();                           /* Yes, condition met, return to caller                 */
                 OS_TRACE_FLAG_PEND(p_grp);
                 OS_TRACE_FLAG_PEND_EXIT(OS_ERR_NONE);
                *p_err = OS_ERR_NONE;
                 return (flags_rdy);
             } else {                                           /* Block task until events occur or timeout             */
                 if ((opt & OS_OPT_PEND_NON_BLOCKING) != 0u) {
                     CPU_CRITICAL_EXIT();
                     OS_TRACE_FLAG_PEND_EXIT(OS_ERR_PEND_WOULD_BLOCK);
                    *p_err = OS_ERR_PEND_WOULD_BLOCK;           /* Specified non-blocking so task would block           */
                     return ((OS_FLAGS)0);
                 } else {                                       /* Specified blocking so check is scheduler is locked   */
                     if (OSSchedLockNestingCtr > 0u) {          /* See if called with scheduler locked ...        */
                         CPU_CRITICAL_EXIT();
                         OS_TRACE_FLAG_PEND_EXIT(OS_ERR_SCHED_LOCKED);
                        *p_err = OS_ERR_SCHED_LOCKED;           /* ... can't PEND when locked                           */
                         return (0u);
                     }
                 }

                 OS_FlagBlock(p_grp,
                              flags,
                              opt,
                              timeout);
                 CPU_CRITICAL_EXIT();
             }
             break;
#endif

        default:
             CPU_CRITICAL_EXIT();
             OS_TRACE_FLAG_PEND_FAILED(p_grp);
             OS_TRACE_FLAG_PEND_EXIT(OS_ERR_OPT_INVALID);
            *p_err = OS_ERR_OPT_INVALID;
             return (0u);
    }

    OS_TRACE_FLAG_PEND_BLOCK(p_grp);

    OSSched();                                                  /* Find next HPT ready to run                           */

    CPU_CRITICAL_ENTER();
    switch (OSTCBCurPtr->PendStatus) {
        case OS_STATUS_PEND_OK:                                 /* We got the event flags                               */
#if (OS_CFG_TS_EN > 0u)
             if (p_ts != (CPU_TS *)0) {
                *p_ts = OSTCBCurPtr->TS;
             }
#endif
             OS_TRACE_FLAG_PEND(p_grp);
            *p_err = OS_ERR_NONE;
             break;

        case OS_STATUS_PEND_ABORT:                              /* Indicate that we aborted                             */
#if (OS_CFG_TS_EN > 0u)
             if (p_ts != (CPU_TS *)0) {
                *p_ts = OSTCBCurPtr->TS;
             }
#endif
             CPU_CRITICAL_EXIT();
             OS_TRACE_FLAG_PEND_FAILED(p_grp);
            *p_err = OS_ERR_PEND_ABORT;
             break;

        case OS_STATUS_PEND_TIMEOUT:                            /* Indicate that we didn't get semaphore within timeout */
             if (p_ts != (CPU_TS *)0) {
                *p_ts = 0u;
             }
             CPU_CRITICAL_EXIT();
             OS_TRACE_FLAG_PEND_FAILED(p_grp);
            *p_err = OS_ERR_TIMEOUT;
             break;

        case OS_STATUS_PEND_DEL:                                /* Indicate that object pended on has been deleted      */
#if (OS_CFG_TS_EN > 0u)
             if (p_ts != (CPU_TS *)0) {
                *p_ts = OSTCBCurPtr->TS;
             }
#endif
             CPU_CRITICAL_EXIT();
             OS_TRACE_FLAG_PEND_FAILED(p_grp);
            *p_err = OS_ERR_OBJ_DEL;
             break;

        default:
             CPU_CRITICAL_EXIT();
             OS_TRACE_FLAG_PEND_FAILED(p_grp);
            *p_err = OS_ERR_STATUS_INVALID;
             break;
    }
    if (*p_err != OS_ERR_NONE) {
        OS_TRACE_FLAG_PEND_EXIT(*p_err);
        return (0u);
    }

    flags_rdy = OSTCBCurPtr->FlagsRdy;
    if (consume == OS_TRUE) {                                   /* See if we need to consume the flags                  */
        switch (mode) {
            case OS_OPT_PEND_FLAG_SET_ALL:
            case OS_OPT_PEND_FLAG_SET_ANY:                      /* Clear ONLY the flags we got                          */
                 p_grp->Flags &= ~flags_rdy;
                 break;

#if (OS_CFG_FLAG_MODE_CLR_EN > 0u)
            case OS_OPT_PEND_FLAG_CLR_ALL:
            case OS_OPT_PEND_FLAG_CLR_ANY:                      /* Set   ONLY the flags we got                          */
                 p_grp->Flags |=  flags_rdy;
                 break;
#endif
            default:
                 CPU_CRITICAL_EXIT();
                 OS_TRACE_FLAG_PEND_EXIT(OS_ERR_OPT_INVALID);
                *p_err = OS_ERR_OPT_INVALID;
                 return (0u);
        }
    }
    CPU_CRITICAL_EXIT();
    OS_TRACE_FLAG_PEND_EXIT(OS_ERR_NONE);
   *p_err = OS_ERR_NONE;                                        /* Event(s) must have occurred                          */
    return (flags_rdy);
}


/*
************************************************************************************************************************
*                                          ABORT WAITING ON AN EVENT FLAG GROUP
*
* Description: This function aborts & readies any tasks currently waiting on an event flag group.  This function should
*              be used to fault-abort the wait on the event flag group, rather than to normally post to the event flag
*              group OSFlagPost().
*
* Arguments  : p_grp     is a pointer to the event flag group
*
*              opt       determines the type of ABORT performed:
*
*                            OS_OPT_PEND_ABORT_1          ABORT wait for a single task (HPT) waiting on the event flag
*                            OS_OPT_PEND_ABORT_ALL        ABORT wait for ALL tasks that are  waiting on the event flag
*                            OS_OPT_POST_NO_SCHED         Do not call the scheduler
*
*              p_err     is a pointer to a variable that will contain an error code returned by this function.
*
*                            OS_ERR_NONE                  At least one task waiting on the event flag group and was
*                                                         readied and informed of the aborted wait; check return value
*                                                         for the number of tasks whose wait on the event flag group
*                                                         was aborted
*                            OS_ERR_OBJ_PTR_NULL          If 'p_grp' is a NULL pointer
*                            OS_ERR_OBJ_TYPE              If 'p_grp' is not pointing at an event flag group
*                            OS_ERR_OPT_INVALID           If you specified an invalid option
*                            OS_ERR_OS_NOT_RUNNING        If uC/OS-III is not running yet
*                            OS_ERR_PEND_ABORT_ISR        If you called this function from an ISR
*                            OS_ERR_PEND_ABORT_NONE       No task were pending
*
* Returns    : == 0          if no tasks were waiting on the event flag group, or upon error.
*              >  0          if one or more tasks waiting on the event flag group are now readied and informed.
*
* Note(s)    : none
************************************************************************************************************************
*/

#if (OS_CFG_FLAG_PEND_ABORT_EN > 0u)
OS_OBJ_QTY  OSFlagPendAbort (OS_FLAG_GRP  *p_grp,
                             OS_OPT        opt,
                             OS_ERR       *p_err)
{
    OS_PEND_LIST  *p_pend_list;
    OS_TCB        *p_tcb;
    CPU_TS         ts;
    OS_OBJ_QTY     nbr_tasks;
    CPU_SR_ALLOC();


#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return ((OS_OBJ_QTY)0u);
    }
#endif

#if (OS_CFG_CALLED_FROM_ISR_CHK_EN > 0u)
    if (OSIntNestingCtr > 0u) {                                 /* Not allowed to Pend Abort from an ISR                */
       *p_err = OS_ERR_PEND_ABORT_ISR;
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
    if (p_grp == (OS_FLAG_GRP *)0) {                            /* Validate 'p_grp'                                     */
       *p_err  =  OS_ERR_OBJ_PTR_NULL;
        return (0u);
    }
    switch (opt) {                                              /* Validate 'opt'                                       */
        case OS_OPT_PEND_ABORT_1:
        case OS_OPT_PEND_ABORT_ALL:
        case OS_OPT_PEND_ABORT_1   | OS_OPT_POST_NO_SCHED:
        case OS_OPT_PEND_ABORT_ALL | OS_OPT_POST_NO_SCHED:
             break;

        default:
            *p_err = OS_ERR_OPT_INVALID;
             return (0u);
    }
#endif

#if (OS_CFG_OBJ_TYPE_CHK_EN > 0u)
    if (p_grp->Type != OS_OBJ_TYPE_FLAG) {                      /* Make sure event flag group was created               */
       *p_err = OS_ERR_OBJ_TYPE;
        return (0u);
    }
#endif

    CPU_CRITICAL_ENTER();
    p_pend_list = &p_grp->PendList;
    if (p_pend_list->HeadPtr == (OS_TCB *)0) {                  /* Any task waiting on flag group?                      */
        CPU_CRITICAL_EXIT();                                    /* No                                                   */
       *p_err = OS_ERR_PEND_ABORT_NONE;
        return (0u);
    }

    nbr_tasks = 0u;
#if (OS_CFG_TS_EN > 0u)
    ts        = OS_TS_GET();                                    /* Get local time stamp so all tasks get the same time  */
#else
    ts        = 0u;
#endif

    while (p_pend_list->HeadPtr != (OS_TCB *)0) {
        p_tcb = p_pend_list->HeadPtr;
        OS_PendAbort(p_tcb,
                     ts,
                     OS_STATUS_PEND_ABORT);
        nbr_tasks++;
        if (opt != OS_OPT_PEND_ABORT_ALL) {                     /* Pend abort all tasks waiting?                        */
            break;                                              /* No                                                   */
        }
    }
    CPU_CRITICAL_EXIT();

    if ((opt & OS_OPT_POST_NO_SCHED) == 0u) {
        OSSched();                                              /* Run the scheduler                                    */
    }

   *p_err = OS_ERR_NONE;
    return (nbr_tasks);
}
#endif


/*
************************************************************************************************************************
*                                       GET FLAGS WHO CAUSED TASK TO BECOME READY
*
* Description: This function is called to obtain the flags that caused the task to become ready to run.
*              In other words, this function allows you to tell "Who done it!".
*
* Arguments  : p_err     is a pointer to an error code
*
*                            OS_ERR_NONE              If the call was successful
*                            OS_ERR_OS_NOT_RUNNING    If uC/OS-III is not running yet
*                            OS_ERR_PEND_ISR          If called from an ISR
*
* Returns    : The flags that caused the task to be ready.
*
* Note(s)    : none
************************************************************************************************************************
*/

OS_FLAGS  OSFlagPendGetFlagsRdy (OS_ERR  *p_err)
{
    OS_FLAGS  flags;
    CPU_SR_ALLOC();


#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return ((OS_FLAGS)0);
    }
#endif

#if (OS_CFG_INVALID_OS_CALLS_CHK_EN > 0u)
    if (OSRunning != OS_STATE_OS_RUNNING) {                     /* Is the kernel running?                               */
       *p_err = OS_ERR_OS_NOT_RUNNING;
        return (0u);
    }
#endif

#if (OS_CFG_CALLED_FROM_ISR_CHK_EN > 0u)
    if (OSIntNestingCtr > 0u) {                                 /* See if called from ISR ...                           */
       *p_err = OS_ERR_PEND_ISR;                                /* ... can't get from an ISR                            */
        return (0u);
    }
#endif

    CPU_CRITICAL_ENTER();
    flags = OSTCBCurPtr->FlagsRdy;
    CPU_CRITICAL_EXIT();
   *p_err = OS_ERR_NONE;
    return (flags);
}


/*
************************************************************************************************************************
*                                                POST EVENT FLAG BIT(S)
*
* Description: This function is called to set or clear some bits in an event flag group.  The bits to set or clear are
*              specified by a 'bit mask'.
*
* Arguments  : p_grp         is a pointer to the desired event flag group.
*
*              flags         If 'opt' (see below) is OS_OPT_POST_FLAG_SET, each bit that is set in 'flags' will
*                            set the corresponding bit in the event flag group.  e.g. to set bits 0, 4
*                            and 5 you would set 'flags' to:
*
*                                0x31     (note, bit 0 is least significant bit)
*
*                            If 'opt' (see below) is OS_OPT_POST_FLAG_CLR, each bit that is set in 'flags' will
*                            CLEAR the corresponding bit in the event flag group.  e.g. to clear bits 0,
*                            4 and 5 you would specify 'flags' as:
*
*                                0x31     (note, bit 0 is least significant bit)
*
*              opt           indicates whether the flags will be:
*
*                                OS_OPT_POST_FLAG_SET       set
*                                OS_OPT_POST_FLAG_CLR       cleared
*
*                            you can also 'add' OS_OPT_POST_NO_SCHED to prevent the scheduler from being called.
*
*              p_err         is a pointer to an error code and can be:
*
*                                OS_ERR_NONE                The call was successful
*                                OS_ERR_OBJ_PTR_NULL        You passed a NULL pointer
*                                OS_ERR_OBJ_TYPE            You are not pointing to an event flag group
*                                OS_ERR_OPT_INVALID         You specified an invalid option
*                                OS_ERR_OS_NOT_RUNNING      If uC/OS-III is not running yet
*
* Returns    : the new value of the event flags bits that are still set.
*
* Note(s)    : 1) The execution time of this function depends on the number of tasks waiting on the event flag group.
************************************************************************************************************************
*/

OS_FLAGS  OSFlagPost (OS_FLAG_GRP  *p_grp,
                      OS_FLAGS      flags,
                      OS_OPT        opt,
                      OS_ERR       *p_err)
{

    OS_FLAGS       flags_cur;
    OS_FLAGS       flags_rdy;
    OS_OPT         mode;
    OS_PEND_LIST  *p_pend_list;
    OS_TCB        *p_tcb;
    OS_TCB        *p_tcb_next;
    CPU_TS         ts;
    CPU_SR_ALLOC();


#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return (0u);
    }
#endif

    OS_TRACE_FLAG_POST_ENTER(p_grp, flags, opt);

#if (OS_CFG_INVALID_OS_CALLS_CHK_EN > 0u)
    if (OSRunning != OS_STATE_OS_RUNNING) {                     /* Is the kernel running?                               */
        OS_TRACE_FLAG_POST_EXIT(OS_ERR_OS_NOT_RUNNING);
       *p_err = OS_ERR_OS_NOT_RUNNING;
        return (0u);
    }
#endif

#if (OS_CFG_ARG_CHK_EN > 0u)
    if (p_grp == (OS_FLAG_GRP *)0) {                            /* Validate 'p_grp'                                     */
        OS_TRACE_FLAG_POST_FAILED(p_grp);
        OS_TRACE_FLAG_POST_EXIT(OS_ERR_OBJ_PTR_NULL);
       *p_err  = OS_ERR_OBJ_PTR_NULL;
        return (0u);
    }
#endif

#if (OS_CFG_OBJ_TYPE_CHK_EN > 0u)
    if (p_grp->Type != OS_OBJ_TYPE_FLAG) {                      /* Make sure we are pointing to an event flag grp       */
        OS_TRACE_FLAG_POST_FAILED(p_grp);
        OS_TRACE_FLAG_POST_EXIT(OS_ERR_OBJ_TYPE);
       *p_err = OS_ERR_OBJ_TYPE;
        return (0u);
    }
#endif

#if (OS_CFG_TS_EN > 0u)
    ts = OS_TS_GET();                                           /* Get timestamp                                        */
#else
    ts = 0u;
#endif

    OS_TRACE_FLAG_POST(p_grp);

    switch (opt) {
        case OS_OPT_POST_FLAG_SET:
        case OS_OPT_POST_FLAG_SET | OS_OPT_POST_NO_SCHED:
             CPU_CRITICAL_ENTER();
             p_grp->Flags |=  flags;                            /* Set   the flags specified in the group               */
             break;

        case OS_OPT_POST_FLAG_CLR:
        case OS_OPT_POST_FLAG_CLR | OS_OPT_POST_NO_SCHED:
             CPU_CRITICAL_ENTER();
             p_grp->Flags &= ~flags;                            /* Clear the flags specified in the group               */
             break;

        default:
            *p_err = OS_ERR_OPT_INVALID;                        /* INVALID option                                       */
             OS_TRACE_FLAG_POST_EXIT(*p_err);
             return (0u);
    }
#if (OS_CFG_TS_EN > 0u)
    p_grp->TS   = ts;
#endif
    p_pend_list = &p_grp->PendList;
    if (p_pend_list->HeadPtr == (OS_TCB *)0) {                  /* Any task waiting on event flag group?                */
        CPU_CRITICAL_EXIT();                                    /* No                                                   */
       *p_err = OS_ERR_NONE;
        OS_TRACE_FLAG_POST_EXIT(*p_err);
        return (p_grp->Flags);
    }

    p_tcb = p_pend_list->HeadPtr;
    while (p_tcb != (OS_TCB *)0) {                              /* Go through all tasks waiting on event flag(s)        */
        p_tcb_next = p_tcb->PendNextPtr;
        mode       = p_tcb->FlagsOpt & OS_OPT_PEND_FLAG_MASK;
        switch (mode) {
            case OS_OPT_PEND_FLAG_SET_ALL:                      /* See if all req. flags are set for current node       */
                 flags_rdy = (p_grp->Flags & p_tcb->FlagsPend);
                 if (flags_rdy == p_tcb->FlagsPend) {
                     OS_FlagTaskRdy(p_tcb,                      /* Make task RTR, event(s) Rx'd                         */
                                    flags_rdy,
                                    ts);
                 }
                 break;

            case OS_OPT_PEND_FLAG_SET_ANY:                      /* See if any flag set                                  */
                 flags_rdy = (p_grp->Flags & p_tcb->FlagsPend);
                 if (flags_rdy != 0u) {
                     OS_FlagTaskRdy(p_tcb,                      /* Make task RTR, event(s) Rx'd                         */
                                    flags_rdy,
                                    ts);
                 }
                 break;

#if (OS_CFG_FLAG_MODE_CLR_EN > 0u)
            case OS_OPT_PEND_FLAG_CLR_ALL:                      /* See if all req. flags are set for current node       */
                 flags_rdy = (OS_FLAGS)(~p_grp->Flags & p_tcb->FlagsPend);
                 if (flags_rdy == p_tcb->FlagsPend) {
                     OS_FlagTaskRdy(p_tcb,                      /* Make task RTR, event(s) Rx'd                         */
                                    flags_rdy,
                                    ts);
                 }
                 break;

            case OS_OPT_PEND_FLAG_CLR_ANY:                      /* See if any flag set                                  */
                 flags_rdy = (OS_FLAGS)(~p_grp->Flags & p_tcb->FlagsPend);
                 if (flags_rdy != 0u) {
                     OS_FlagTaskRdy(p_tcb,                      /* Make task RTR, event(s) Rx'd                         */
                                    flags_rdy,
                                    ts);
                 }
                 break;
#endif
            default:
                 CPU_CRITICAL_EXIT();
                *p_err = OS_ERR_FLAG_PEND_OPT;
                 OS_TRACE_FLAG_POST_EXIT(*p_err);
                 return (0u);
        }
                                                                /* Point to next task waiting for event flag(s)         */
        p_tcb = p_tcb_next;
    }
    CPU_CRITICAL_EXIT();

    if ((opt & OS_OPT_POST_NO_SCHED) == 0u) {
        OSSched();
    }

    CPU_CRITICAL_ENTER();
    flags_cur = p_grp->Flags;
    CPU_CRITICAL_EXIT();
   *p_err     = OS_ERR_NONE;

    OS_TRACE_FLAG_POST_EXIT(*p_err);
    return (flags_cur);
}


/*
************************************************************************************************************************
*                         SUSPEND TASK UNTIL EVENT FLAG(s) RECEIVED OR TIMEOUT OCCURS
*
* Description: This function is internal to uC/OS-III and is used to put a task to sleep until the desired
*              event flag bit(s) are set.
*
* Arguments  : p_grp         is a pointer to the desired event flag group.
*              -----
*
*              flags         Is a bit pattern indicating which bit(s) (i.e. flags) you wish to check.
*                            The bits you want are specified by setting the corresponding bits in
*                            'flags'.  e.g. if your application wants to wait for bits 0 and 1 then
*                            'flags' would contain 0x03.
*
*              opt           specifies whether you want ALL bits to be set/cleared or ANY of the bits
*                            to be set/cleared.
*                            You can specify the following argument:
*
*                                OS_OPT_PEND_FLAG_CLR_ALL   You will check ALL bits in 'mask' to be clear (0)
*                                OS_OPT_PEND_FLAG_CLR_ANY   You will check ANY bit  in 'mask' to be clear (0)
*                                OS_OPT_PEND_FLAG_SET_ALL   You will check ALL bits in 'mask' to be set   (1)
*                                OS_OPT_PEND_FLAG_SET_ANY   You will check ANY bit  in 'mask' to be set   (1)
*
*              timeout       is the desired amount of time that the task will wait for the event flag
*                            bit(s) to be set.
*
* Returns    : none
*
* Note(s)    : This function is INTERNAL to uC/OS-III and your application should not call it.
************************************************************************************************************************
*/

void  OS_FlagBlock (OS_FLAG_GRP  *p_grp,
                    OS_FLAGS      flags,
                    OS_OPT        opt,
                    OS_TICK       timeout)
{
    OSTCBCurPtr->FlagsPend = flags;                             /* Save the flags that we need to wait for              */
    OSTCBCurPtr->FlagsOpt  = opt;                               /* Save the type of wait we are doing                   */
    OSTCBCurPtr->FlagsRdy  = 0u;

    OS_Pend((OS_PEND_OBJ *)((void *)p_grp),
             OSTCBCurPtr,
             OS_TASK_PEND_ON_FLAG,
             timeout);
}


/*
************************************************************************************************************************
*                                      CLEAR THE CONTENTS OF AN EVENT FLAG GROUP
*
* Description: This function is called by OSFlagDel() to clear the contents of an event flag group
*

* Argument(s): p_grp     is a pointer to the event flag group to clear
*              -----
*
* Returns    : none
*
* Note(s)    : This function is INTERNAL to uC/OS-III and your application should not call it.
************************************************************************************************************************
*/

void  OS_FlagClr (OS_FLAG_GRP  *p_grp)
{
    OS_PEND_LIST  *p_pend_list;


#if (OS_OBJ_TYPE_REQ > 0u)
    p_grp->Type             = OS_OBJ_TYPE_NONE;
#endif
#if (OS_CFG_DBG_EN > 0u)
    p_grp->NamePtr          = (CPU_CHAR *)((void *)"?FLAG");    /* Unknown name                                         */
#endif
    p_grp->Flags            =  0u;
    p_pend_list             = &p_grp->PendList;
    OS_PendListInit(p_pend_list);
}


/*
************************************************************************************************************************
*                                    ADD/REMOVE EVENT FLAG GROUP TO/FROM DEBUG LIST
*
* Description: These functions are called by uC/OS-III to add or remove an event flag group from the event flag debug
*              list.
*
* Arguments  : p_grp     is a pointer to the event flag group to add/remove
*
* Returns    : none
*
* Note(s)    : These functions are INTERNAL to uC/OS-III and your application should not call it.
************************************************************************************************************************
*/

#if (OS_CFG_DBG_EN > 0u)
void  OS_FlagDbgListAdd (OS_FLAG_GRP  *p_grp)
{
    p_grp->DbgNamePtr                = (CPU_CHAR *)((void *)" ");
    p_grp->DbgPrevPtr                = (OS_FLAG_GRP *)0;
    if (OSFlagDbgListPtr == (OS_FLAG_GRP *)0) {
        p_grp->DbgNextPtr            = (OS_FLAG_GRP *)0;
    } else {
        p_grp->DbgNextPtr            = OSFlagDbgListPtr;
        OSFlagDbgListPtr->DbgPrevPtr = p_grp;
    }
    OSFlagDbgListPtr                 = p_grp;
}


void  OS_FlagDbgListRemove (OS_FLAG_GRP  *p_grp)
{
    OS_FLAG_GRP  *p_grp_next;
    OS_FLAG_GRP  *p_grp_prev;


    p_grp_prev = p_grp->DbgPrevPtr;
    p_grp_next = p_grp->DbgNextPtr;

    if (p_grp_prev == (OS_FLAG_GRP *)0) {
        OSFlagDbgListPtr = p_grp_next;
        if (p_grp_next != (OS_FLAG_GRP *)0) {
            p_grp_next->DbgPrevPtr = (OS_FLAG_GRP *)0;
        }
        p_grp->DbgNextPtr = (OS_FLAG_GRP *)0;

    } else if (p_grp_next == (OS_FLAG_GRP *)0) {
        p_grp_prev->DbgNextPtr = (OS_FLAG_GRP *)0;
        p_grp->DbgPrevPtr      = (OS_FLAG_GRP *)0;

    } else {
        p_grp_prev->DbgNextPtr =  p_grp_next;
        p_grp_next->DbgPrevPtr =  p_grp_prev;
        p_grp->DbgNextPtr      = (OS_FLAG_GRP *)0;
        p_grp->DbgPrevPtr      = (OS_FLAG_GRP *)0;
    }
}
#endif


/*
************************************************************************************************************************
*                                        MAKE TASK READY-TO-RUN, EVENT(s) OCCURRED
*
* Description: This function is internal to uC/OS-III and is used to make a task ready-to-run because the desired event
*              flag bits have been set.
*
* Arguments  : p_tcb         is a pointer to the OS_TCB of the task to remove
*              -----
*
*              flags_rdy     contains the bit pattern of the event flags that cause the task to become ready-to-run.
*
*              ts            is a timestamp associated with the post
*
* Returns    : none
*
* Note(s)    : This function is INTERNAL to uC/OS-III and your application should not call it.
************************************************************************************************************************
*/

void   OS_FlagTaskRdy (OS_TCB    *p_tcb,
                       OS_FLAGS   flags_rdy,
                       CPU_TS     ts)
{
#if (OS_CFG_TS_EN == 0u)
    (void)ts;                                                   /* Prevent compiler warning for not using 'ts'          */
#endif

    p_tcb->FlagsRdy   = flags_rdy;
    p_tcb->PendStatus = OS_STATUS_PEND_OK;                      /* Clear pend status                                    */
    p_tcb->PendOn     = OS_TASK_PEND_ON_NOTHING;                /* Indicate no longer pending                           */
#if (OS_CFG_TS_EN > 0u)
    p_tcb->TS         = ts;
#endif
    switch (p_tcb->TaskState) {
        case OS_TASK_STATE_PEND:
        case OS_TASK_STATE_PEND_TIMEOUT:
#if (OS_CFG_TICK_EN > 0u)
             if (p_tcb->TaskState == OS_TASK_STATE_PEND_TIMEOUT) {
                 OS_TickListRemove(p_tcb);                      /* Remove from tick list                                */
             }
#endif
             OS_RdyListInsert(p_tcb);                           /* Insert the task in the ready list                    */
             p_tcb->TaskState = OS_TASK_STATE_RDY;
             break;

        case OS_TASK_STATE_PEND_SUSPENDED:
        case OS_TASK_STATE_PEND_TIMEOUT_SUSPENDED:
             p_tcb->TaskState = OS_TASK_STATE_SUSPENDED;
             break;

        case OS_TASK_STATE_RDY:
        case OS_TASK_STATE_DLY:
        case OS_TASK_STATE_DLY_SUSPENDED:
        case OS_TASK_STATE_SUSPENDED:
        default:
                                                                /* Default case.                                        */
             break;
    }
    OS_PendListRemove(p_tcb);
}
#endif
