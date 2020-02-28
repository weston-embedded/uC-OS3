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
*                                 THREAD LOCAL STORAGE (TLS) MANAGEMENT
*                                         NEWLIB IMPLEMENTATION
*
* File    : os_tls.c
* Version : V3.08.00
*********************************************************************************************************
*/

#define  MICRIUM_SOURCE
#include "../../Source/os.h"

#if defined(OS_CFG_TLS_TBL_SIZE) && (OS_CFG_TLS_TBL_SIZE > 0u)
#include <reent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef VSC_INCLUDE_SOURCE_FILE_NAMES
const  CPU_CHAR  *os_tls__c = "$Id: $";
#endif


/*
************************************************************************************************************************
*                                                     DATA TYPES
************************************************************************************************************************
*/

typedef  struct  _reent  REENT;

/*
************************************************************************************************************************
*                                                   LOCAL VARIABLES
************************************************************************************************************************
*/

static  CPU_DATA             OS_TLS_NextAvailID;                          /* Next available TLS ID                    */

static  CPU_DATA             OS_TLS_NewLibID;                             /* ID used to store library space pointer   */

static  OS_MUTEX             OS_TLS_NewLib_MallocMutex;                   /* NewLib malloc() Mutex                    */
static  OS_MUTEX             OS_TLS_NewLib_EnvMutex;                      /* NewLib env()    Mutex                    */

/*
************************************************************************************************************************
*                                                   LOCAL FUNCTIONS
************************************************************************************************************************
*/

static  void  OS_TLS_NewLib_MallocLock  (void);
static  void  OS_TLS_NewLib_MallocUnlock(void);
static  void  OS_TLS_NewLib_EnvLock     (void);
static  void  OS_TLS_NewLib_EnvUnlock   (void);


/*
************************************************************************************************************************
*                                       ALLOCATE THE NEXT AVAILABLE TLS ID
*
* Description: This function is called to obtain the ID of the next free TLS (Task Local Storage) register 'id'
*
* Arguments  : p_err       is a pointer to a variable that will hold an error code related to this call.
*
*                            OS_ERR_NONE               if the call was successful
*                            OS_ERR_TLS_NO_MORE_AVAIL  if you are attempting to assign more TLS than you declared
*                                                           available through OS_CFG_TLS_TBL_SIZE.
*
* Returns    : The next available TLS 'id' or OS_CFG_TLS_TBL_SIZE if an error is detected.
************************************************************************************************************************
*/

OS_TLS_ID  OS_TLS_GetID (OS_ERR  *p_err)
{
    OS_TLS_ID  id;
    CPU_SR_ALLOC();



#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return ((OS_TLS_ID)OS_CFG_TLS_TBL_SIZE);
    }
#endif

    CPU_CRITICAL_ENTER();
    if (OS_TLS_NextAvailID >= OS_CFG_TLS_TBL_SIZE) {        /* See if we exceeded the number of IDs available         */
       *p_err = OS_ERR_TLS_NO_MORE_AVAIL;                   /* Yes, cannot allocate more TLS                          */
        CPU_CRITICAL_EXIT();
        return ((OS_TLS_ID)OS_CFG_TLS_TBL_SIZE);
    }

    id    = OS_TLS_NextAvailID;
    OS_TLS_NextAvailID++;
    CPU_CRITICAL_EXIT();
   *p_err = OS_ERR_NONE;
    return (id);
}


