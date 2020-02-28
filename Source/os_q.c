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
*                                       MESSAGE QUEUE MANAGEMENT
*
* File    : os_q.c
* Version : V3.08.00
*********************************************************************************************************
*/

#define  MICRIUM_SOURCE
#include "os.h"

#ifdef VSC_INCLUDE_SOURCE_FILE_NAMES
const  CPU_CHAR  *os_q__c = "$Id: $";
#endif


#if (OS_CFG_Q_EN > 0u)
/*
************************************************************************************************************************
*                                               CREATE A MESSAGE QUEUE
*
* Description: This function is called by your application to create a message queue.  Message queues MUST be created
*              before they can be used.
*
* Arguments  : p_q         is a pointer to the message queue
*
*              p_name      is a pointer to an ASCII string that will be used to name the message queue
*
*              max_qty     indicates the maximum size of the message queue (must be non-zero).  Note that it's also not
*                          possible to have a size higher than the maximum number of OS_MSGs available.
*
*              p_err       is a pointer to a variable that will contain an error code returned by this function.
*
*                              OS_ERR_NONE                    The call was successful
*                              OS_ERR_CREATE_ISR              Can't create from an ISR
*                              OS_ERR_ILLEGAL_CREATE_RUN_TIME If you are trying to create the Queue after you called
*                                                               OSSafetyCriticalStart()
*                              OS_ERR_OBJ_PTR_NULL            If you passed a NULL pointer for 'p_q'
*                              OS_ERR_Q_SIZE                  If the size you specified is 0
*                              OS_ERR_OBJ_CREATED             If the message queue was already created
*
* Returns    : none
*
* Note(s)    : none
************************************************************************************************************************
*/

void  OSQCreate (OS_Q        *p_q,
                 CPU_CHAR    *p_name,
                 OS_MSG_QTY   max_qty,
                 OS_ERR      *p_err)

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
    if (OSIntNestingCtr > 0u) {                                 /* Not allowed to be called from an ISR                 */
       *p_err = OS_ERR_CREATE_ISR;
        return;
    }
#endif

#if (OS_CFG_ARG_CHK_EN > 0u)
    if (p_q == (OS_Q *)0) {                                     /* Validate arguments                                   */
       *p_err = OS_ERR_OBJ_PTR_NULL;
        return;
    }
    if (max_qty == 0u) {                                        /* Cannot specify a zero size queue                     */
       *p_err = OS_ERR_Q_SIZE;
        return;
    }
#endif

    CPU_CRITICAL_ENTER();
#if (OS_OBJ_TYPE_REQ > 0u)
    if (p_q->Type == OS_OBJ_TYPE_Q) {
        CPU_CRITICAL_EXIT();
        *p_err = OS_ERR_OBJ_CREATED;
        return;
    }
    p_q->Type    = OS_OBJ_TYPE_Q;                               /* Mark the data structure as a message queue           */
#endif
#if (OS_CFG_DBG_EN > 0u)
    p_q->NamePtr = p_name;
#else
    (void)p_name;
#endif
    OS_MsgQInit(&p_q->MsgQ,                                     /* Initialize the queue                                 */
                max_qty);
    OS_PendListInit(&p_q->PendList);                            /* Initialize the waiting list                          */

#if (OS_CFG_DBG_EN > 0u)
    OS_QDbgListAdd(p_q);
    OSQQty++;                                                   /* One more queue created                               */
#endif
    OS_TRACE_Q_CREATE(p_q, p_name);
    CPU_CRITICAL_EXIT();
   *p_err = OS_ERR_NONE;
}


/*
************************************************************************************************************************
*                                               DELETE A MESSAGE QUEUE
*
* Description: This function deletes a message queue and readies all tasks pending on the queue.
*
* Arguments  : p_q       is a pointer to the message queue you want to delete
*
*              opt       determines delete options as follows:
*
*                            OS_OPT_DEL_NO_PEND          Delete the queue ONLY if no task pending
*                            OS_OPT_DEL_ALWAYS           Deletes the queue even if tasks are waiting.
*                                                        In this case, all the tasks pending will be readied.
*
*              p_err     is a pointer to a variable that will contain an error code returned by this function.
*
*                            OS_ERR_NONE                    The call was successful and the queue was deleted
*                            OS_ERR_DEL_ISR                 If you tried to delete the queue from an ISR
*                            OS_ERR_ILLEGAL_DEL_RUN_TIME    If you are trying to delete the message queue after you
*                                                             called OSStart()
*                            OS_ERR_OBJ_PTR_NULL            If you pass a NULL pointer for 'p_q'
*                            OS_ERR_OBJ_TYPE                If the message queue was not created
*                            OS_ERR_OPT_INVALID             An invalid option was specified
*                            OS_ERR_OS_NOT_RUNNING          If uC/OS-III is not running yet
*                            OS_ERR_TASK_WAITING            One or more tasks were waiting on the queue
*
* Returns    : == 0          if no tasks were waiting on the queue, or upon error.
*              >  0          if one or more tasks waiting on the queue are now readied and informed.
*
* Note(s)    : 1) This function must be used with care.  Tasks that would normally expect the presence of the queue MUST
*                 check the return code of OSQPend().
*
*              2) Because ALL tasks pending on the queue will be readied, you MUST be careful in applications where the
*                 queue is used for mutual exclusion because the resource(s) will no longer be guarded by the queue.
************************************************************************************************************************
*/

