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
*                                           MUTEX MANAGEMENT
*
* File    : os_mutex.c
* Version : V3.08.00
*********************************************************************************************************
*/

#define  MICRIUM_SOURCE
#include "os.h"

#ifdef VSC_INCLUDE_SOURCE_FILE_NAMES
const  CPU_CHAR  *os_mutex__c = "$Id: $";
#endif


#if (OS_CFG_MUTEX_EN > 0u)
/*
************************************************************************************************************************
*                                                   CREATE A MUTEX
*
* Description: This function creates a mutex.
*
* Arguments  : p_mutex       is a pointer to the mutex to initialize.  Your application is responsible for allocating
*                            storage for the mutex.
*
*              p_name        is a pointer to the name you would like to give the mutex.
*
*              p_err         is a pointer to a variable that will contain an error code returned by this function.
*
*                                OS_ERR_NONE                    If the call was successful
*                                OS_ERR_CREATE_ISR              If you called this function from an ISR
*                                OS_ERR_ILLEGAL_CREATE_RUN_TIME If you are trying to create the mutex after you called
*                                                                 OSSafetyCriticalStart()
*                                OS_ERR_OBJ_PTR_NULL            If 'p_mutex' is a NULL pointer
*                                OS_ERR_OBJ_CREATED             If the mutex was already created
*
* Returns    : none
*
* Note(s)    : none
************************************************************************************************************************
*/

void  OSMutexCreate (OS_MUTEX  *p_mutex,
                     CPU_CHAR  *p_name,
                     OS_ERR    *p_err)
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
    if (p_mutex == (OS_MUTEX *)0) {                             /* Validate 'p_mutex'                                   */
       *p_err = OS_ERR_OBJ_PTR_NULL;
        return;
    }
#endif

    CPU_CRITICAL_ENTER();
#if (OS_OBJ_TYPE_REQ > 0u)
    if (p_mutex->Type == OS_OBJ_TYPE_MUTEX) {
        CPU_CRITICAL_EXIT();
        *p_err = OS_ERR_OBJ_CREATED;
        return;
    }
    p_mutex->Type              =  OS_OBJ_TYPE_MUTEX;            /* Mark the data structure as a mutex                   */
#endif
#if (OS_CFG_DBG_EN > 0u)
    p_mutex->NamePtr           =  p_name;
#else
    (void)p_name;
#endif
    p_mutex->MutexGrpNextPtr   = (OS_MUTEX *)0;
    p_mutex->OwnerTCBPtr       = (OS_TCB   *)0;
    p_mutex->OwnerNestingCtr   =             0u;                /* Mutex is available                                   */
#if (OS_CFG_TS_EN > 0u)
    p_mutex->TS                =             0u;
#endif
    OS_PendListInit(&p_mutex->PendList);                        /* Initialize the waiting list                          */

#if (OS_CFG_DBG_EN > 0u)
    OS_MutexDbgListAdd(p_mutex);
    OSMutexQty++;
#endif

    OS_TRACE_MUTEX_CREATE(p_mutex, p_name);
    CPU_CRITICAL_EXIT();
   *p_err = OS_ERR_NONE;
}


/*
************************************************************************************************************************
*                                                   DELETE A MUTEX
*
* Description: This function deletes a mutex and readies all tasks pending on the mutex.
*
* Arguments  : p_mutex       is a pointer to the mutex to delete
*
*              opt           determines delete options as follows:
*
*                                OS_OPT_DEL_NO_PEND          Delete mutex ONLY if no task pending
*                                OS_OPT_DEL_ALWAYS           Deletes the mutex even if tasks are waiting.
*                                                            In this case, all the tasks pending will be readied.
*
*              p_err         is a pointer to a variable that will contain an error code returned by this function.
*
*                                OS_ERR_NONE                    The call was successful and the mutex was deleted
*                                OS_ERR_DEL_ISR                 If you attempted to delete the mutex from an ISR
*                                OS_ERR_ILLEGAL_DEL_RUN_TIME    If you are trying to delete the mutex after you called
*                                                                 OSStart()
*                                OS_ERR_OBJ_PTR_NULL            If 'p_mutex' is a NULL pointer
*                                OS_ERR_OBJ_TYPE                If 'p_mutex' is not pointing to a mutex
*                                OS_ERR_OPT_INVALID             An invalid option was specified
*                                OS_ERR_OS_NOT_RUNNING          If uC/OS-III is not running yet
*                                OS_ERR_TASK_WAITING            One or more tasks were waiting on the mutex
*
* Returns    : == 0          if no tasks were waiting on the mutex, or upon error.
*              >  0          if one or more tasks waiting on the mutex are now readied and informed.
*
* Note(s)    : 1) This function must be used with care.  Tasks that would normally expect the presence of the mutex MUST
*                 check the return code of OSMutexPend().
*
*              2) Because ALL tasks pending on the mutex will be readied, you MUST be careful in applications where the
*                 mutex is used for mutual exclusion because the resource(s) will no longer be guarded by the mutex.
************************************************************************************************************************
*/

#if (OS_CFG_MUTEX_DEL_EN > 0u)
OS_OBJ_QTY  OSMutexDel (OS_MUTEX  *p_mutex,
                        OS_OPT     opt,
                        OS_ERR    *p_err)
{
    OS_OBJ_QTY     nbr_tasks;
    OS_PEND_LIST  *p_pend_list;
    OS_TCB        *p_tcb;
    OS_TCB        *p_tcb_owner;
    CPU_TS         ts;
#if (OS_CFG_MUTEX_EN > 0u)
    OS_PRIO        prio_new;
#endif
    CPU_SR_ALLOC();


#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return (0u);
    }