/*
************************************************************************************************************************
*                                        GET THE CURRENT VALUE OF A TLS REGISTER
*
* Description: This function is called to obtain the current value of a TLS register
*
* Arguments  : p_tcb     is a pointer to the OS_TCB of the task you want to read the TLS register from.  If 'p_tcb' is
*                        a NULL pointer then you will get the TLS register of the current task.
*
*              id        is the 'id' of the desired TLS register.  Note that the 'id' must be less than
*                        'OS_TLS_NextAvailID'
*
*              p_err     is a pointer to a variable that will hold an error code related to this call.
*
*                            OS_ERR_NONE            if the call was successful
*                            OS_ERR_OS_NOT_RUNNING  if the kernel has not started yet
*                            OS_ERR_TLS_ID_INVALID  if the 'id' is greater or equal to OS_TLS_NextAvailID
*                            OS_ERR_TLS_NOT_EN      if the task was created by specifying that TLS support was not
*                                                     needed for the task
*
* Returns    : The current value of the task's TLS register or 0 if an error is detected.
*
* Note(s)    : 1) p_tcb->Opt contains options passed to OSTaskCreate().  One of these options (OS_OPT_TASK_NO_TLS) is
*                 used to specify that the user doesn't want TLS support for the task being created.  In other words,
*                 by default, TLS support is enabled if OS_CFG_TLS_TBL_SIZE is defined and > 0 so the user must
*                 specifically indicate that he/she doesn't want TLS supported for a task.
************************************************************************************************************************
*/

OS_TLS  OS_TLS_GetValue (OS_TCB     *p_tcb,
                         OS_TLS_ID   id,
                         OS_ERR     *p_err)
{
    OS_TLS    value;
    CPU_SR_ALLOC();



#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return ((OS_TLS)0);
    }
#endif


#if OS_CFG_ARG_CHK_EN > 0u
    if (id >= OS_TLS_NextAvailID) {                             /* Caller must specify an ID that's been assigned     */
       *p_err = OS_ERR_TLS_ID_INVALID;
        return ((OS_TLS)0);
    }
#endif

    CPU_CRITICAL_ENTER();
    if (p_tcb == (OS_TCB *)0) {                                 /* Does caller want to use current task's TCB?        */
        p_tcb = OSTCBCurPtr;                                    /* Yes                                                */
        if (OSTCBCurPtr == (OS_TCB *)0) {                       /* Is the kernel running?                             */
            CPU_CRITICAL_EXIT();                                /* No, then caller cannot specify NULL                */
           *p_err = OS_ERR_OS_NOT_RUNNING;
            return ((OS_TLS)0);
        }
    }
    if ((p_tcb->Opt & OS_OPT_TASK_NO_TLS) == OS_OPT_NONE) {     /* See if TLS is available for this task              */
        value = p_tcb->TLS_Tbl[id];                             /* Yes                                                */
        CPU_CRITICAL_EXIT();
       *p_err = OS_ERR_NONE;
        return ((OS_TLS)value);
    } else {
        CPU_CRITICAL_EXIT();                                    /* No                                                 */
       *p_err = OS_ERR_TLS_NOT_EN;
        return ((OS_TLS)0);
    }
}


/*
************************************************************************************************************************
*                                          DEFINE TLS DESTRUCTOR FUNCTION
*
* Description: This function is called by the user to assign a 'destructor' function to a specific TLS.  When a task is
*              deleted, all the destructors are called for all the task's TLS for which there is a destructor function
*              defined.  In other when a task is deleted, all the non-NULL functions present in OS_TLS_DestructPtrTbl[]
*              will be called.
*
* Arguments  : id          is the ID of the TLS destructor to set
*
*              p_destruct  is a pointer to a function that is associated with a specific TLS register and is called when
*                          a task is deleted.  The prototype of such functions is:
*
*                            void  MyDestructFunction (OS_TCB     *p_tcb,
*                                                      OS_TLS_ID   id,
*                                                      OS_TLS      value);
*
*                          you can specify a NULL pointer if you don't want to have a fucntion associated with a TLS
*                          register.  A NULL pointer (i.e. no function associated with a TLS register) is the default
*                          value placed in OS_TLS_DestructPtrTbl[].
*
*              p_err       is a pointer to an error return code.  The possible values are:
*
*                            OS_ERR_NONE             The call was successful.
*                            OS_ERR_TLS_ID_INVALID   You you specified an invalid TLS ID
*
* Returns    : none
*
* Note       : none
************************************************************************************************************************
*/

void  OS_TLS_SetDestruct (OS_TLS_ID            id,
                          OS_TLS_DESTRUCT_PTR  p_destruct,
                          OS_ERR              *p_err)
{
   (void)&id;
   (void)&p_destruct;
   *p_err = OS_ERR_NONE;
}