#if (OS_CFG_Q_DEL_EN > 0u)
OS_OBJ_QTY  OSQDel (OS_Q    *p_q,
                    OS_OPT   opt,
                    OS_ERR  *p_err)
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

    OS_TRACE_Q_DEL_ENTER(p_q, opt);

#ifdef OS_SAFETY_CRITICAL_IEC61508
    if (OSSafetyCriticalStartFlag == OS_TRUE) {
        OS_TRACE_Q_DEL_EXIT(OS_ERR_ILLEGAL_DEL_RUN_TIME);
       *p_err = OS_ERR_ILLEGAL_DEL_RUN_TIME;
        return (0u);
    }
#endif

#if (OS_CFG_CALLED_FROM_ISR_CHK_EN > 0u)
    if (OSIntNestingCtr > 0u) {                                 /* Can't delete a message queue from an ISR             */
        OS_TRACE_Q_DEL_EXIT(OS_ERR_DEL_ISR);
       *p_err = OS_ERR_DEL_ISR;
        return (0u);
    }
#endif

#if (OS_CFG_INVALID_OS_CALLS_CHK_EN > 0u)
    if (OSRunning != OS_STATE_OS_RUNNING) {                     /* Is the kernel running?                               */
        OS_TRACE_Q_DEL_EXIT(OS_ERR_OS_NOT_RUNNING);
       *p_err = OS_ERR_OS_NOT_RUNNING;
        return (0u);
    }
#endif

#if (OS_CFG_ARG_CHK_EN > 0u)
    if (p_q == (OS_Q *)0) {                                     /* Validate 'p_q'                                       */
        OS_TRACE_Q_DEL_EXIT(OS_ERR_OBJ_PTR_NULL);
       *p_err =  OS_ERR_OBJ_PTR_NULL;
        return (0u);
    }
#endif

#if (OS_CFG_OBJ_TYPE_CHK_EN > 0u)
    if (p_q->Type != OS_OBJ_TYPE_Q) {                           /* Make sure message queue was created                  */
        OS_TRACE_Q_DEL_EXIT(OS_ERR_OBJ_TYPE);
       *p_err = OS_ERR_OBJ_TYPE;
        return (0u);
    }
#endif

    CPU_CRITICAL_ENTER();
    p_pend_list = &p_q->PendList;
    nbr_tasks   = 0u;
    switch (opt) {
        case OS_OPT_DEL_NO_PEND:                                /* Delete message queue only if no task waiting         */
             if (p_pend_list->HeadPtr == (OS_TCB *)0) {
#if (OS_CFG_DBG_EN > 0u)
                 OS_QDbgListRemove(p_q);
                 OSQQty--;
#endif
                 OS_TRACE_Q_DEL(p_q);
                 OS_QClr(p_q);
                 CPU_CRITICAL_EXIT();
                *p_err = OS_ERR_NONE;
             } else {
                 CPU_CRITICAL_EXIT();
                *p_err = OS_ERR_TASK_WAITING;
             }
             break;

        case OS_OPT_DEL_ALWAYS:                                 /* Always delete the message queue                      */
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
             OS_QDbgListRemove(p_q);
             OSQQty--;
#endif
             OS_TRACE_Q_DEL(p_q);
             OS_QClr(p_q);
             CPU_CRITICAL_EXIT();
             OSSched();                                         /* Find highest priority task ready to run              */
            *p_err = OS_ERR_NONE;
             break;

        default:
             CPU_CRITICAL_EXIT();
            *p_err = OS_ERR_OPT_INVALID;
             break;
    }
    OS_TRACE_Q_DEL_EXIT(*p_err);
    return (nbr_tasks);
}
#endif