#endif

    OS_TRACE_MUTEX_DEL_ENTER(p_mutex, opt);

#ifdef OS_SAFETY_CRITICAL_IEC61508
    if (OSSafetyCriticalStartFlag == OS_TRUE) {
        OS_TRACE_MUTEX_DEL_EXIT(OS_ERR_ILLEGAL_DEL_RUN_TIME);
       *p_err = OS_ERR_ILLEGAL_DEL_RUN_TIME;
        return (0u);
    }
#endif

#if (OS_CFG_CALLED_FROM_ISR_CHK_EN > 0u)
    if (OSIntNestingCtr > 0u) {                                 /* Not allowed to delete a mutex from an ISR            */
        OS_TRACE_MUTEX_DEL_EXIT(OS_ERR_DEL_ISR);
       *p_err = OS_ERR_DEL_ISR;
        return (0u);
    }
#endif

#if (OS_CFG_INVALID_OS_CALLS_CHK_EN > 0u)
    if (OSRunning != OS_STATE_OS_RUNNING) {                     /* Is the kernel running?                               */
        OS_TRACE_MUTEX_DEL_EXIT(OS_ERR_OS_NOT_RUNNING);
       *p_err = OS_ERR_OS_NOT_RUNNING;
        return (0u);
    }
#endif

#if (OS_CFG_ARG_CHK_EN > 0u)
    if (p_mutex == (OS_MUTEX *)0) {                             /* Validate 'p_mutex'                                   */
        OS_TRACE_MUTEX_DEL_EXIT(OS_ERR_OBJ_PTR_NULL);
       *p_err = OS_ERR_OBJ_PTR_NULL;
        return (0u);
    }
#endif

#if (OS_CFG_OBJ_TYPE_CHK_EN > 0u)
    if (p_mutex->Type != OS_OBJ_TYPE_MUTEX) {                   /* Make sure mutex was created                          */
        OS_TRACE_MUTEX_DEL_EXIT(OS_ERR_OBJ_TYPE);
       *p_err = OS_ERR_OBJ_TYPE;
        return (0u);
    }
#endif

    CPU_CRITICAL_ENTER();
    p_pend_list = &p_mutex->PendList;
    nbr_tasks   = 0u;
    switch (opt) {
        case OS_OPT_DEL_NO_PEND:                                /* Delete mutex only if no task waiting                 */
             if (p_pend_list->HeadPtr == (OS_TCB *)0) {
#if (OS_CFG_DBG_EN > 0u)
                 OS_MutexDbgListRemove(p_mutex);
                 OSMutexQty--;
#endif
                 OS_TRACE_MUTEX_DEL(p_mutex);
                 if (p_mutex->OwnerTCBPtr != (OS_TCB *)0) {     /* Does the mutex belong to a task?                     */
                     OS_MutexGrpRemove(p_mutex->OwnerTCBPtr, p_mutex); /* yes, remove it from the task group.           */
                 }
                 OS_MutexClr(p_mutex);
                 CPU_CRITICAL_EXIT();
                *p_err = OS_ERR_NONE;
             } else {
                 CPU_CRITICAL_EXIT();
                *p_err = OS_ERR_TASK_WAITING;
             }
             break;

        case OS_OPT_DEL_ALWAYS:                                 /* Always delete the mutex                              */
#if (OS_CFG_TS_EN > 0u)
             ts = OS_TS_GET();                                  /* Get timestamp                                        */
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
             OS_MutexDbgListRemove(p_mutex);
             OSMutexQty--;
#endif
             OS_TRACE_MUTEX_DEL(p_mutex);
             p_tcb_owner = p_mutex->OwnerTCBPtr;
             if (p_tcb_owner != (OS_TCB *)0) {                  /* Does the mutex belong to a task?                     */
                 OS_MutexGrpRemove(p_tcb_owner, p_mutex);       /* yes, remove it from the task group.                  */
             }


             if (p_tcb_owner != (OS_TCB *)0) {                  /* Did we had to change the prio of owner?              */
                 if (p_tcb_owner->Prio != p_tcb_owner->BasePrio) {
                     prio_new = OS_MutexGrpPrioFindHighest(p_tcb_owner);
                     prio_new = (prio_new > p_tcb_owner->BasePrio) ? p_tcb_owner->BasePrio : prio_new;
                     OS_TaskChangePrio(p_tcb_owner, prio_new);
                     OS_TRACE_MUTEX_TASK_PRIO_DISINHERIT(p_tcb_owner, p_tcb_owner->Prio);
                 }
             }

             OS_MutexClr(p_mutex);
             CPU_CRITICAL_EXIT();
             OSSched();                                         /* Find highest priority task ready to run              */
            *p_err = OS_ERR_NONE;
             break;

        default:
             CPU_CRITICAL_EXIT();
            *p_err = OS_ERR_OPT_INVALID;
             break;
    }
    OS_TRACE_MUTEX_DEL_EXIT(*p_err);
    return (nbr_tasks);
}
#endif