/*
************************************************************************************************************************
*                                       SET THE CURRENT VALUE OF A TASK TLS REGISTER
*
* Description: This function is called to change the current value of a task TLS register.
*
* Arguments  : p_tcb     is a pointer to the OS_TCB of the task you want to set the task's TLS register for.  If 'p_tcb'
*                        is a NULL pointer then you will change the TLS register of the current task.
*
*              id        is the 'id' of the desired task TLS register.  Note that the 'id' must be less than
*                        'OS_TLS_NextAvailID'
*
*              value     is the desired value for the task TLS register.
*
*              p_err     is a pointer to a variable that will hold an error code related to this call.
*
*                            OS_ERR_NONE            if the call was successful
*                            OS_ERR_OS_NOT_RUNNING  if the kernel has not started yet
*                            OS_ERR_TLS_ID_INVALID  if you specified an invalid TLS ID
*                            OS_ERR_TLS_NOT_EN      if the task was created by specifying that TLS support was not
*                                                     needed for the task
*
* Returns    : none
*
* Note(s)    : 1) p_tcb->Opt contains options passed to OSTaskCreate().  One of these options (OS_OPT_TASK_NO_TLS) is
*                 used to specify that the user doesn't want TLS support for the task being created.  In other words,
*                 by default, TLS support is enabled if OS_CFG_TLS_TBL_SIZE is defined and > 0 so the user must
*                 specifically indicate that he/she doesn't want TLS supported for a task.
************************************************************************************************************************
*/

void  OS_TLS_SetValue (OS_TCB     *p_tcb,
                       OS_TLS_ID   id,
                       OS_TLS      value,
                       OS_ERR     *p_err)
{
    CPU_SR_ALLOC();



#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return;
    }
#endif

#if OS_CFG_ARG_CHK_EN > 0u
    if (id >= OS_TLS_NextAvailID) {                             /* Caller must specify an ID that's been assigned     */
       *p_err = OS_ERR_TLS_ID_INVALID;
        return;
    }
#endif

    CPU_CRITICAL_ENTER();                                       /* Does caller want to use current task's TCB?        */
    if (p_tcb == (OS_TCB *)0) {                                 /* Yes                                                */
        p_tcb = OSTCBCurPtr;                                    /* Is the kernel running?                             */
        if (OSTCBCurPtr == (OS_TCB *)0) {                       /* No, then caller cannot specify NULL                */
            CPU_CRITICAL_EXIT();
           *p_err = OS_ERR_OS_NOT_RUNNING;
            return;
        }
    }
    if ((p_tcb->Opt & OS_OPT_TASK_NO_TLS) == OS_OPT_NONE) {     /* See if TLS is available for this task              */
        p_tcb->TLS_Tbl[id] = value;                             /* Yes                                                */
        CPU_CRITICAL_EXIT();
       *p_err              = OS_ERR_NONE;
    } else {
        CPU_CRITICAL_EXIT();                                    /* No                                                 */
       *p_err              = OS_ERR_TLS_NOT_EN;
    }
}


/*
************************************************************************************************************************
************************************************************************************************************************
*                                             uC/OS-III INTERNAL FUNCTIONS
*                                         DO NOT CALL FROM THE APPLICATION CODE
************************************************************************************************************************
************************************************************************************************************************
*/

/*
************************************************************************************************************************
*                                       INITIALIZE THE TASK LOCAL STORAGE SERVICES
*
* Description: This function is called by uC/OS-III to initialize the TLS id allocator.
*
*              This function also initializes an array containing function pointers.  There is one function associated
*                  to each task TLS register and all the functions (assuming non-NULL) will be called when the task is
*                  deleted.
*
* Arguments  : p_err    is a pointer to an error return value.  Possible values are:
*
*                       OS_ERR_NONE               if the call was successful
*                       OS_ERR_TLS_NO_MORE_AVAIL  if you are attempting to assign more TLS than you declared
*                                                      available through OS_CFG_TLS_TBL_SIZE.
*
* Returns    : none
*
* Note       : This function is INTERNAL to uC/OS-III and your application should not call it.
************************************************************************************************************************
*/