/*
************************************************************************************************************************
*                                                     FLUSH QUEUE
*
* Description : This function is used to flush the contents of the message queue.
*
* Arguments   : p_q        is a pointer to the message queue to flush
*
*               p_err      is a pointer to a variable that will contain an error code returned by this function.
*
*                              OS_ERR_NONE              Upon success
*                              OS_ERR_FLUSH_ISR         If you called this function from an ISR
*                              OS_ERR_OBJ_PTR_NULL      If you passed a NULL pointer for 'p_q'
*                              OS_ERR_OBJ_TYPE          If you didn't create the message queue
*                              OS_ERR_OS_NOT_RUNNING    If uC/OS-III is not running yet
*
* Returns     : == 0       if no entries were freed, or upon error.
*               >  0       the number of freed entries.
*
* Note(s)     : 1) You should use this function with great care because, when to flush the queue, you LOOSE the
*                  references to what the queue entries are pointing to and thus, you could cause 'memory leaks'.  In
*                  other words, the data you are pointing to that's being referenced by the queue entries should, most
*                  likely, need to be de-allocated (i.e. freed).
************************************************************************************************************************
*/

#if (OS_CFG_Q_FLUSH_EN > 0u)
OS_MSG_QTY  OSQFlush (OS_Q    *p_q,
                      OS_ERR  *p_err)
{
    OS_MSG_QTY  entries;
    CPU_SR_ALLOC();



#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return (0u);
    }
#endif

#if (OS_CFG_CALLED_FROM_ISR_CHK_EN > 0u)
    if (OSIntNestingCtr > 0u) {                                 /* Can't flush a message queue from an ISR              */
       *p_err = OS_ERR_FLUSH_ISR;
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
    if (p_q == (OS_Q *)0) {                                     /* Validate arguments                                   */
       *p_err = OS_ERR_OBJ_PTR_NULL;
        return (0u);
    }
#endif

#if (OS_CFG_OBJ_TYPE_CHK_EN > 0u)
    if (p_q->Type != OS_OBJ_TYPE_Q) {                           /* Make sure message queue was created                  */
       *p_err = OS_ERR_OBJ_TYPE;
        return (0u);
    }
#endif

    CPU_CRITICAL_ENTER();
    entries = OS_MsgQFreeAll(&p_q->MsgQ);                       /* Return all OS_MSGs to the OS_MSG pool                */
    CPU_CRITICAL_EXIT();
   *p_err   = OS_ERR_NONE;
    return (entries);
}
#endif


/*
************************************************************************************************************************
*                                            PEND ON A QUEUE FOR A MESSAGE
*
* Description: This function waits for a message to be sent to a queue.
*
* Arguments  : p_q           is a pointer to the message queue
*
*              timeout       is an optional timeout period (in clock ticks).  If non-zero, your task will wait for a
*                            message to arrive at the queue up to the amount of time specified by this argument.  If you
*                            specify 0, however, your task will wait forever at the specified queue or, until a message
*                            arrives.
*
*              opt           determines whether the user wants to block if the queue is empty or not:
*
*                                OS_OPT_PEND_BLOCKING
*                                OS_OPT_PEND_NON_BLOCKING
*
*              p_msg_size    is a pointer to a variable that will receive the size of the message
*
*              p_ts          is a pointer to a variable that will receive the timestamp of when the message was
*                            received, pend aborted or the message queue deleted,  If you pass a NULL pointer (i.e.
*                            (CPU_TS *)0) then you will not get the timestamp.  In other words, passing a NULL pointer
*                            is valid and indicates that you don't need the timestamp.
*
*              p_err         is a pointer to a variable that will contain an error code returned by this function.
*
*                                OS_ERR_NONE               The call was successful and your task received a message
*                                OS_ERR_OBJ_DEL            If 'p_q' was deleted
*                                OS_ERR_OBJ_PTR_NULL       If you pass a NULL pointer for 'p_q'
*                                OS_ERR_OBJ_TYPE           If the message queue was not created
*                                OS_ERR_OPT_INVALID        You specified an invalid option
*                                OS_ERR_OS_NOT_RUNNING     If uC/OS-III is not running yet
*                                OS_ERR_PEND_ABORT         The pend was aborted
*                                OS_ERR_PEND_ISR           If you called this function from an ISR
*                                OS_ERR_PEND_WOULD_BLOCK   If you specified non-blocking but the queue was not empty
*                                OS_ERR_PTR_INVALID        If you passed a NULL pointer of 'p_msg_size'
*                                OS_ERR_SCHED_LOCKED       The scheduler is locked
*                                OS_ERR_STATUS_INVALID     If the pend status has an invalid value
*                                OS_ERR_TIMEOUT            A message was not received within the specified timeout
*                                                          would lead to a suspension.
*                                OS_ERR_TICK_DISABLED      If kernel ticks are disabled and a timeout is specified
*
* Returns    : != (void *)0  is a pointer to the message received
*              == (void *)0  if you received a NULL pointer message or,
*                            if no message was received or,
*                            if 'p_q' is a NULL pointer or,
*                            if you didn't pass a pointer to a queue.
*
* Note(s)    : This API 'MUST NOT' be called from a timer callback function.
************************************************************************************************************************
*/