/*
************************************************************************************************************************
*                                                    PEND ON MUTEX
*
* Description: This function waits for a mutex.
*
* Arguments  : p_mutex       is a pointer to the mutex
*
*              timeout       is an optional timeout period (in clock ticks).  If non-zero, your task will wait for the
*                            resource up to the amount of time (in 'ticks') specified by this argument.  If you specify
*                            0, however, your task will wait forever at the specified mutex or, until the resource
*                            becomes available.
*
*              opt           determines whether the user wants to block if the mutex is available or not:
*
*                                OS_OPT_PEND_BLOCKING
*                                OS_OPT_PEND_NON_BLOCKING
*
*              p_ts          is a pointer to a variable that will receive the timestamp of when the mutex was posted or
*                            pend aborted or the mutex deleted.  If you pass a NULL pointer (i.e. (CPU_TS *)0) then you
*                            will not get the timestamp.  In other words, passing a NULL pointer is valid and indicates
*                            that you don't need the timestamp.
*
*              p_err         is a pointer to a variable that will contain an error code returned by this function.
*
*                                OS_ERR_NONE               The call was successful and your task owns the resource
*                                OS_ERR_MUTEX_OWNER        If calling task already owns the mutex
*                                OS_ERR_MUTEX_OVF          Mutex nesting counter overflowed
*                                OS_ERR_OBJ_DEL            If 'p_mutex' was deleted
*                                OS_ERR_OBJ_PTR_NULL       If 'p_mutex' is a NULL pointer
*                                OS_ERR_OBJ_TYPE           If 'p_mutex' is not pointing at a mutex
*                                OS_ERR_OPT_INVALID        If you didn't specify a valid option
*                                OS_ERR_OS_NOT_RUNNING     If uC/OS-III is not running yet
*                                OS_ERR_PEND_ABORT         If the pend was aborted by another task
*                                OS_ERR_PEND_ISR           If you called this function from an ISR and the result
*                                                          would lead to a suspension
*                                OS_ERR_PEND_WOULD_BLOCK   If you specified non-blocking but the mutex was not
*                                                          available
*                                OS_ERR_SCHED_LOCKED       If you called this function when the scheduler is locked
*                                OS_ERR_STATUS_INVALID     If the pend status has an invalid value
*                                OS_ERR_TIMEOUT            The mutex was not received within the specified timeout
*                                OS_ERR_TICK_DISABLED      If kernel ticks are disabled and a timeout is specified
*
* Returns    : none
*
* Note(s)    : This API 'MUST NOT' be called from a timer callback function.
************************************************************************************************************************
*/

void  OSMutexPend (OS_MUTEX  *p_mutex,
                   OS_TICK    timeout,
                   OS_OPT     opt,
                   CPU_TS    *p_ts,
                   OS_ERR    *p_err)
{
    OS_TCB  *p_tcb;
    CPU_SR_ALLOC();


#if (OS_CFG_TS_EN == 0u)
    (void)p_ts;                                                 /* Prevent compiler warning for not using 'ts'          */
#endif

#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return;
    }
#endif

    OS_TRACE_MUTEX_PEND_ENTER(p_mutex, timeout, opt, p_ts);

#if (OS_CFG_TICK_EN == 0u)
    if (timeout != 0u) {
       *p_err = OS_ERR_TICK_DISABLED;
        OS_TRACE_MUTEX_PEND_FAILED(p_mutex);
        OS_TRACE_MUTEX_PEND_EXIT(OS_ERR_TICK_DISABLED);
        return;
    }
#endif

#if (OS_CFG_CALLED_FROM_ISR_CHK_EN > 0u)
    if (OSIntNestingCtr > 0u) {                                 /* Not allowed to call from an ISR                      */
        OS_TRACE_MUTEX_PEND_FAILED(p_mutex);
        OS_TRACE_MUTEX_PEND_EXIT(OS_ERR_PEND_ISR);
       *p_err = OS_ERR_PEND_ISR;
        return;
    }
#endif

#if (OS_CFG_INVALID_OS_CALLS_CHK_EN > 0u)
    if (OSRunning != OS_STATE_OS_RUNNING) {                     /* Is the kernel running?                               */
        OS_TRACE_MUTEX_PEND_EXIT(OS_ERR_OS_NOT_RUNNING);
       *p_err = OS_ERR_OS_NOT_RUNNING;
        return;
    }
#endif

#if (OS_CFG_ARG_CHK_EN > 0u)
    if (p_mutex == (OS_MUTEX *)0) {                             /* Validate arguments                                   */
        OS_TRACE_MUTEX_PEND_FAILED(p_mutex);
        OS_TRACE_MUTEX_PEND_EXIT(OS_ERR_OBJ_PTR_NULL);
       *p_err = OS_ERR_OBJ_PTR_NULL;
        return;
    }
    switch (opt) {                                              /* Validate 'opt'                                       */
        case OS_OPT_PEND_BLOCKING:
        case OS_OPT_PEND_NON_BLOCKING:
             break;

        default:
             OS_TRACE_MUTEX_PEND_FAILED(p_mutex);
             OS_TRACE_MUTEX_PEND_EXIT(OS_ERR_OPT_INVALID);
            *p_err = OS_ERR_OPT_INVALID;
             return;
    }
#endif

#if (OS_CFG_OBJ_TYPE_CHK_EN > 0u)
    if (p_mutex->Type != OS_OBJ_TYPE_MUTEX) {                   /* Make sure mutex was created                          */
        OS_TRACE_MUTEX_PEND_FAILED(p_mutex);
        OS_TRACE_MUTEX_PEND_EXIT(OS_ERR_OBJ_TYPE);
       *p_err = OS_ERR_OBJ_TYPE;
        return;
    }