void  OS_TLS_Init (OS_ERR *p_err)
{
    OS_TLS_NextAvailID = 0u;

    OS_TLS_NewLibID    = OS_TLS_GetID(p_err);
    if (*p_err != OS_ERR_NONE) {
        return;
    }

    OSMutexCreate(&OS_TLS_NewLib_MallocMutex,
                  "TLS Malloc Mtx",
                   p_err);

    OSMutexCreate(&OS_TLS_NewLib_EnvMutex,
                  "TLS Env    Mtx",
                   p_err);
}


/*
************************************************************************************************************************
*                                                  TASK CREATE HOOK
*
* Description: This function is called by OSTaskCreate()
*
* Arguments  : p_tcb     is a pointer to the OS_TCB of the task being created.
*
* Returns    : none
*
* Note(s)    : 1) OSTaskCreate() clears all entries in p_tcb->TLS_Tbl[] before calling OS_TLS_TaskCreate() so no need
*                 to this here.
************************************************************************************************************************
*/

void  OS_TLS_TaskCreate (OS_TCB  *p_tcb)
{
    OS_TLS  p_tls;


    if ((p_tcb->Opt & OS_OPT_TASK_NO_TLS) == OS_OPT_NONE) {           /* See if TLS is available for this task        */
        p_tls                           = (OS_TLS)malloc(sizeof(struct _reent));
        memset((void *)p_tls, 0, sizeof(struct _reent));              /* Clear the data structure                     */
        p_tcb->TLS_Tbl[OS_TLS_NewLibID] = p_tls;                      /* Save pointer to this storage area in the TCB */
    }
}


/*
************************************************************************************************************************
*                                                  TASK DELETE HOOK
*
* Description: This function is called by OSTaskDel()
*
* Arguments  : p_tcb     is a pointer to the OS_TCB of the task being deleted.
*
* Returns    : none
************************************************************************************************************************
*/

void  OS_TLS_TaskDel (OS_TCB  *p_tcb)
{
    OS_TLS      p_tls;
    FILE       *fp;
    CPU_INT08U  i;


    p_tls = p_tcb->TLS_Tbl[OS_TLS_NewLibID];
    if ((p_tcb->Opt & OS_OPT_TASK_NO_TLS) == OS_OPT_NONE) {    /* See if TLS is available for this task               */
        if (p_tls != (OS_TLS)0) {
            fp = &((REENT *)p_tls)->__sf[0];
            for (i = 0; i < 3; i++) {                          /* Avoid closing stdin, stdout, stderr so ...          */
                fp->_close = NULL;                             /* ... other threads can still use them.               */
                fp++;
            }
            free((struct _reent *)p_tls);                      /* Free all the heap memory in the reent structure.    */
        }
        p_tcb->TLS_Tbl[OS_TLS_NewLibID] = (OS_TLS)0;           /* Put null pointer indicating no longer valid pointer */
    }
}


/*
************************************************************************************************************************
*                                                  TASK SWITCH HOOK
*
* Description: This function is called by OSSched() and OSIntExit() just prior to calling the context switch code
*
* Arguments  : none
*
* Returns    : none
*
* Note       : 1) It's assumed that OSTCBCurPtr points to the task being switched out and OSTCBHighRdyPtr points to the
*                 task being switched in.
************************************************************************************************************************
*/

void  OS_TLS_TaskSw (void)
{
    OS_TLS  p_tls;


    p_tls = OSTCBHighRdyPtr->TLS_Tbl[OS_TLS_NewLibID];
    if ((OSTCBHighRdyPtr->Opt & OS_OPT_TASK_NO_TLS) == OS_OPT_NONE) {    /* See if TLS is available for this task     */
        if (p_tls != (void *)0) {
            _impure_ptr = (struct _reent *)p_tls;
        }
    }
}