void  *OSQPend (OS_Q         *p_q,
                OS_TICK       timeout,
                OS_OPT        opt,
                OS_MSG_SIZE  *p_msg_size,
                CPU_TS       *p_ts,
                OS_ERR       *p_err)
{
    void  *p_void;
    CPU_SR_ALLOC();


#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return ((void *)0);
    }
#endif

    OS_TRACE_Q_PEND_ENTER(p_q, timeout, opt, p_msg_size, p_ts);

#if (OS_CFG_TICK_EN == 0u)
    if (timeout != 0u) {
       *p_err = OS_ERR_TICK_DISABLED;
        OS_TRACE_Q_PEND_FAILED(p_q);
        OS_TRACE_Q_PEND_EXIT(OS_ERR_TICK_DISABLED);
        return ((void *)0);
    }
#endif

#if (OS_CFG_CALLED_FROM_ISR_CHK_EN > 0u)
    if (OSIntNestingCtr > 0u) {                                 /* Not allowed to call from an ISR                      */
        if ((opt & OS_OPT_PEND_NON_BLOCKING) != OS_OPT_PEND_NON_BLOCKING) {
            OS_TRACE_Q_PEND_FAILED(p_q);
            OS_TRACE_Q_PEND_EXIT(OS_ERR_PEND_ISR);
           *p_err = OS_ERR_PEND_ISR;
            return ((void *)0);
        }
    }
#endif

#if (OS_CFG_INVALID_OS_CALLS_CHK_EN > 0u)
    if (OSRunning != OS_STATE_OS_RUNNING) {                     /* Is the kernel running?                               */
        OS_TRACE_Q_PEND_EXIT(OS_ERR_OS_NOT_RUNNING);
       *p_err = OS_ERR_OS_NOT_RUNNING;
        return ((void *)0);
    }
#endif

#if (OS_CFG_ARG_CHK_EN > 0u)
    if (p_q == (OS_Q *)0) {                                     /* Validate arguments                                   */
        OS_TRACE_Q_PEND_FAILED(p_q);
        OS_TRACE_Q_PEND_EXIT(OS_ERR_OBJ_PTR_NULL);
       *p_err = OS_ERR_OBJ_PTR_NULL;
        return ((void *)0);
    }
    if (p_msg_size == (OS_MSG_SIZE *)0) {
        OS_TRACE_Q_PEND_FAILED(p_q);
        OS_TRACE_Q_PEND_EXIT(OS_ERR_PTR_INVALID);
       *p_err = OS_ERR_PTR_INVALID;
        return ((void *)0);
    }
    switch (opt) {
        case OS_OPT_PEND_BLOCKING:
        case OS_OPT_PEND_NON_BLOCKING:
             break;

        default:
             OS_TRACE_Q_PEND_FAILED(p_q);
             OS_TRACE_Q_PEND_EXIT(OS_ERR_OPT_INVALID);
            *p_err = OS_ERR_OPT_INVALID;
             return ((void *)0);
    }
#endif

#if (OS_CFG_OBJ_TYPE_CHK_EN > 0u)
    if (p_q->Type != OS_OBJ_TYPE_Q) {                           /* Make sure message queue was created                  */
        OS_TRACE_Q_PEND_FAILED(p_q);
        OS_TRACE_Q_PEND_EXIT(OS_ERR_OBJ_TYPE);
       *p_err = OS_ERR_OBJ_TYPE;
        return ((void *)0);
    }