#endif

    CPU_CRITICAL_ENTER();
    if (p_mutex->OwnerNestingCtr == 0u) {                       /* Resource available?                                  */
        p_mutex->OwnerTCBPtr     = OSTCBCurPtr;                 /* Yes, caller may proceed                              */
        p_mutex->OwnerNestingCtr = 1u;
#if (OS_CFG_TS_EN > 0u)
        if (p_ts != (CPU_TS *)0) {
           *p_ts = p_mutex->TS;
        }
#endif
        OS_MutexGrpAdd(OSTCBCurPtr, p_mutex);                   /* Add mutex to owner's group                           */
        CPU_CRITICAL_EXIT();
        OS_TRACE_MUTEX_PEND(p_mutex);
        OS_TRACE_MUTEX_PEND_EXIT(OS_ERR_NONE);
       *p_err = OS_ERR_NONE;
        return;
    }

    if (OSTCBCurPtr == p_mutex->OwnerTCBPtr) {                  /* See if current task is already the owner of the mutex*/
        if (p_mutex->OwnerNestingCtr == (OS_NESTING_CTR)-1) {
            CPU_CRITICAL_EXIT();
            OS_TRACE_MUTEX_PEND_FAILED(p_mutex);
            OS_TRACE_MUTEX_PEND_EXIT(OS_ERR_MUTEX_OVF);
           *p_err = OS_ERR_MUTEX_OVF;
            return;
        }
        p_mutex->OwnerNestingCtr++;
#if (OS_CFG_TS_EN > 0u)
        if (p_ts != (CPU_TS *)0) {
           *p_ts = p_mutex->TS;
        }
#endif
        CPU_CRITICAL_EXIT();
        OS_TRACE_MUTEX_PEND_FAILED(p_mutex);
        OS_TRACE_MUTEX_PEND_EXIT(OS_ERR_MUTEX_OWNER);
       *p_err = OS_ERR_MUTEX_OWNER;                             /* Indicate that current task already owns the mutex    */
        return;
    }

    if ((opt & OS_OPT_PEND_NON_BLOCKING) != 0u) {               /* Caller wants to block if not available?              */
        CPU_CRITICAL_EXIT();
#if (OS_CFG_TS_EN > 0u)
        if (p_ts != (CPU_TS *)0) {
           *p_ts = 0u;
        }
#endif
        OS_TRACE_MUTEX_PEND_FAILED(p_mutex);
        OS_TRACE_MUTEX_PEND_EXIT(OS_ERR_PEND_WOULD_BLOCK);
       *p_err = OS_ERR_PEND_WOULD_BLOCK;                        /* No                                                   */
        return;
    } else {
        if (OSSchedLockNestingCtr > 0u) {                       /* Can't pend when the scheduler is locked              */
            CPU_CRITICAL_EXIT();
#if (OS_CFG_TS_EN > 0u)
            if (p_ts != (CPU_TS *)0) {
               *p_ts = 0u;
            }
#endif
            OS_TRACE_MUTEX_PEND_FAILED(p_mutex);
            OS_TRACE_MUTEX_PEND_EXIT(OS_ERR_SCHED_LOCKED);
           *p_err = OS_ERR_SCHED_LOCKED;
            return;
        }
    }

    p_tcb = p_mutex->OwnerTCBPtr;                               /* Point to the TCB of the Mutex owner                  */
    if (p_tcb->Prio > OSTCBCurPtr->Prio) {                      /* See if mutex owner has a lower priority than current */
        OS_TaskChangePrio(p_tcb, OSTCBCurPtr->Prio);
        OS_TRACE_MUTEX_TASK_PRIO_INHERIT(p_tcb, p_tcb->Prio);
    }

    OS_Pend((OS_PEND_OBJ *)((void *)p_mutex),                   /* Block task pending on Mutex                          */
             OSTCBCurPtr,
             OS_TASK_PEND_ON_MUTEX,
             timeout);

    CPU_CRITICAL_EXIT();
    OS_TRACE_MUTEX_PEND_BLOCK(p_mutex);
    OSSched();                                                  /* Find the next highest priority task ready to run     */

    CPU_CRITICAL_ENTER();
    switch (OSTCBCurPtr->PendStatus) {
        case OS_STATUS_PEND_OK:                                 /* We got the mutex                                     */
#if (OS_CFG_TS_EN > 0u)
             if (p_ts != (CPU_TS *)0) {
                *p_ts = OSTCBCurPtr->TS;
             }
#endif
             OS_TRACE_MUTEX_PEND(p_mutex);
            *p_err = OS_ERR_NONE;
             break;

        case OS_STATUS_PEND_ABORT:                              /* Indicate that we aborted                             */
#if (OS_CFG_TS_EN > 0u)
             if (p_ts != (CPU_TS *)0) {
                *p_ts = OSTCBCurPtr->TS;
             }
#endif
             OS_TRACE_MUTEX_PEND_FAILED(p_mutex);
            *p_err = OS_ERR_PEND_ABORT;
             break;

        case OS_STATUS_PEND_TIMEOUT:                            /* Indicate that we didn't get mutex within timeout     */
#if (OS_CFG_TS_EN > 0u)
             if (p_ts != (CPU_TS *)0) {
                *p_ts = 0u;
             }
#endif
             OS_TRACE_MUTEX_PEND_FAILED(p_mutex);
            *p_err = OS_ERR_TIMEOUT;
             break;

        case OS_STATUS_PEND_DEL:                                /* Indicate that object pended on has been deleted      */
#if (OS_CFG_TS_EN > 0u)
             if (p_ts != (CPU_TS *)0) {
                *p_ts = OSTCBCurPtr->TS;
             }
#endif
             OS_TRACE_MUTEX_PEND_FAILED(p_mutex);
            *p_err = OS_ERR_OBJ_DEL;
             break;

        default:
             OS_TRACE_MUTEX_PEND_FAILED(p_mutex);
            *p_err = OS_ERR_STATUS_INVALID;
             break;
    }
    CPU_CRITICAL_EXIT();
    OS_TRACE_MUTEX_PEND_EXIT(*p_err);
}