/*
************************************************************************************************************************
*                                               NEWLIB 'malloc()' MUTEX
*
* Description : These functions are used to gain exclusive access to NewLib resources.
*
* Arguments   : none
************************************************************************************************************************
*/


static  void  OS_TLS_NewLib_MallocLock (void)
{
    OS_ERR  err;
    CPU_TS  ts;


    if (OSRunning == OS_STATE_OS_STOPPED) {
        return;
    }

    OSMutexPend(&OS_TLS_NewLib_MallocMutex,
                 0,
                 OS_OPT_PEND_BLOCKING,
                &ts,
                &err);
}



static  void  OS_TLS_NewLib_MallocUnlock (void)
{
    OS_ERR  err;


    if (OSRunning == OS_STATE_OS_STOPPED) {
        return;
    }

    OSMutexPost(&OS_TLS_NewLib_MallocMutex,
                 OS_OPT_POST_NONE,
                &err);
}


/*
************************************************************************************************************************
*                                               NEWLIB 'env()' MUTEX
*
* Description : These functions are used to gain exclusive access to NewLib resources.
*
* Arguments   : none
************************************************************************************************************************
*/


static  void  OS_TLS_NewLib_EnvLock (void)
{
    OS_ERR  err;
    CPU_TS  ts;


    if (OSRunning == OS_STATE_OS_STOPPED) {
        return;
    }

    OSMutexPend(&OS_TLS_NewLib_EnvMutex,
                 0,
                 OS_OPT_PEND_BLOCKING,
                &ts,
                &err);
}



static  void  OS_TLS_NewLib_EnvUnlock (void)
{
    OS_ERR  err;


    if (OSRunning == OS_STATE_OS_STOPPED) {
        return;
    }

    OSMutexPost(&OS_TLS_NewLib_EnvMutex,
                 OS_OPT_POST_NONE,
                &err);
}


/*
************************************************************************************************************************
*                                               MALLOC LOCK/UNLOCK
*
* Description : uC/OS-III implementations of newlib "__malloc_lock()" and "__malloc_unlock" functions.
*
*               The malloc family of routines call these functions when they need to lock the memory pool.  The version
*               of these routines given here use a uC/OS-III mutex to implement the lock.   This allows multiple
*               uC/OS-III tasks to call malloc().
*
*               A call to malloc() may call "__malloc_lock()" recursively; that is, the sequence of calls may go like:
*
*                   __malloc_lock();
*                   __malloc_lock();
*                   :
*                   :
*                   __malloc_unlock();
*                   __malloc_unlock();
*
*               uC/OS-III supports recursive calls.
*
* Arguments   : Pointer to a newlib reentrancy structure "struct _reent *".
************************************************************************************************************************
*/

void  __malloc_lock (struct _reent * reent)
{
    (void)reent;
    OS_TLS_NewLib_MallocLock();
}



void  __malloc_unlock (struct _reent * reent)
{
    (void)reent;
    OS_TLS_NewLib_MallocUnlock();
}


/*
************************************************************************************************************************
*                                                ENVIRONMENT LOCK/UNLOCK
*
* Description : uC/OS-III implementations of newlib "__env_lock()" and "__env_unlock()" functions.
*
*               The setenv() family of functions call these functions when they need to modify the environ variable.
*               The version of these routines given here use a uC/OS-III mutex to implement the lock.
*               This allows multiple uC/OS-III tasks to call setenv().
*
*               A call to "setenv()" may call "__env_lock()" recursively; that is, the sequence of calls may go like:
*
*                  __env_lock();
*                  __env_lock();
*                  :
*                  :
*                  __env_unlock();
*                  __env_unlock();
*
*               uC/OS-III's Mutexes support recursive calls.
*
* Arguments   : Pointer to a newlib reentrancy structure "struct _reent *".
************************************************************************************************************************
*/

void  __env_lock (struct _reent * reent)
{
    (void)reent;
    OS_TLS_NewLib_EnvLock();
}



void  __env_unlock (struct _reent * reent)
{
    (void)reent;
    OS_TLS_NewLib_EnvUnlock();
}
#endif