#endif

    if (p_ts != (CPU_TS *)0) {
       *p_ts = 0u;                                              /* Initialize the returned timestamp                    */
    }

    CPU_CRITICAL_ENTER();
    p_void = OS_MsgQGet(&p_q->MsgQ,                             /* Any message waiting in the message queue?            */
                        p_msg_size,
                        p_ts,
                        p_err);
    if (*p_err == OS_ERR_NONE) {
        OS_TRACE_Q_PEND(p_q);
        CPU_CRITICAL_EXIT();
        OS_TRACE_Q_PEND_EXIT(OS_ERR_NONE);
        return (p_void);                                        /* Yes, Return message received                         */
    }

    if ((opt & OS_OPT_PEND_NON_BLOCKING) != 0u) {               /* Caller wants to block if not available?              */
        CPU_CRITICAL_EXIT();
        OS_TRACE_Q_PEND_FAILED(p_q);
        OS_TRACE_Q_PEND_EXIT(OS_ERR_PEND_WOULD_BLOCK);
       *p_err = OS_ERR_PEND_WOULD_BLOCK;                        /* No                                                   */
        return ((void *)0);
    } else {
        if (OSSchedLockNestingCtr > 0u) {                       /* Can't pend when the scheduler is locked              */
            CPU_CRITICAL_EXIT();
            OS_TRACE_Q_PEND_FAILED(p_q);
            OS_TRACE_Q_PEND_EXIT(OS_ERR_SCHED_LOCKED);
           *p_err = OS_ERR_SCHED_LOCKED;
            return ((void *)0);
        }
    }

    OS_Pend((OS_PEND_OBJ *)((void *)p_q),                       /* Block task pending on Message Queue                  */
            OSTCBCurPtr,
            OS_TASK_PEND_ON_Q,
            timeout);
    CPU_CRITICAL_EXIT();
    OS_TRACE_Q_PEND_BLOCK(p_q);
    OSSched();                                                  /* Find the next highest priority task ready to run     */

    CPU_CRITICAL_ENTER();
    switch (OSTCBCurPtr->PendStatus) {
        case OS_STATUS_PEND_OK:                                 /* Extract message from TCB (Put there by Post)         */
             p_void     = OSTCBCurPtr->MsgPtr;
            *p_msg_size = OSTCBCurPtr->MsgSize;
#if (OS_CFG_TS_EN > 0u)
             if (p_ts  != (CPU_TS *)0) {
                *p_ts  =  OSTCBCurPtr->TS;
             }
#endif
             OS_TRACE_Q_PEND(p_q);
            *p_err      = OS_ERR_NONE;
             break;

        case OS_STATUS_PEND_ABORT:                              /* Indicate that we aborted                             */
             p_void     = (void *)0;
            *p_msg_size =         0u;
#if (OS_CFG_TS_EN > 0u)
             if (p_ts  != (CPU_TS *)0) {
                *p_ts  =  OSTCBCurPtr->TS;
             }
#endif
             OS_TRACE_Q_PEND_FAILED(p_q);
            *p_err      = OS_ERR_PEND_ABORT;
             break;

        case OS_STATUS_PEND_TIMEOUT:                            /* Indicate that we didn't get event within TO          */
             p_void     = (void *)0;
            *p_msg_size =         0u;
             OS_TRACE_Q_PEND_FAILED(p_q);
            *p_err      = OS_ERR_TIMEOUT;
             break;

        case OS_STATUS_PEND_DEL:                                /* Indicate that object pended on has been deleted      */
             p_void     = (void *)0;
            *p_msg_size =         0u;
#if (OS_CFG_TS_EN > 0u)
             if (p_ts  != (CPU_TS *)0) {
                *p_ts  =  OSTCBCurPtr->TS;
             }
#endif
             OS_TRACE_Q_PEND_FAILED(p_q);
            *p_err      = OS_ERR_OBJ_DEL;
             break;

        default:
             p_void     = (void *)0;
            *p_msg_size =         0u;
             OS_TRACE_Q_PEND_FAILED(p_q);
            *p_err      = OS_ERR_STATUS_INVALID;
             break;
    }
    CPU_CRITICAL_EXIT();
    OS_TRACE_Q_PEND_EXIT(*p_err);
    return (p_void);
}


/*
************************************************************************************************************************
*                                             ABORT WAITING ON A MESSAGE QUEUE
*
* Description: This function aborts & readies any tasks currently waiting on a queue.  This function should be used to
*              fault-abort the wait on the queue, rather than to normally signal the queue via OSQPost().
*
* Arguments  : p_q       is a pointer to the message queue
*
*              opt       determines the type of ABORT performed:
*
*                            OS_OPT_PEND_ABORT_1          ABORT wait for a single task (HPT) waiting on the queue
*                            OS_OPT_PEND_ABORT_ALL        ABORT wait for ALL tasks that are  waiting on the queue
*                            OS_OPT_POST_NO_SCHED         Do not call the scheduler
*
*              p_err     is a pointer to a variable that will contain an error code returned by this function.
*
*                            OS_ERR_NONE                  At least one task waiting on the queue was readied and
*                                                         informed of the aborted wait; check return value for the
*                                                         number of tasks whose wait on the queue was aborted
*                            OS_ERR_OBJ_PTR_NULL          If you pass a NULL pointer for 'p_q'
*                            OS_ERR_OBJ_TYPE              If the message queue was not created
*                            OS_ERR_OPT_INVALID           You specified an invalid option
*                            OS_ERR_OS_NOT_RUNNING        If uC/OS-III is not running yet
*                            OS_ERR_PEND_ABORT_ISR        If this function was called from an ISR
*                            OS_ERR_PEND_ABORT_NONE       No task were pending
*
* Returns    : == 0      if no tasks were waiting on the queue, or upon error.
*              >  0      if one or more tasks waiting on the queue are now readied and informed.
*
* Note(s)    : none
************************************************************************************************************************
*/