/*
************************************************************************************************************************
*                                               ABORT WAITING ON A MUTEX
*
* Description: This function aborts & readies any tasks currently waiting on a mutex.  This function should be used
*              to fault-abort the wait on the mutex, rather than to normally signal the mutex via OSMutexPost().
*
* Arguments  : p_mutex       is a pointer to the mutex
*
*              opt           determines the type of ABORT performed:
*
*                                OS_OPT_PEND_ABORT_1          ABORT wait for a single task (HPT) waiting on the mutex
*                                OS_OPT_PEND_ABORT_ALL        ABORT wait for ALL tasks that are  waiting on the mutex
*                                OS_OPT_POST_NO_SCHED         Do not call the scheduler
*
*              p_err         is a pointer to a variable that will contain an error code returned by this function.
*
*                                OS_ERR_NONE               At least one task waiting on the mutex was readied and
*                                                          informed of the aborted wait; check return value for the
*                                                          number of tasks whose wait on the mutex was aborted
*                                OS_ERR_OBJ_PTR_NULL       If 'p_mutex' is a NULL pointer
*                                OS_ERR_OBJ_TYPE           If 'p_mutex' is not pointing at a mutex
*                                OS_ERR_OPT_INVALID        If you specified an invalid option
*                                OS_ERR_OS_NOT_RUNNING     If uC/OS-III is not running yet
*                                OS_ERR_PEND_ABORT_ISR     If you attempted to call this function from an ISR
*                                OS_ERR_PEND_ABORT_NONE    No task were pending
*
* Returns    : == 0          if no tasks were waiting on the mutex, or upon error.
*              >  0          if one or more tasks waiting on the mutex are now readied and informed.
*
* Note(s)    : none
************************************************************************************************************************
*/

