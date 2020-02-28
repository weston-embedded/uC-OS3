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
*                           Cross Core Embedded Studio (CCES) IMPLEMENTATION
*
* File    : os_tls.c
* Version : V3.08.00
************************************************************************************************************************
*/

#define  MICRIUM_SOURCE
#include "../../Source/os.h"

#ifdef VSC_INCLUDE_SOURCE_FILE_NAMES
const  CPU_CHAR  *os_tls__c = "$Id: $";
#endif

#if defined(OS_CFG_TLS_TBL_SIZE) && (OS_CFG_TLS_TBL_SIZE > 0u)
/*
************************************************************************************************************************
*                                                   LOCAL VARIABLES
************************************************************************************************************************
*/

static  CPU_DATA             OS_TLS_NextAvailID;                          /* Next available TLS ID                    */
static  OS_TLS_DESTRUCT_PTR  OS_TLS_DestructPtrTbl[OS_CFG_TLS_TBL_SIZE];


/*
************************************************************************************************************************
*                                          ALLOCATE THE NEXT AVAILABLE TLS ID
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
*                                         GET THE CURRENT VALUE OF A TLS REGISTER
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
*                                           DEFINE TLS DESTRUCTOR FUNCTION
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
*                            OS_ERR_NONE                   The call was successful.
*                            OS_ERR_TLS_ID_INVALID         You you specified an invalid TLS ID
*                            OS_ERR_TLS_DESTRUCT_ASSIGNED  If a destructor has already been assigned to the TLS ID
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
    CPU_SR_ALLOC();


#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return;
    }
#endif

    if (id >= OS_TLS_NextAvailID) {                             /* See if we exceeded the number of TLS IDs available */
       *p_err = OS_ERR_TLS_ID_INVALID;
        return;
    }

    if (OS_TLS_DestructPtrTbl[id] != (OS_TLS_DESTRUCT_PTR)0) {  /* Can only assign a destructor once                  */
       *p_err = OS_ERR_TLS_DESTRUCT_ASSIGNED;
        return;
    }

    CPU_CRITICAL_ENTER();
    OS_TLS_DestructPtrTbl[id] = p_destruct;
    CPU_CRITICAL_EXIT();
   *p_err                     = OS_ERR_NONE;
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
*                                        INITIALIZE THE TASK LOCAL STORAGE SERVICES
*
* Description: This function is called by uC/OS-III to initialize the TLS id allocator.
*
*              This function also initializes an array containing function pointers.  There is one function associated
*                  to each task TLS register and all the functions (assuming non-NULL) will be called when the task is
*                  deleted.
*
* Arguments  : p_err   is a pointer to an error return value.  Current error can be:
*
*                      OS_ERR_NONE   if the call was successful
*
* Returns    : none
*
* Note       : This function is INTERNAL to uC/OS-III and your application should not call it.
************************************************************************************************************************
*/

void  OS_TLS_Init (OS_ERR *p_err)
{
    OS_TLS_ID  i;



    OS_TLS_NextAvailID = 0u;

    for (i = 0u; i < OS_CFG_TLS_TBL_SIZE; i++) {
         OS_TLS_DestructPtrTbl[i] = (OS_TLS_DESTRUCT_PTR)0;
    }
    *p_err = OS_ERR_NONE;
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
* Note(s)    : 1) This function is INTERNAL to uC/OS-III and your application should not call it.
*
*              2) OSTaskCreate() clears all entries in p_tcb->TLS_Tbl[] before calling OS_TLS_TaskCreate() so no need
*                 to this here.
************************************************************************************************************************
*/

void  OS_TLS_TaskCreate (OS_TCB  *p_tcb)
{
    (void)&p_tcb;
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
*
* Note(s)    : 1) This function is INTERNAL to uC/OS-III and your application should not call it.
************************************************************************************************************************
*/

void  OS_TLS_TaskDel (OS_TCB  *p_tcb)
{
    OS_TLS_ID             id;
    OS_TLS_DESTRUCT_PTR  *p_tbl;


    for (id = 0; id < OS_TLS_NextAvailID; id++) {     /* Call all the destructors associated with the TLS IDs         */
        p_tbl = &OS_TLS_DestructPtrTbl[id];
        if (*p_tbl != (OS_TLS_DESTRUCT_PTR)0) {
            (*p_tbl)(p_tcb, id, p_tcb->TLS_Tbl[id]);
        }
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
* Note(s)    : 1) This function is INTERNAL to uC/OS-III and your application should not call it.
*
*              2) It's assumed that OSTCBCurPtr points to the task being switched out and OSTCBHighRdyPtr points to the
*                 task being switched in.
************************************************************************************************************************
*/

void  OS_TLS_TaskSw (void)
{
}

#endif