#if (OS_CFG_Q_PEND_ABORT_EN > 0u)
OS_OBJ_QTY  OSQPendAbort (OS_Q    *p_q,
                          OS_OPT   opt,
                          OS_ERR  *p_err)
{
    OS_PEND_LIST  *p_pend_list;
    OS_TCB        *p_tcb;
    CPU_TS         ts;
    OS_OBJ_QTY     nbr_tasks;
    CPU_SR_ALLOC();


#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return (0u);
    }
#endif

#if (OS_CFG_CALLED_FROM_ISR_CHK_EN > 0u)
    if (OSIntNestingCtr > 0u) {                                 /* Not allowed to Pend Abort from an ISR                */
       *p_err =  OS_ERR_PEND_ABORT_ISR;
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
    if (p_q == (OS_Q *)0) {                                     /* Validate 'p_q'                                       */
       *p_err =  OS_ERR_OBJ_PTR_NULL;
        return (0u);
    }
    switch (opt) {                                              /* Validate 'opt'                                       */
        case OS_OPT_PEND_ABORT_1:
        case OS_OPT_PEND_ABORT_ALL:
        case OS_OPT_PEND_ABORT_1   | OS_OPT_POST_NO_SCHED:
        case OS_OPT_PEND_ABORT_ALL | OS_OPT_POST_NO_SCHED:
             break;

        default:
            *p_err =  OS_ERR_OPT_INVALID;
             return (0u);
    }
#endif

#if (OS_CFG_OBJ_TYPE_CHK_EN > 0u)
    if (p_q->Type != OS_OBJ_TYPE_Q) {                           /* Make sure queue was created                          */
       *p_err =  OS_ERR_OBJ_TYPE;
        return (0u);
    }
#endif

    CPU_CRITICAL_ENTER();
    p_pend_list = &p_q->PendList;
    if (p_pend_list->HeadPtr == (OS_TCB *)0) {                  /* Any task waiting on queue?                           */
        CPU_CRITICAL_EXIT();                                    /* No                                                   */
       *p_err =  OS_ERR_PEND_ABORT_NONE;
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
*                                               POST MESSAGE TO A QUEUE
*
* Description: This function sends a message to a queue.  With the 'opt' argument, you can specify whether the message
*              is broadcast to all waiting tasks and/or whether you post the message to the front of the queue (LIFO)
*              or normally (FIFO) at the end of the queue.
*
* Arguments  : p_q           is a pointer to a message queue that must have been created by OSQCreate().
*
*              p_void        is a pointer to the message to send.
*
*              msg_size      specifies the size of the message (in bytes)
*
*              opt           determines the type of POST performed:
*
*                                OS_OPT_POST_ALL          POST to ALL tasks that are waiting on the queue.  This option
*                                                         can be added to either OS_OPT_POST_FIFO or OS_OPT_POST_LIFO
*                                OS_OPT_POST_FIFO         POST message to end of queue (FIFO) and wake up a single
*                                                         waiting task.
*                                OS_OPT_POST_LIFO         POST message to the front of the queue (LIFO) and wake up
*                                                         a single waiting task.
*                                OS_OPT_POST_NO_SCHED     Do not call the scheduler
*
*                            Note(s): 1) OS_OPT_POST_NO_SCHED can be added (or OR'd) with one of the other options.
*                                     2) OS_OPT_POST_ALL      can be added (or OR'd) with one of the other options.
*                                     3) Possible combination of options are:
*
*                                        OS_OPT_POST_FIFO
*                                        OS_OPT_POST_LIFO
*                                        OS_OPT_POST_FIFO + OS_OPT_POST_ALL
*                                        OS_OPT_POST_LIFO + OS_OPT_POST_ALL
*                                        OS_OPT_POST_FIFO + OS_OPT_POST_NO_SCHED
*                                        OS_OPT_POST_LIFO + OS_OPT_POST_NO_SCHED
*                                        OS_OPT_POST_FIFO + OS_OPT_POST_ALL + OS_OPT_POST_NO_SCHED
*                                        OS_OPT_POST_LIFO + OS_OPT_POST_ALL + OS_OPT_POST_NO_SCHED
*
*              p_err         is a pointer to a variable that will contain an error code returned by this function.
*
*                                OS_ERR_NONE              The call was successful and the message was sent
*                                OS_ERR_MSG_POOL_EMPTY    If there are no more OS_MSGs to use to place the message into
*                                OS_ERR_OBJ_PTR_NULL      If 'p_q' is a NULL pointer
*                                OS_ERR_OBJ_TYPE          If the message queue was not initialized
*                                OS_ERR_OPT_INVALID       You specified an invalid option
*                                OS_ERR_OS_NOT_RUNNING    If uC/OS-III is not running yet
*                                OS_ERR_Q_MAX             If the queue is full
*
* Returns    : None
*
* Note(s)    : none
************************************************************************************************************************
*/

void  OSQPost (OS_Q         *p_q,
               void         *p_void,
               OS_MSG_SIZE   msg_size,
               OS_OPT        opt,
               OS_ERR       *p_err)
{
    OS_OPT         post_type;
    OS_PEND_LIST  *p_pend_list;
    OS_TCB        *p_tcb;
    OS_TCB        *p_tcb_next;
    CPU_TS         ts;
    CPU_SR_ALLOC();


#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return;
    }
#endif

    OS_TRACE_Q_POST_ENTER(p_q, p_void, msg_size, opt);

#if (OS_CFG_INVALID_OS_CALLS_CHK_EN > 0u)
    if (OSRunning != OS_STATE_OS_RUNNING) {                     /* Is the kernel running?                               */
        OS_TRACE_Q_POST_EXIT(OS_ERR_OS_NOT_RUNNING);
       *p_err = OS_ERR_OS_NOT_RUNNING;
        return;
    }
#endif

#if (OS_CFG_ARG_CHK_EN > 0u)
    if (p_q == (OS_Q *)0) {                                     /* Validate 'p_q'                                       */
        OS_TRACE_Q_POST_FAILED(p_q);
        OS_TRACE_Q_POST_EXIT(OS_ERR_OBJ_PTR_NULL);
       *p_err = OS_ERR_OBJ_PTR_NULL;
        return;
    }
    switch (opt) {                                              /* Validate 'opt'                                       */
        case OS_OPT_POST_FIFO:
        case OS_OPT_POST_LIFO:
        case OS_OPT_POST_FIFO | OS_OPT_POST_ALL:
        case OS_OPT_POST_LIFO | OS_OPT_POST_ALL:
        case OS_OPT_POST_FIFO | OS_OPT_POST_NO_SCHED:
        case OS_OPT_POST_LIFO | OS_OPT_POST_NO_SCHED:
        case OS_OPT_POST_FIFO | (OS_OPT)(OS_OPT_POST_ALL | OS_OPT_POST_NO_SCHED):
        case OS_OPT_POST_LIFO | (OS_OPT)(OS_OPT_POST_ALL | OS_OPT_POST_NO_SCHED):
             break;

        default:
             OS_TRACE_Q_POST_FAILED(p_q);
             OS_TRACE_Q_POST_EXIT(OS_ERR_OPT_INVALID);
            *p_err =  OS_ERR_OPT_INVALID;
             return;
    }
#endif

#if (OS_CFG_OBJ_TYPE_CHK_EN > 0u)
    if (p_q->Type != OS_OBJ_TYPE_Q) {                           /* Make sure message queue was created                  */
        OS_TRACE_Q_POST_FAILED(p_q);
        OS_TRACE_Q_POST_EXIT(OS_ERR_OBJ_TYPE);
       *p_err = OS_ERR_OBJ_TYPE;
        return;
    }
#endif
#if (OS_CFG_TS_EN > 0u)
    ts = OS_TS_GET();                                           /* Get timestamp                                        */
#else
    ts = 0u;
#endif

    OS_TRACE_Q_POST(p_q);

    CPU_CRITICAL_ENTER();
    p_pend_list = &p_q->PendList;
    if (p_pend_list->HeadPtr == (OS_TCB *)0) {                  /* Any task waiting on message queue?                   */
        if ((opt & OS_OPT_POST_LIFO) == 0u) {                   /* Determine whether we post FIFO or LIFO               */
            post_type = OS_OPT_POST_FIFO;
        } else {
            post_type = OS_OPT_POST_LIFO;
        }
        OS_MsgQPut(&p_q->MsgQ,                                  /* Place message in the message queue                   */
                   p_void,
                   msg_size,
                   post_type,
                   ts,
                   p_err);
        CPU_CRITICAL_EXIT();
        OS_TRACE_Q_POST_EXIT(*p_err);
        return;
    }

    p_tcb = p_pend_list->HeadPtr;
    while (p_tcb != (OS_TCB *)0) {
        p_tcb_next = p_tcb->PendNextPtr;
        OS_Post((OS_PEND_OBJ *)((void *)p_q),
                p_tcb,
                p_void,
                msg_size,
                ts);
        if ((opt & OS_OPT_POST_ALL) == 0u)  {                   /* Post message to all tasks waiting?                   */
            break;                                              /* No                                                   */
        }
        p_tcb = p_tcb_next;
    }

    CPU_CRITICAL_EXIT();

    if ((opt & OS_OPT_POST_NO_SCHED) == 0u) {
        OSSched();                                              /* Run the scheduler                                    */
    }

   *p_err = OS_ERR_NONE;
    OS_TRACE_Q_POST_EXIT(*p_err);
}