#if (OS_CFG_MUTEX_PEND_ABORT_EN > 0u)
OS_OBJ_QTY  OSMutexPendAbort (OS_MUTEX  *p_mutex,
                              OS_OPT     opt,
                              OS_ERR    *p_err)
{
    OS_PEND_LIST  *p_pend_list;
    OS_TCB        *p_tcb;
    OS_TCB        *p_tcb_owner;
    CPU_TS         ts;
    OS_OBJ_QTY     nbr_tasks;
    OS_PRIO        prio_new;
    CPU_SR_ALLOC();


#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return ((OS_OBJ_QTY)0u);
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
    if (p_mutex == (OS_MUTEX *)0) {                             /* Validate 'p_mutex'                                   */
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
    if (p_mutex->Type != OS_OBJ_TYPE_MUTEX) {                   /* Make sure mutex was created                          */
       *p_err =  OS_ERR_OBJ_TYPE;
        return (0u);
    }
#endif

    CPU_CRITICAL_ENTER();
    p_pend_list = &p_mutex->PendList;
    if (p_pend_list->HeadPtr == (OS_TCB *)0) {                  /* Any task waiting on mutex?                           */
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
        p_tcb_owner = p_mutex->OwnerTCBPtr;
        prio_new    = p_tcb_owner->Prio;
        if ((p_tcb_owner->Prio != p_tcb_owner->BasePrio) &&
            (p_tcb_owner->Prio == p_tcb->Prio)) {               /* Has the owner inherited a priority?                  */
            prio_new = OS_MutexGrpPrioFindHighest(p_tcb_owner);
            prio_new = (prio_new > p_tcb_owner->BasePrio) ? p_tcb_owner->BasePrio : prio_new;
        }

        if(prio_new != p_tcb_owner->Prio) {
            OS_TaskChangePrio(p_tcb_owner, prio_new);
            OS_TRACE_MUTEX_TASK_PRIO_DISINHERIT(p_tcb_owner, p_tcb_owner->Prio);
        }

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
*                                                   POST TO A MUTEX
*
* Description: This function signals a mutex.
*
* Arguments  : p_mutex       is a pointer to the mutex
*
*              opt           is an option you can specify to alter the behavior of the post.  The choices are:
*
*                                OS_OPT_POST_NONE        No special option selected
*                                OS_OPT_POST_NO_SCHED    If you don't want the scheduler to be called after the post.
*
*              p_err         is a pointer to a variable that will contain an error code returned by this function.
*
*                                OS_ERR_NONE               The call was successful and the mutex was signaled
*                                OS_ERR_MUTEX_NESTING      Mutex owner nested its use of the mutex
*                                OS_ERR_MUTEX_NOT_OWNER    If the task posting is not the Mutex owner
*                                OS_ERR_OBJ_PTR_NULL       If 'p_mutex' is a NULL pointer
*                                OS_ERR_OBJ_TYPE           If 'p_mutex' is not pointing at a mutex
*                                OS_ERR_OPT_INVALID        If you specified an invalid option
*                                OS_ERR_OS_NOT_RUNNING     If uC/OS-III is not running yet
*                                OS_ERR_POST_ISR           If you attempted to post from an ISR
*
* Returns    : none
*
* Note(s)    : none
************************************************************************************************************************
*/

void  OSMutexPost (OS_MUTEX  *p_mutex,
                   OS_OPT     opt,
                   OS_ERR    *p_err)
{
    OS_PEND_LIST  *p_pend_list;
    OS_TCB        *p_tcb;
    CPU_TS         ts;
    OS_PRIO        prio_new;
    CPU_SR_ALLOC();


#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return;
    }
#endif

    OS_TRACE_MUTEX_POST_ENTER(p_mutex, opt);

#if (OS_CFG_CALLED_FROM_ISR_CHK_EN > 0u)
    if (OSIntNestingCtr > 0u) {                                 /* Not allowed to call from an ISR                      */
        OS_TRACE_MUTEX_POST_FAILED(p_mutex);
        OS_TRACE_MUTEX_POST_EXIT(OS_ERR_POST_ISR);
       *p_err = OS_ERR_POST_ISR;
        return;
    }
#endif

#if (OS_CFG_INVALID_OS_CALLS_CHK_EN > 0u)
    if (OSRunning != OS_STATE_OS_RUNNING) {                     /* Is the kernel running?                               */
        OS_TRACE_MUTEX_POST_EXIT(OS_ERR_OS_NOT_RUNNING);
       *p_err = OS_ERR_OS_NOT_RUNNING;
        return;
    }
#endif

#if (OS_CFG_ARG_CHK_EN > 0u)
    if (p_mutex == (OS_MUTEX *)0) {                             /* Validate 'p_mutex'                                   */
        OS_TRACE_MUTEX_POST_FAILED(p_mutex);
        OS_TRACE_MUTEX_POST_EXIT(OS_ERR_OBJ_PTR_NULL);
       *p_err = OS_ERR_OBJ_PTR_NULL;
        return;
    }
    switch (opt) {                                              /* Validate 'opt'                                       */
        case OS_OPT_POST_NONE:
        case OS_OPT_POST_NO_SCHED:
             break;

        default:
             OS_TRACE_MUTEX_POST_FAILED(p_mutex);
             OS_TRACE_MUTEX_POST_EXIT(OS_ERR_OPT_INVALID);
            *p_err =  OS_ERR_OPT_INVALID;
             return;
    }
#endif

#if (OS_CFG_OBJ_TYPE_CHK_EN > 0u)
    if (p_mutex->Type != OS_OBJ_TYPE_MUTEX) {                   /* Make sure mutex was created                          */
        OS_TRACE_MUTEX_POST_FAILED(p_mutex);
        OS_TRACE_MUTEX_POST_EXIT(OS_ERR_OBJ_TYPE);
       *p_err = OS_ERR_OBJ_TYPE;
        return;
    }
#endif

    CPU_CRITICAL_ENTER();
    if (OSTCBCurPtr != p_mutex->OwnerTCBPtr) {                  /* Make sure the mutex owner is releasing the mutex     */
        CPU_CRITICAL_EXIT();
        OS_TRACE_MUTEX_POST_FAILED(p_mutex);
        OS_TRACE_MUTEX_POST_EXIT(OS_ERR_MUTEX_NOT_OWNER);
       *p_err = OS_ERR_MUTEX_NOT_OWNER;
        return;
    }

    OS_TRACE_MUTEX_POST(p_mutex);

#if (OS_CFG_TS_EN > 0u)
    ts          = OS_TS_GET();                                  /* Get timestamp                                        */
    p_mutex->TS = ts;
#else
    ts          = 0u;
#endif
    p_mutex->OwnerNestingCtr--;                                 /* Decrement owner's nesting counter                    */
    if (p_mutex->OwnerNestingCtr > 0u) {                        /* Are we done with all nestings?                       */
        CPU_CRITICAL_EXIT();                                     /* No                                                   */
        OS_TRACE_MUTEX_POST_EXIT(OS_ERR_MUTEX_NESTING);
       *p_err = OS_ERR_MUTEX_NESTING;
        return;
    }

    OS_MutexGrpRemove(OSTCBCurPtr, p_mutex);                    /* Remove mutex from owner's group                      */

    p_pend_list = &p_mutex->PendList;
    if (p_pend_list->HeadPtr == (OS_TCB *)0) {                  /* Any task waiting on mutex?                           */
        p_mutex->OwnerTCBPtr     = (OS_TCB *)0;                 /* No                                                   */
        p_mutex->OwnerNestingCtr =           0u;
        CPU_CRITICAL_EXIT();
        OS_TRACE_MUTEX_POST_EXIT(OS_ERR_NONE);
       *p_err = OS_ERR_NONE;
        return;
    }
                                                                /* Yes                                                  */
    if (OSTCBCurPtr->Prio != OSTCBCurPtr->BasePrio) {           /* Has owner inherited a priority?                      */
        prio_new = OS_MutexGrpPrioFindHighest(OSTCBCurPtr);     /* Yes, find highest priority pending                   */
        prio_new = (prio_new > OSTCBCurPtr->BasePrio) ? OSTCBCurPtr->BasePrio : prio_new;
        if (prio_new > OSTCBCurPtr->Prio) {
            OS_RdyListRemove(OSTCBCurPtr);
            OSTCBCurPtr->Prio = prio_new;                       /* Lower owner's priority back to its original one      */
            OS_TRACE_MUTEX_TASK_PRIO_DISINHERIT(OSTCBCurPtr, prio_new);
            OS_PrioInsert(prio_new);
            OS_RdyListInsertTail(OSTCBCurPtr);                  /* Insert owner in ready list at new priority           */
            OSPrioCur         = prio_new;
        }
    }
                                                                /* Get TCB from head of pend list                       */
    p_tcb                    = p_pend_list->HeadPtr;
    p_mutex->OwnerTCBPtr     = p_tcb;                           /* Give mutex to new owner                              */
    p_mutex->OwnerNestingCtr = 1u;
    OS_MutexGrpAdd(p_tcb, p_mutex);
                                                                /* Post to mutex                                        */
    OS_Post((OS_PEND_OBJ *)((void *)p_mutex),
                           p_tcb,
                           (void *)0,
                           0u,
                           ts);

    CPU_CRITICAL_EXIT();

    if ((opt & OS_OPT_POST_NO_SCHED) == 0u) {
        OSSched();                                              /* Run the scheduler                                    */
    }
    OS_TRACE_MUTEX_POST_EXIT(OS_ERR_NONE);
   *p_err = OS_ERR_NONE;
}


/*
************************************************************************************************************************
*                                            CLEAR THE CONTENTS OF A MUTEX
*
* Description: This function is called by OSMutexDel() to clear the contents of a mutex
*

* Argument(s): p_mutex      is a pointer to the mutex to clear
*              -------
*
* Returns    : none
*
* Note(s)    : This function is INTERNAL to uC/OS-III and your application should not call it.
************************************************************************************************************************
*/

void  OS_MutexClr (OS_MUTEX  *p_mutex)
{
#if (OS_OBJ_TYPE_REQ > 0u)
    p_mutex->Type              =  OS_OBJ_TYPE_NONE;             /* Mark the data structure as a NONE                    */
#endif
#if (OS_CFG_DBG_EN > 0u)
    p_mutex->NamePtr           = (CPU_CHAR *)((void *)"?MUTEX");
#endif
    p_mutex->MutexGrpNextPtr   = (OS_MUTEX *)0;
    p_mutex->OwnerTCBPtr       = (OS_TCB   *)0;
    p_mutex->OwnerNestingCtr   =             0u;
#if (OS_CFG_TS_EN > 0u)
    p_mutex->TS                =             0u;
#endif
    OS_PendListInit(&p_mutex->PendList);                        /* Initialize the waiting list                          */
}


/*
************************************************************************************************************************
*                                          ADD/REMOVE MUTEX TO/FROM DEBUG LIST
*
* Description: These functions are called by uC/OS-III to add or remove a mutex to/from the debug list.
*
* Arguments  : p_mutex     is a pointer to the mutex to add/remove
*
* Returns    : none
*
* Note(s)    : These functions are INTERNAL to uC/OS-III and your application should not call it.
************************************************************************************************************************
*/

#if (OS_CFG_DBG_EN > 0u)
void  OS_MutexDbgListAdd (OS_MUTEX  *p_mutex)
{
    p_mutex->DbgNamePtr               = (CPU_CHAR *)((void *)" ");
    p_mutex->DbgPrevPtr               = (OS_MUTEX *)0;
    if (OSMutexDbgListPtr == (OS_MUTEX *)0) {
        p_mutex->DbgNextPtr           = (OS_MUTEX *)0;
    } else {
        p_mutex->DbgNextPtr           =  OSMutexDbgListPtr;
        OSMutexDbgListPtr->DbgPrevPtr =  p_mutex;
    }
    OSMutexDbgListPtr                 =  p_mutex;
}


void  OS_MutexDbgListRemove (OS_MUTEX  *p_mutex)
{
    OS_MUTEX  *p_mutex_next;
    OS_MUTEX  *p_mutex_prev;


    p_mutex_prev = p_mutex->DbgPrevPtr;
    p_mutex_next = p_mutex->DbgNextPtr;

    if (p_mutex_prev == (OS_MUTEX *)0) {
        OSMutexDbgListPtr = p_mutex_next;
        if (p_mutex_next != (OS_MUTEX *)0) {
            p_mutex_next->DbgPrevPtr = (OS_MUTEX *)0;
        }
        p_mutex->DbgNextPtr = (OS_MUTEX *)0;

    } else if (p_mutex_next == (OS_MUTEX *)0) {
        p_mutex_prev->DbgNextPtr = (OS_MUTEX *)0;
        p_mutex->DbgPrevPtr      = (OS_MUTEX *)0;

    } else {
        p_mutex_prev->DbgNextPtr =  p_mutex_next;
        p_mutex_next->DbgPrevPtr =  p_mutex_prev;
        p_mutex->DbgNextPtr      = (OS_MUTEX *)0;
        p_mutex->DbgPrevPtr      = (OS_MUTEX *)0;
    }
}
#endif


/*
************************************************************************************************************************
*                                               MUTEX GROUP ADD
*
* Description: This function is called by the kernel to add a mutex to a task's mutex group.
*

* Argument(s): p_tcb        is a pointer to the tcb of the task to give the mutex to.
*
*              p_mutex      is a point to the mutex to add to the group.
*
*
* Returns    : none
*
* Note(s)    : 1) This function is INTERNAL to uC/OS-III and your application MUST NOT call it.
************************************************************************************************************************
*/

void  OS_MutexGrpAdd (OS_TCB  *p_tcb, OS_MUTEX  *p_mutex)
{
    p_mutex->MutexGrpNextPtr = p_tcb->MutexGrpHeadPtr;      /* The mutex grp is not sorted add to head of list.       */
    p_tcb->MutexGrpHeadPtr   = p_mutex;
}


/*
************************************************************************************************************************
*                                              MUTEX GROUP REMOVE
*
* Description: This function is called by the kernel to remove a mutex to a task's mutex group.
*

* Argument(s): p_tcb        is a pointer to the tcb of the task to remove the mutex from.
*
*              p_mutex      is a point to the mutex to remove from the group.
*
*
* Returns    : none
*
* Note(s)    : 1) This function is INTERNAL to uC/OS-III and your application MUST NOT call it.
************************************************************************************************************************
*/

void  OS_MutexGrpRemove (OS_TCB  *p_tcb, OS_MUTEX  *p_mutex)
{
    OS_MUTEX  **pp_mutex;

    pp_mutex = &p_tcb->MutexGrpHeadPtr;

    while(*pp_mutex != p_mutex) {
        pp_mutex = &(*pp_mutex)->MutexGrpNextPtr;
    }

    *pp_mutex = (*pp_mutex)->MutexGrpNextPtr;
}


/*
************************************************************************************************************************
*                                              MUTEX FIND HIGHEST PENDING
*
* Description: This function is called by the kernel to find the highest task pending on any mutex from a group.
*

* Argument(s): p_tcb        is a pointer to the tcb of the task to process.
*
*
* Returns    : Highest priority pending or OS_CFG_PRIO_MAX - 1u if none found.
*
* Note(s)    : 1) This function is INTERNAL to uC/OS-III and your application MUST NOT call it.
************************************************************************************************************************
*/

OS_PRIO  OS_MutexGrpPrioFindHighest (OS_TCB  *p_tcb)
{
    OS_MUTEX  **pp_mutex;
    OS_PRIO     highest_prio;
    OS_PRIO     prio;
    OS_TCB     *p_head;


    highest_prio = (OS_PRIO)(OS_CFG_PRIO_MAX - 1u);
    pp_mutex = &p_tcb->MutexGrpHeadPtr;

    while(*pp_mutex != (OS_MUTEX *)0) {
        p_head = (*pp_mutex)->PendList.HeadPtr;
        if (p_head != (OS_TCB *)0) {
            prio = p_head->Prio;
            if(prio < highest_prio) {
                highest_prio = prio;
            }
        }
        pp_mutex = &(*pp_mutex)->MutexGrpNextPtr;
    }

    return (highest_prio);
}


/*
************************************************************************************************************************
*                                               MUTEX GROUP POST ALL
*
* Description: This function is called by the kernel to post (release) all the mutex from a group. Used when deleting
*              a task.
*

* Argument(s): p_tcb        is a pointer to the tcb of the task to process.
*
*
* Returns    : none.
*
* Note(s)    : 1) This function is INTERNAL to uC/OS-III and your application MUST NOT call it.
************************************************************************************************************************
*/

void  OS_MutexGrpPostAll (OS_TCB  *p_tcb)
{
    OS_MUTEX      *p_mutex;
    OS_MUTEX      *p_mutex_next;
    CPU_TS         ts;
    OS_PEND_LIST  *p_pend_list;
    OS_TCB        *p_tcb_new;


    p_mutex = p_tcb->MutexGrpHeadPtr;

    while(p_mutex != (OS_MUTEX *)0) {

        OS_TRACE_MUTEX_POST(p_mutex);

        p_mutex_next = p_mutex->MutexGrpNextPtr;
#if (OS_CFG_TS_EN > 0u)
        ts           = OS_TS_GET();                             /* Get timestamp                                        */
        p_mutex->TS  = ts;
#else
        ts           = 0u;
#endif
        OS_MutexGrpRemove(p_tcb,  p_mutex);                     /* Remove mutex from owner's group                      */

        p_pend_list = &p_mutex->PendList;
        if (p_pend_list->HeadPtr == (OS_TCB *)0) {              /* Any task waiting on mutex?                           */
            p_mutex->OwnerNestingCtr =           0u;            /* Decrement owner's nesting counter                    */
            p_mutex->OwnerTCBPtr     = (OS_TCB *)0;             /* No                                                   */
        } else {
                                                                /* Get TCB from head of pend list                       */
            p_tcb_new                = p_pend_list->HeadPtr;
            p_mutex->OwnerTCBPtr     = p_tcb;                   /* Give mutex to new owner                              */
            p_mutex->OwnerNestingCtr = 1u;
            OS_MutexGrpAdd(p_tcb_new, p_mutex);
                                                                /* Post to mutex                                        */
            OS_Post((OS_PEND_OBJ *)((void *)p_mutex),
                                   p_tcb_new,
                                   (void *)0,
                                   0u,
                                   ts);
        }

        p_mutex = p_mutex_next;
    }

}

#endif /* OS_CFG_MUTEX_EN */