/*
************************************************************************************************************************
*                                        CLEAR THE CONTENTS OF A MESSAGE QUEUE
*
* Description: This function is called by OSQDel() to clear the contents of a message queue
*

* Argument(s): p_q      is a pointer to the queue to clear
*              ---
*
* Returns    : none
*
* Note(s)    : 1) This function is INTERNAL to uC/OS-III and your application MUST NOT call it.
************************************************************************************************************************
*/

void  OS_QClr (OS_Q  *p_q)
{
    (void)OS_MsgQFreeAll(&p_q->MsgQ);                           /* Return all OS_MSGs to the free list                  */
#if (OS_OBJ_TYPE_REQ > 0u)
    p_q->Type    =  OS_OBJ_TYPE_NONE;                           /* Mark the data structure as a NONE                    */
#endif
#if (OS_CFG_DBG_EN > 0u)
    p_q->NamePtr = (CPU_CHAR *)((void *)"?Q");
#endif
    OS_MsgQInit(&p_q->MsgQ,                                     /* Initialize the list of OS_MSGs                       */
                0u);
    OS_PendListInit(&p_q->PendList);                            /* Initialize the waiting list                          */
}


/*
************************************************************************************************************************
*                                      ADD/REMOVE MESSAGE QUEUE TO/FROM DEBUG LIST
*
* Description: These functions are called by uC/OS-III to add or remove a message queue to/from a message queue debug
*              list.
*
* Arguments  : p_q     is a pointer to the message queue to add/remove
*
* Returns    : none
*
* Note(s)    : These functions are INTERNAL to uC/OS-III and your application should not call it.
************************************************************************************************************************
*/

#if (OS_CFG_DBG_EN > 0u)
void  OS_QDbgListAdd (OS_Q  *p_q)
{
    p_q->DbgNamePtr               = (CPU_CHAR *)((void *)" ");
    p_q->DbgPrevPtr               = (OS_Q *)0;
    if (OSQDbgListPtr == (OS_Q *)0) {
        p_q->DbgNextPtr           = (OS_Q *)0;
    } else {
        p_q->DbgNextPtr           =  OSQDbgListPtr;
        OSQDbgListPtr->DbgPrevPtr =  p_q;
    }
    OSQDbgListPtr                 =  p_q;
}


void  OS_QDbgListRemove (OS_Q  *p_q)
{
    OS_Q  *p_q_next;
    OS_Q  *p_q_prev;


    p_q_prev = p_q->DbgPrevPtr;
    p_q_next = p_q->DbgNextPtr;

    if (p_q_prev == (OS_Q *)0) {
        OSQDbgListPtr = p_q_next;
        if (p_q_next != (OS_Q *)0) {
            p_q_next->DbgPrevPtr = (OS_Q *)0;
        }
        p_q->DbgNextPtr = (OS_Q *)0;

    } else if (p_q_next == (OS_Q *)0) {
        p_q_prev->DbgNextPtr = (OS_Q *)0;
        p_q->DbgPrevPtr      = (OS_Q *)0;

    } else {
        p_q_prev->DbgNextPtr =  p_q_next;
        p_q_next->DbgPrevPtr =  p_q_prev;
        p_q->DbgNextPtr      = (OS_Q *)0;
        p_q->DbgPrevPtr      = (OS_Q *)0;
    }
}
#endif
#endif
