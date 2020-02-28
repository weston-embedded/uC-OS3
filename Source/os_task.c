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
*                                            TASK MANAGEMENT
*
* File    : os_task.c
* Version : V3.08.00
*********************************************************************************************************
*/

#define  MICRIUM_SOURCE
#include "os.h"


#ifdef VSC_INCLUDE_SOURCE_FILE_NAMES
const  CPU_CHAR  *os_task__c = "$Id: $";
#endif

/*
************************************************************************************************************************
*                                                CHANGE PRIORITY OF A TASK
*
* Description: This function allows you to change the priority of a task dynamically.  Note that the new
*              priority MUST be available.
*
* Arguments  : p_tcb      is the TCB of the tack to change the priority for
*
*              prio_new   is the new priority
*
*              p_err      is a pointer to an error code returned by this function:
*
*                             OS_ERR_NONE                    Is the call was successful
*                             OS_ERR_OS_NOT_RUNNING          If uC/OS-III is not running yet
*                             OS_ERR_PRIO_INVALID            If the priority you specify is higher that the maximum allowed
*                                                              (i.e. >= (OS_CFG_PRIO_MAX-1)) or already in use by a kernel
*                                                              task
*                             OS_ERR_STATE_INVALID           If the task is in an invalid state
*                             OS_ERR_TASK_CHANGE_PRIO_ISR    If you tried to change the task's priority from an ISR
*
* Returns    : none
*
* Note(s)    : none
************************************************************************************************************************
*/

#if (OS_CFG_TASK_CHANGE_PRIO_EN > 0u)
void  OSTaskChangePrio (OS_TCB   *p_tcb,
                        OS_PRIO   prio_new,
                        OS_ERR   *p_err)
{
#if (OS_CFG_MUTEX_EN > 0u)
    OS_PRIO  prio_high;
#endif
    CPU_SR_ALLOC();


#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return;
    }
#endif

#if (OS_CFG_ARG_CHK_EN > 0u)
    if ((p_tcb != (OS_TCB *)0) && (p_tcb->TaskState == OS_TASK_STATE_DEL)) {
       *p_err = OS_ERR_STATE_INVALID;
        return;
    }
#endif

#if (OS_CFG_CALLED_FROM_ISR_CHK_EN > 0u)
    if (OSIntNestingCtr > 0u) {                                 /* Not allowed to call from an ISR                      */
       *p_err = OS_ERR_TASK_CHANGE_PRIO_ISR;
        return;
    }
#endif

    if (prio_new >= (OS_CFG_PRIO_MAX - 1u)) {                   /* Cannot set to Idle Task priority                     */
       *p_err = OS_ERR_PRIO_INVALID;
        return;
    }

    CPU_CRITICAL_ENTER();

    if (p_tcb == (OS_TCB *)0) {                                 /* Are we changing the priority of 'self'?              */
        if (OSRunning != OS_STATE_OS_RUNNING) {
            CPU_CRITICAL_EXIT();
           *p_err = OS_ERR_OS_NOT_RUNNING;
            return;
        }
        p_tcb = OSTCBCurPtr;
    }

#if (OS_CFG_MUTEX_EN > 0u)
    p_tcb->BasePrio = prio_new;                                 /* Update base priority                                 */

    if (p_tcb->MutexGrpHeadPtr != (OS_MUTEX *)0) {              /* Owning a mutex?                                      */
        if (prio_new > p_tcb->Prio) {
            prio_high = OS_MutexGrpPrioFindHighest(p_tcb);
            if (prio_new > prio_high) {
                prio_new = prio_high;
            }
        }
    }
#endif

    OS_TaskChangePrio(p_tcb, prio_new);

    OS_TRACE_TASK_PRIO_CHANGE(p_tcb, prio_new);
    CPU_CRITICAL_EXIT();

    if (OSRunning == OS_STATE_OS_RUNNING) {
        OSSched();                                              /* Run highest priority task ready                      */
    }

   *p_err = OS_ERR_NONE;
}
#endif


/*
************************************************************************************************************************
*                                                    CREATE A TASK
*
* Description: This function is used to have uC/OS-III manage the execution of a task.  Tasks can either be created
*              prior to the start of multitasking or by a running task.  A task cannot be created by an ISR.
*
* Arguments  : p_tcb          is a pointer to the task's TCB
*
*              p_name         is a pointer to an ASCII string to provide a name to the task.
*
*              p_task         is a pointer to the task's code
*
*              p_arg          is a pointer to an optional data area which can be used to pass parameters to
*                             the task when the task first executes.  Where the task is concerned it thinks
*                             it was invoked and passed the argument 'p_arg' as follows:
*
*                                 void Task (void *p_arg)
*                                 {
*                                     for (;;) {
*                                         Task code;
*                                     }
*                                 }
*
*              prio           is the task's priority.  A unique priority MUST be assigned to each task and the
*                             lower the number, the higher the priority.
*
*              p_stk_base     is a pointer to the base address of the stack (i.e. low address).
*
*              stk_limit      is the number of stack elements to set as 'watermark' limit for the stack.  This value
*                             represents the number of CPU_STK entries left before the stack is full.  For example,
*                             specifying 10% of the 'stk_size' value indicates that the stack limit will be reached
*                             when the stack reaches 90% full.
*
*              stk_size       is the size of the stack in number of elements.  If CPU_STK is set to CPU_INT08U,
*                             'stk_size' corresponds to the number of bytes available.  If CPU_STK is set to
*                             CPU_INT16U, 'stk_size' contains the number of 16-bit entries available.  Finally, if
*                             CPU_STK is set to CPU_INT32U, 'stk_size' contains the number of 32-bit entries
*                             available on the stack.
*
*              q_size         is the maximum number of messages that can be sent to the task
*
*              time_quanta    amount of time (in ticks) for time slice when round-robin between tasks.  Specify 0 to use
*                             the default.
*
*              p_ext          is a pointer to a user supplied memory location which is used as a TCB extension.
*                             For example, this user memory can hold the contents of floating-point registers
*                             during a context switch, the time each task takes to execute, the number of times
*                             the task has been switched-in, etc.
*
*              opt            contains additional information (or options) about the behavior of the task.
*                             See OS_OPT_TASK_xxx in OS.H.  Current choices are:
*
*                                 OS_OPT_TASK_NONE            No option selected
*                                 OS_OPT_TASK_STK_CHK         Stack checking to be allowed for the task
*                                 OS_OPT_TASK_STK_CLR         Clear the stack when the task is created
*                                 OS_OPT_TASK_SAVE_FP         If the CPU has floating-point registers, save them
*                                                             during a context switch.
*                                 OS_OPT_TASK_NO_TLS          If the caller doesn't want or need TLS (Thread Local
*                                                             Storage) support for the task.  If you do not include this
*                                                             option, TLS will be supported by default.
*
*              p_err          is a pointer to an error code that will be set during this call.  The value pointer
*                             to by 'p_err' can be:
*
*                                 OS_ERR_NONE                    If the function was successful
*                                 OS_ERR_ILLEGAL_CREATE_RUN_TIME If you are trying to create the task after you called
*                                                                   OSSafetyCriticalStart()
*                                 OS_ERR_PRIO_INVALID            If the priority you specify is higher that the maximum
*                                                                   allowed (i.e. >= OS_CFG_PRIO_MAX-1) or,
*                                 OS_ERR_STK_OVF                 If the stack was overflowed during stack init
*                                 OS_ERR_STK_INVALID             If you specified a NULL pointer for 'p_stk_base'
*                                 OS_ERR_STK_SIZE_INVALID        If you specified zero for the 'stk_size'
*                                 OS_ERR_STK_LIMIT_INVALID       If you specified a 'stk_limit' greater than or equal
*                                                                   to 'stk_size'
*                                 OS_ERR_TASK_CREATE_ISR         If you tried to create a task from an ISR
*                                 OS_ERR_TASK_INVALID            If you specified a NULL pointer for 'p_task'
*                                 OS_ERR_TCB_INVALID             If you specified a NULL pointer for 'p_tcb'
*
* Returns    : none
*
* Note(s)    : 1) OSTaskCreate() will return with the error OS_ERR_STK_OVF when a stack overflow is detected
*                 during stack initialization. In that specific case some memory may have been corrupted. It is
*                 therefore recommended to treat OS_ERR_STK_OVF as a fatal error.
************************************************************************************************************************
*/

void  OSTaskCreate (OS_TCB        *p_tcb,
                    CPU_CHAR      *p_name,
                    OS_TASK_PTR    p_task,
                    void          *p_arg,
                    OS_PRIO        prio,
                    CPU_STK       *p_stk_base,
                    CPU_STK_SIZE   stk_limit,
                    CPU_STK_SIZE   stk_size,
                    OS_MSG_QTY     q_size,
                    OS_TICK        time_quanta,
                    void          *p_ext,
                    OS_OPT         opt,
                    OS_ERR        *p_err)
{
    CPU_STK_SIZE   i;
#if (OS_CFG_TASK_REG_TBL_SIZE > 0u)
    OS_REG_ID      reg_nbr;
#endif
#if defined(OS_CFG_TLS_TBL_SIZE) && (OS_CFG_TLS_TBL_SIZE > 0u)
    OS_TLS_ID      id;
#endif

    CPU_STK       *p_sp;
    CPU_STK       *p_stk_limit;
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
    if (OSIntNestingCtr > 0u) {                                 /* --------- CANNOT CREATE A TASK FROM AN ISR --------- */
        OS_TRACE_TASK_CREATE_FAILED(p_tcb);
       *p_err = OS_ERR_TASK_CREATE_ISR;
        return;
    }
#endif

#if (OS_CFG_ARG_CHK_EN > 0u)                                    /* ---------------- VALIDATE ARGUMENTS ---------------- */
    if (p_tcb == (OS_TCB *)0) {                                 /* User must supply a valid OS_TCB                      */
        OS_TRACE_TASK_CREATE_FAILED(p_tcb);
       *p_err = OS_ERR_TCB_INVALID;
        return;
    }
    if (p_task == (OS_TASK_PTR)0u) {                            /* User must supply a valid task                        */
        OS_TRACE_TASK_CREATE_FAILED(p_tcb);
       *p_err = OS_ERR_TASK_INVALID;
        return;
    }
    if (p_stk_base == (CPU_STK *)0) {                           /* User must supply a valid stack base address          */
        OS_TRACE_TASK_CREATE_FAILED(p_tcb);
       *p_err = OS_ERR_STK_INVALID;
        return;
    }
    if (stk_size < OSCfg_StkSizeMin) {                          /* User must supply a valid minimum stack size          */
        OS_TRACE_TASK_CREATE_FAILED(p_tcb);
       *p_err = OS_ERR_STK_SIZE_INVALID;
        return;
    }
    if (stk_limit >= stk_size) {                                /* User must supply a valid stack limit                 */
        OS_TRACE_TASK_CREATE_FAILED(p_tcb);
       *p_err = OS_ERR_STK_LIMIT_INVALID;
        return;
    }
    if ((prio  > (OS_CFG_PRIO_MAX - 2u)) &&                     /* Priority must be within 0 and OS_CFG_PRIO_MAX-1      */
        (prio != (OS_CFG_PRIO_MAX - 1u))) {
        OS_TRACE_TASK_CREATE_FAILED(p_tcb);
       *p_err = OS_ERR_PRIO_INVALID;
        return;
    }
#endif

    if (prio == (OS_CFG_PRIO_MAX - 1u)) {
#if (OS_CFG_TASK_IDLE_EN > 0u)
        if (p_tcb != &OSIdleTaskTCB) {
            OS_TRACE_TASK_CREATE_FAILED(p_tcb);
           *p_err = OS_ERR_PRIO_INVALID;                        /* Not allowed to use same priority as idle task        */
            return;
        }
#else
        OS_TRACE_TASK_CREATE_FAILED(p_tcb);
       *p_err = OS_ERR_PRIO_INVALID;                            /* Not allowed to use same priority as idle task        */
        return;
#endif
    }

    OS_TaskInitTCB(p_tcb);                                      /* Initialize the TCB to default values                 */

   *p_err = OS_ERR_NONE;
                                                                /* -------------- CLEAR THE TASK'S STACK -------------- */
    if (((opt & OS_OPT_TASK_STK_CHK) != 0u) ||                  /* See if stack checking has been enabled               */
        ((opt & OS_OPT_TASK_STK_CLR) != 0u)) {                  /* See if stack needs to be cleared                     */
        if ((opt & OS_OPT_TASK_STK_CLR) != 0u) {
            p_sp = p_stk_base;
            for (i = 0u; i < stk_size; i++) {                   /* Stack grows from HIGH to LOW memory                  */
               *p_sp = 0u;                                      /* Clear from bottom of stack and up!                   */
                p_sp++;
            }
        }
    }
                                                                /* ------ INITIALIZE THE STACK FRAME OF THE TASK ------ */
#if (CPU_CFG_STK_GROWTH == CPU_STK_GROWTH_HI_TO_LO)
    p_stk_limit = p_stk_base + stk_limit;
#else
    p_stk_limit = p_stk_base + (stk_size - 1u) - stk_limit;
#endif

    p_sp = OSTaskStkInit(p_task,
                         p_arg,
                         p_stk_base,
                         p_stk_limit,
                         stk_size,
                         opt);

#if (CPU_CFG_STK_GROWTH == CPU_STK_GROWTH_HI_TO_LO)             /* Check if we overflown the stack during init          */
    if (p_sp < p_stk_base) {
       *p_err = OS_ERR_STK_OVF;
        return;
    }
#else
    if (p_sp > (p_stk_base + stk_size)) {
       *p_err = OS_ERR_STK_OVF;
        return;
    }
#endif

#if (OS_CFG_TASK_STK_REDZONE_EN > 0u)                           /* Initialize Redzoned stack                            */
    OS_TaskStkRedzoneInit(p_stk_base, stk_size);
#endif

                                                                /* ------------ INITIALIZE THE TCB FIELDS ------------- */
#if (OS_CFG_DBG_EN > 0u)
    p_tcb->TaskEntryAddr = p_task;                              /* Save task entry point address                        */
    p_tcb->TaskEntryArg  = p_arg;                               /* Save task entry argument                             */
#endif

#if (OS_CFG_DBG_EN > 0u)
    p_tcb->NamePtr       = p_name;                              /* Save task name                                       */
#else
    (void)p_name;
#endif

    p_tcb->Prio          = prio;                                /* Save the task's priority                             */

#if (OS_CFG_MUTEX_EN > 0u)
    p_tcb->BasePrio      = prio;                                /* Set the base priority                                */
#endif

    p_tcb->StkPtr        = p_sp;                                /* Save the new top-of-stack pointer                    */
    p_tcb->StkLimitPtr   = p_stk_limit;                         /* Save the stack limit pointer                         */

#if (OS_CFG_SCHED_ROUND_ROBIN_EN > 0u)
    p_tcb->TimeQuanta    = time_quanta;                         /* Save the #ticks for time slice (0 means not sliced)  */
    if (time_quanta == 0u) {
        p_tcb->TimeQuantaCtr = OSSchedRoundRobinDfltTimeQuanta;
    } else {
        p_tcb->TimeQuantaCtr = time_quanta;
    }
#else
    (void)time_quanta;
#endif

    p_tcb->ExtPtr        = p_ext;                               /* Save pointer to TCB extension                        */
#if ((OS_CFG_DBG_EN > 0u) || (OS_CFG_STAT_TASK_STK_CHK_EN > 0u) || (OS_CFG_TASK_STK_REDZONE_EN > 0u))
    p_tcb->StkBasePtr    = p_stk_base;                          /* Save pointer to the base address of the stack        */
    p_tcb->StkSize       = stk_size;                            /* Save the stack size (in number of CPU_STK elements)  */
#endif
    p_tcb->Opt           = opt;                                 /* Save task options                                    */

#if (OS_CFG_TASK_REG_TBL_SIZE > 0u)
    for (reg_nbr = 0u; reg_nbr < OS_CFG_TASK_REG_TBL_SIZE; reg_nbr++) {
        p_tcb->RegTbl[reg_nbr] = 0u;
    }
#endif

#if (OS_CFG_TASK_Q_EN > 0u)
    OS_MsgQInit(&p_tcb->MsgQ,                                   /* Initialize the task's message queue                  */
                q_size);
#else
    (void)q_size;
#endif

    OSTaskCreateHook(p_tcb);                                    /* Call user defined hook                               */

    OS_TRACE_TASK_CREATE(p_tcb);
    OS_TRACE_TASK_SEM_CREATE(p_tcb, p_name);
#if (OS_CFG_TASK_Q_EN > 0u)
    OS_TRACE_TASK_MSG_Q_CREATE(&p_tcb->MsgQ, p_name);
#endif

#if defined(OS_CFG_TLS_TBL_SIZE) && (OS_CFG_TLS_TBL_SIZE > 0u)
    for (id = 0u; id < OS_CFG_TLS_TBL_SIZE; id++) {
        p_tcb->TLS_Tbl[id] = 0u;
    }
    OS_TLS_TaskCreate(p_tcb);                                   /* Call TLS hook                                        */
#endif
                                                                /* -------------- ADD TASK TO READY LIST -------------- */
    CPU_CRITICAL_ENTER();
    OS_PrioInsert(p_tcb->Prio);
    OS_RdyListInsertTail(p_tcb);

#if (OS_CFG_DBG_EN > 0u)
    OS_TaskDbgListAdd(p_tcb);
#endif

    OSTaskQty++;                                                /* Increment the #tasks counter                         */

    if (OSRunning != OS_STATE_OS_RUNNING) {                     /* Return if multitasking has not started               */
        CPU_CRITICAL_EXIT();
        return;
    }

    CPU_CRITICAL_EXIT();

    OSSched();
}


/*
************************************************************************************************************************
*                                                     DELETE A TASK
*
* Description: This function allows you to delete a task.  The calling task can delete itself by specifying a NULL
*              pointer for 'p_tcb'.  The deleted task is returned to the dormant state and can be re-activated by
*              creating the deleted task again.
*
* Arguments  : p_tcb      is the TCB of the tack to delete
*
*              p_err      is a pointer to an error code returned by this function:
*
*                             OS_ERR_NONE                    If the call is successful
*                             OS_ERR_ILLEGAL_DEL_RUN_TIME    If you are trying to delete the task after you called
*                                                              OSStart()
*                             OS_ERR_OS_NOT_RUNNING          If uC/OS-III is not running yet
*                             OS_ERR_STATE_INVALID           If the state of the task is invalid
*                             OS_ERR_TASK_DEL_IDLE           If you attempted to delete uC/OS-III's idle task
*                             OS_ERR_TASK_DEL_INVALID        If you attempted to delete uC/OS-III's ISR handler task
*                             OS_ERR_TASK_DEL_ISR            If you tried to delete a task from an ISR
*
* Returns    : none
*
* Note(s)    : 1) 'p_err' gets set to OS_ERR_NONE before OSSched() to allow the returned err or code to be monitored even
*                 for a task that is deleting itself. In this case, 'p_err' MUST point to a global variable that can be
*                 accessed by another task.
************************************************************************************************************************
*/

#if (OS_CFG_TASK_DEL_EN > 0u)
void  OSTaskDel (OS_TCB  *p_tcb,
                 OS_ERR  *p_err)
{
#if (OS_CFG_MUTEX_EN > 0u)
    OS_TCB   *p_tcb_owner;
    OS_PRIO   prio_new;
#endif
    CPU_SR_ALLOC();


#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return;
    }
#endif

#ifdef OS_SAFETY_CRITICAL_IEC61508
    if (OSSafetyCriticalStartFlag == OS_TRUE) {
       *p_err = OS_ERR_ILLEGAL_DEL_RUN_TIME;
        return;
    }
#endif

#if (OS_CFG_CALLED_FROM_ISR_CHK_EN > 0u)
    if (OSIntNestingCtr > 0u) {                                 /* See if trying to delete from ISR                     */
       *p_err = OS_ERR_TASK_DEL_ISR;
        return;
    }
#endif

#if (OS_CFG_INVALID_OS_CALLS_CHK_EN > 0u)
    if (OSRunning != OS_STATE_OS_RUNNING) {                     /* Is the kernel running?                               */
       *p_err = OS_ERR_OS_NOT_RUNNING;
        return;
    }
#endif

#if (OS_CFG_TASK_IDLE_EN > 0u)
    if (p_tcb == &OSIdleTaskTCB) {                              /* Not allowed to delete the idle task                  */
       *p_err = OS_ERR_TASK_DEL_IDLE;
        return;
    }
#endif

    if (p_tcb == (OS_TCB *)0) {                                 /* Delete 'Self'?                                       */
        CPU_CRITICAL_ENTER();
        p_tcb  = OSTCBCurPtr;                                   /* Yes.                                                 */
        CPU_CRITICAL_EXIT();
    }

    CPU_CRITICAL_ENTER();
    switch (p_tcb->TaskState) {
        case OS_TASK_STATE_RDY:
             OS_RdyListRemove(p_tcb);
             break;

        case OS_TASK_STATE_SUSPENDED:
             break;

        case OS_TASK_STATE_DLY:                                 /* Task is only delayed, not on any wait list           */
        case OS_TASK_STATE_DLY_SUSPENDED:
#if (OS_CFG_TICK_EN > 0u)
             OS_TickListRemove(p_tcb);
#endif
             break;

        case OS_TASK_STATE_PEND:
        case OS_TASK_STATE_PEND_SUSPENDED:
        case OS_TASK_STATE_PEND_TIMEOUT:
        case OS_TASK_STATE_PEND_TIMEOUT_SUSPENDED:
             switch (p_tcb->PendOn) {                           /* See what we are pending on                           */
                 case OS_TASK_PEND_ON_NOTHING:
                 case OS_TASK_PEND_ON_TASK_Q:                   /* There is no wait list for these two                  */
                 case OS_TASK_PEND_ON_TASK_SEM:
                      break;

                 case OS_TASK_PEND_ON_FLAG:                     /* Remove from pend list                                */
                 case OS_TASK_PEND_ON_Q:
                 case OS_TASK_PEND_ON_SEM:
                      OS_PendListRemove(p_tcb);
                      break;

#if (OS_CFG_MUTEX_EN > 0u)
                 case OS_TASK_PEND_ON_MUTEX:
                      p_tcb_owner = ((OS_MUTEX *)((void *)p_tcb->PendObjPtr))->OwnerTCBPtr;
                      prio_new = p_tcb_owner->Prio;
                      OS_PendListRemove(p_tcb);
                      if ((p_tcb_owner->Prio != p_tcb_owner->BasePrio) &&
                          (p_tcb_owner->Prio == p_tcb->Prio)) { /* Has the owner inherited a priority?                  */
                          prio_new = OS_MutexGrpPrioFindHighest(p_tcb_owner);
                          prio_new = (prio_new > p_tcb_owner->BasePrio) ? p_tcb_owner->BasePrio : prio_new;
                      }
                      p_tcb->PendOn = OS_TASK_PEND_ON_NOTHING;

                      if (prio_new != p_tcb_owner->Prio) {
                          OS_TaskChangePrio(p_tcb_owner, prio_new);
                          OS_TRACE_MUTEX_TASK_PRIO_DISINHERIT(p_tcb_owner, p_tcb_owner->Prio);
                      }
                      break;
#endif

                 default:
                                                                /* Default case.                                        */
                      break;
             }
#if (OS_CFG_TICK_EN > 0u)
             if ((p_tcb->TaskState == OS_TASK_STATE_PEND_TIMEOUT) ||
                 (p_tcb->TaskState == OS_TASK_STATE_PEND_TIMEOUT_SUSPENDED)) {
                 OS_TickListRemove(p_tcb);
             }
#endif
             break;

        default:
            CPU_CRITICAL_EXIT();
           *p_err = OS_ERR_STATE_INVALID;
            return;
    }

#if (OS_CFG_MUTEX_EN > 0u)
    if(p_tcb->MutexGrpHeadPtr != (OS_MUTEX *)0) {
        OS_MutexGrpPostAll(p_tcb);
    }
#endif

#if (OS_CFG_TASK_Q_EN > 0u)
    (void)OS_MsgQFreeAll(&p_tcb->MsgQ);                         /* Free task's message queue messages                   */
#endif

    OSTaskDelHook(p_tcb);                                       /* Call user defined hook                               */

#if defined(OS_CFG_TLS_TBL_SIZE) && (OS_CFG_TLS_TBL_SIZE > 0u)
    OS_TLS_TaskDel(p_tcb);                                      /* Call TLS hook                                        */
#endif

#if (OS_CFG_DBG_EN > 0u)
    OS_TaskDbgListRemove(p_tcb);
#endif

    OSTaskQty--;                                                /* One less task being managed                          */

    OS_TRACE_TASK_DEL(p_tcb);

#if (OS_CFG_TASK_STK_REDZONE_EN == 0u)                          /* Don't clear the TCB before checking the red-zone     */
    OS_TaskInitTCB(p_tcb);                                      /* Initialize the TCB to default values                 */
#endif
    p_tcb->TaskState = (OS_STATE)OS_TASK_STATE_DEL;             /* Indicate that the task was deleted                   */

   *p_err = OS_ERR_NONE;                                        /* See Note #1.                                         */
    CPU_CRITICAL_EXIT();

    OSSched();                                                  /* Find new highest priority task                       */
}
#endif


/*
************************************************************************************************************************
*                                                    FLUSH TASK's QUEUE
*
* Description: This function is used to flush the task's internal message queue.
*
* Arguments  : p_tcb       is a pointer to the task's OS_TCB.  Specifying a NULL pointer indicates that you wish to
*                          flush the message queue of the calling task.
*
*              p_err       is a pointer to a variable that will contain an error code returned by this function.
*
*                              OS_ERR_NONE              Upon success
*                              OS_ERR_FLUSH_ISR         If you called this function from an ISR
*                              OS_ERR_OS_NOT_RUNNING    If uC/OS-III is not running yet
*
* Returns     : The number of entries freed from the queue
*
* Note(s)     : 1) You should use this function with great care because, when to flush the queue, you LOOSE the
*                  references to what the queue entries are pointing to and thus, you could cause 'memory leaks'.  In
*                  other words, the data you are pointing to that's being referenced by the queue entries should, most
*                  likely, need to be de-allocated (i.e. freed).
************************************************************************************************************************
*/

#if (OS_CFG_TASK_Q_EN > 0u)
OS_MSG_QTY  OSTaskQFlush (OS_TCB  *p_tcb,
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

    if (p_tcb == (OS_TCB *)0) {                                 /* Flush message queue of calling task?                 */
        CPU_CRITICAL_ENTER();
        p_tcb = OSTCBCurPtr;
        CPU_CRITICAL_EXIT();
    }

    CPU_CRITICAL_ENTER();
    entries = OS_MsgQFreeAll(&p_tcb->MsgQ);                     /* Return all OS_MSGs to the OS_MSG pool                */
    CPU_CRITICAL_EXIT();
   *p_err   = OS_ERR_NONE;
    return (entries);
}
#endif


/*
************************************************************************************************************************
*                                                  WAIT FOR A MESSAGE
*
* Description: This function causes the current task to wait for a message to be posted to it.
*
* Arguments  : timeout       is an optional timeout period (in clock ticks).  If non-zero, your task will wait for a
*                            message to arrive up to the amount of time specified by this argument.
*                            If you specify 0, however, your task will wait forever or, until a message arrives.
*
*              opt           determines whether the user wants to block if the task's queue is empty or not:
*
*                                OS_OPT_PEND_BLOCKING
*                                OS_OPT_PEND_NON_BLOCKING
*
*              p_msg_size    is a pointer to a variable that will receive the size of the message
*
*              p_ts          is a pointer to a variable that will receive the timestamp of when the message was
*                            received.  If you pass a NULL pointer (i.e. (CPU_TS *)0) then you will not get the
*                            timestamp.  In other words, passing a NULL pointer is valid and indicates that you don't
*                            need the timestamp.
*
*              p_err         is a pointer to where an error message will be deposited.  Possible error
*                            messages are:
*
*                                OS_ERR_NONE               The call was successful and your task received a message.
*                                OS_ERR_OPT_INVALID        If you specified an invalid option
*                                OS_ERR_OS_NOT_RUNNING     If uC/OS-III is not running yet
*                                OS_ERR_PEND_ABORT         If the pend was aborted
*                                OS_ERR_PEND_ISR           If you called this function from an ISR and the result
*                                OS_ERR_PEND_WOULD_BLOCK   If you specified non-blocking but the queue was not empty
*                                OS_ERR_PTR_INVALID        If 'p_msg_size' is NULL
*                                OS_ERR_SCHED_LOCKED       If the scheduler is locked
*                                OS_ERR_TIMEOUT            A message was not received within the specified timeout
*                                                          would lead to a suspension
*                                OS_ERR_TICK_DISABLED      If kernel ticks are disabled and a timeout is specified
*
* Returns    : A pointer to the message received or a NULL pointer upon error.
*
* Note(s)    : 1) It is possible to receive NULL pointers when there are no errors.
*
*            : 2) This API 'MUST NOT' be called from a timer callback function.
************************************************************************************************************************
*/

#if (OS_CFG_TASK_Q_EN > 0u)
void  *OSTaskQPend (OS_TICK       timeout,
                    OS_OPT        opt,
                    OS_MSG_SIZE  *p_msg_size,
                    CPU_TS       *p_ts,
                    OS_ERR       *p_err)
{
    OS_MSG_Q  *p_msg_q;
    void      *p_void;
    CPU_SR_ALLOC();


#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return ((void *)0);
    }
#endif

    OS_TRACE_TASK_MSG_Q_PEND_ENTER(&OSTCBCurPtr->MsgQ, timeout, opt, p_msg_size, p_ts);

#if (OS_CFG_TICK_EN == 0u)
    if (timeout != 0u) {
       *p_err = OS_ERR_TICK_DISABLED;
        OS_TRACE_TASK_MSG_Q_PEND_EXIT(OS_ERR_TICK_DISABLED);
        return ((void *)0);
    }
#endif

#if (OS_CFG_CALLED_FROM_ISR_CHK_EN > 0u)
    if (OSIntNestingCtr > 0u) {                                 /* Can't Pend from an ISR                               */
        OS_TRACE_TASK_MSG_Q_PEND_EXIT(OS_ERR_PEND_ISR);
       *p_err = OS_ERR_PEND_ISR;
        return ((void *)0);
    }
#endif

#if (OS_CFG_INVALID_OS_CALLS_CHK_EN > 0u)
    if (OSRunning != OS_STATE_OS_RUNNING) {                     /* Is the kernel running?                               */
        OS_TRACE_TASK_MSG_Q_PEND_EXIT(OS_ERR_OS_NOT_RUNNING);
       *p_err = OS_ERR_OS_NOT_RUNNING;
        return ((void *)0);
    }
#endif


#if (OS_CFG_ARG_CHK_EN > 0u)                                    /* ---------------- VALIDATE ARGUMENTS ---------------- */
    if (p_msg_size == (OS_MSG_SIZE *)0) {                       /* User must supply a valid destination for msg size    */
        OS_TRACE_TASK_MSG_Q_PEND_EXIT(OS_ERR_PTR_INVALID);
       *p_err = OS_ERR_PTR_INVALID;
        return ((void *)0);
    }
    switch (opt) {                                              /* User must supply a valid option                      */
        case OS_OPT_PEND_BLOCKING:
        case OS_OPT_PEND_NON_BLOCKING:
             break;

        default:
             OS_TRACE_TASK_MSG_Q_PEND_EXIT(OS_ERR_OPT_INVALID);
            *p_err = OS_ERR_OPT_INVALID;
             return ((void *)0);
    }
#endif

    if (p_ts != (CPU_TS *)0) {
       *p_ts = 0u;                                              /* Initialize the returned timestamp                    */
    }

    CPU_CRITICAL_ENTER();
    p_msg_q = &OSTCBCurPtr->MsgQ;                               /* Any message waiting in the message queue?            */
    p_void  = OS_MsgQGet(p_msg_q,
                         p_msg_size,
                         p_ts,
                         p_err);
    if (*p_err == OS_ERR_NONE) {
#if (OS_CFG_TASK_PROFILE_EN > 0u)
#if (OS_CFG_TS_EN > 0u)
        if (p_ts != (CPU_TS *)0) {
            OSTCBCurPtr->MsgQPendTime = OS_TS_GET() - *p_ts;
            if (OSTCBCurPtr->MsgQPendTimeMax < OSTCBCurPtr->MsgQPendTime) {
                OSTCBCurPtr->MsgQPendTimeMax = OSTCBCurPtr->MsgQPendTime;
            }
        }
#endif
#endif
        OS_TRACE_TASK_MSG_Q_PEND(p_msg_q);
        CPU_CRITICAL_EXIT();
        OS_TRACE_TASK_MSG_Q_PEND_EXIT(OS_ERR_NONE);
        return (p_void);                                        /* Yes, Return oldest message received                  */
    }

    if ((opt & OS_OPT_PEND_NON_BLOCKING) != 0u) {               /* Caller wants to block if not available?              */
       *p_err = OS_ERR_PEND_WOULD_BLOCK;                        /* No                                                   */
        CPU_CRITICAL_EXIT();
        OS_TRACE_TASK_MSG_Q_PEND_FAILED(p_msg_q);
        OS_TRACE_TASK_MSG_Q_PEND_EXIT(OS_ERR_PEND_WOULD_BLOCK);
        return ((void *)0);
    } else {                                                    /* Yes                                                  */
        if (OSSchedLockNestingCtr > 0u) {                       /* Can't block when the scheduler is locked             */
            CPU_CRITICAL_EXIT();
            OS_TRACE_TASK_MSG_Q_PEND_FAILED(p_msg_q);
            OS_TRACE_TASK_MSG_Q_PEND_EXIT(OS_ERR_SCHED_LOCKED);
           *p_err = OS_ERR_SCHED_LOCKED;
            return ((void *)0);
        }
    }

    OS_Pend((OS_PEND_OBJ *)0,                                   /* Block task pending on Message                        */
             OSTCBCurPtr,
             OS_TASK_PEND_ON_TASK_Q,
             timeout);
    CPU_CRITICAL_EXIT();
    OS_TRACE_TASK_MSG_Q_PEND_BLOCK(p_msg_q);
    OSSched();                                                  /* Find the next highest priority task ready to run     */

    CPU_CRITICAL_ENTER();
    switch (OSTCBCurPtr->PendStatus) {
        case OS_STATUS_PEND_OK:                                 /* Extract message from TCB (Put there by Post)         */
             p_void      = OSTCBCurPtr->MsgPtr;
            *p_msg_size  = OSTCBCurPtr->MsgSize;
#if (OS_CFG_TS_EN > 0u)
             if (p_ts != (CPU_TS *)0) {
                *p_ts = OSTCBCurPtr->TS;
#if (OS_CFG_TASK_PROFILE_EN > 0u)
                OSTCBCurPtr->MsgQPendTime = OS_TS_GET() - OSTCBCurPtr->TS;
                if (OSTCBCurPtr->MsgQPendTimeMax < OSTCBCurPtr->MsgQPendTime) {
                    OSTCBCurPtr->MsgQPendTimeMax = OSTCBCurPtr->MsgQPendTime;
                }
#endif
             }
#endif
             OS_TRACE_TASK_MSG_Q_PEND(p_msg_q);
            *p_err = OS_ERR_NONE;
             break;

        case OS_STATUS_PEND_ABORT:                              /* Indicate that we aborted                             */
             p_void     = (void *)0;
            *p_msg_size = 0u;
             if (p_ts != (CPU_TS *)0) {
                *p_ts = 0u;
             }
             OS_TRACE_TASK_MSG_Q_PEND_FAILED(p_msg_q);
            *p_err      =  OS_ERR_PEND_ABORT;
             break;

        case OS_STATUS_PEND_TIMEOUT:                            /* Indicate that we didn't get event within TO          */
        default:
             p_void     = (void *)0;
            *p_msg_size = 0u;
#if (OS_CFG_TS_EN > 0u)
             if (p_ts  != (CPU_TS *)0) {
                *p_ts = OSTCBCurPtr->TS;
             }
#endif
             OS_TRACE_TASK_MSG_Q_PEND_FAILED(p_msg_q);
            *p_err      =  OS_ERR_TIMEOUT;
             break;
    }
    CPU_CRITICAL_EXIT();
    OS_TRACE_TASK_MSG_Q_PEND_EXIT(*p_err);
    return (p_void);                                            /* Return received message                              */
}
#endif


/*
************************************************************************************************************************
*                                              ABORT WAITING FOR A MESSAGE
*
* Description: This function aborts & readies the task specified.  This function should be used to fault-abort the wait
*              for a message, rather than to normally post the message to the task via OSTaskQPost().
*
* Arguments  : p_tcb     is a pointer to the task to pend abort
*
*              opt       provides options for this function:
*
*                            OS_OPT_POST_NONE         No option specified
*                            OS_OPT_POST_NO_SCHED     Indicates that the scheduler will not be called.
*
*              p_err     is a pointer to a variable that will contain an error code returned by this function.
*
*                            OS_ERR_NONE               If the task was readied and informed of the aborted wait
*                            OS_ERR_OPT_INVALID        If you specified an invalid option
*                            OS_ERR_OS_NOT_RUNNING     If uC/OS-III is not running yet
*                            OS_ERR_PEND_ABORT_ISR     If you called this function from an ISR
*                            OS_ERR_PEND_ABORT_NONE    If task was not pending on a message and thus there is nothing to
*                                                      abort
*                            OS_ERR_PEND_ABORT_SELF    If you passed a NULL pointer for 'p_tcb'
*
* Returns    : == OS_FALSE   if task was not waiting for a message, or upon error.
*              == OS_TRUE    if task was waiting for a message and was readied and informed.
*
* Note(s)    : none
************************************************************************************************************************
*/

#if (OS_CFG_TASK_Q_EN > 0u) && (OS_CFG_TASK_Q_PEND_ABORT_EN > 0u)
CPU_BOOLEAN  OSTaskQPendAbort (OS_TCB  *p_tcb,
                               OS_OPT   opt,
                               OS_ERR  *p_err)
{
    CPU_TS  ts;
    CPU_SR_ALLOC();


#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return (OS_FALSE);
    }
#endif

#if (OS_CFG_CALLED_FROM_ISR_CHK_EN > 0u)
    if (OSIntNestingCtr > 0u) {                                 /* See if called from ISR ...                           */
       *p_err = OS_ERR_PEND_ABORT_ISR;                          /* ... can't Pend Abort from an ISR                     */
        return (OS_FALSE);
    }
#endif

#if (OS_CFG_INVALID_OS_CALLS_CHK_EN > 0u)
    if (OSRunning != OS_STATE_OS_RUNNING) {                     /* Is the kernel running?                               */
       *p_err = OS_ERR_OS_NOT_RUNNING;
        return (OS_FALSE);
    }
#endif

#if (OS_CFG_ARG_CHK_EN > 0u)                                    /* ---------------- VALIDATE ARGUMENTS ---------------- */
    switch (opt) {                                              /* User must supply a valid option                      */
        case OS_OPT_POST_NONE:
        case OS_OPT_POST_NO_SCHED:
             break;

        default:
            *p_err = OS_ERR_OPT_INVALID;
             return (OS_FALSE);
    }
#endif

    CPU_CRITICAL_ENTER();
#if (OS_CFG_ARG_CHK_EN > 0u)
    if ((p_tcb == (OS_TCB *)0) ||                               /* Pend abort self?                                     */
        (p_tcb == OSTCBCurPtr)) {
        CPU_CRITICAL_EXIT();
       *p_err = OS_ERR_PEND_ABORT_SELF;                         /* ... doesn't make sense                               */
        return (OS_FALSE);
    }
#endif

    if (p_tcb->PendOn != OS_TASK_PEND_ON_TASK_Q) {              /* Is task waiting for a message?                       */
        CPU_CRITICAL_EXIT();                                    /* No                                                   */
       *p_err = OS_ERR_PEND_ABORT_NONE;
        return (OS_FALSE);
    }

#if (OS_CFG_TS_EN > 0u)
    ts = OS_TS_GET();                                           /* Get timestamp of when the abort occurred             */
#else
    ts = 0u;
#endif
    OS_PendAbort(p_tcb,                                         /* Abort the pend                                       */
                 ts,
                 OS_STATUS_PEND_ABORT);
    CPU_CRITICAL_EXIT();
    if ((opt & OS_OPT_POST_NO_SCHED) == 0u) {
        OSSched();                                              /* Run the scheduler                                    */
    }
   *p_err = OS_ERR_NONE;
    return (OS_TRUE);
}
#endif


/*
************************************************************************************************************************
*                                               POST MESSAGE TO A TASK
*
* Description: This function sends a message to a task.
*
* Arguments  : p_tcb      is a pointer to the TCB of the task receiving a message.  If you specify a NULL pointer then
*                         the message will be posted to the task's queue of the calling task.  In other words, you'd be
*                         posting a message to yourself.
*
*              p_void     is a pointer to the message to send.
*
*              msg_size   is the size of the message sent (in bytes)
*
*              opt        specifies whether the post will be FIFO or LIFO:
*
*                             OS_OPT_POST_FIFO       Post at the end   of the queue
*                             OS_OPT_POST_LIFO       Post at the front of the queue
*
*                             OS_OPT_POST_NO_SCHED   Do not run the scheduler after the post
*
*                          Note(s): 1) OS_OPT_POST_NO_SCHED can be added with one of the other options.
*
*
*              p_err      is a pointer to a variable that will hold the error code associated
*                         with the outcome of this call.  Errors can be:
*
*                             OS_ERR_NONE              The call was successful and the message was sent
*                             OS_ERR_MSG_POOL_EMPTY    If there are no more OS_MSGs available from the pool
*                             OS_ERR_OPT_INVALID       If you specified an invalid option
*                             OS_ERR_OS_NOT_RUNNING    If uC/OS-III is not running yet
*                             OS_ERR_Q_MAX             If the queue is full
*                             OS_ERR_STATE_INVALID     If the task is in an invalid state.  This should never happen
*                                                      and if it does, would be considered a system failure
*
* Returns    : none
*
* Note(s)    : none
************************************************************************************************************************
*/

#if (OS_CFG_TASK_Q_EN > 0u)
void  OSTaskQPost (OS_TCB       *p_tcb,
                   void         *p_void,
                   OS_MSG_SIZE   msg_size,
                   OS_OPT        opt,
                   OS_ERR       *p_err)
{
    CPU_TS  ts;
    CPU_SR_ALLOC();


#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return;
    }
#endif

    OS_TRACE_TASK_MSG_Q_POST_ENTER(&p_tcb->MsgQ, p_void, msg_size, opt);

#if (OS_CFG_INVALID_OS_CALLS_CHK_EN > 0u)
    if (OSRunning != OS_STATE_OS_RUNNING) {                     /* Is the kernel running?                               */
        OS_TRACE_TASK_MSG_Q_POST_EXIT(OS_ERR_OS_NOT_RUNNING);
       *p_err = OS_ERR_OS_NOT_RUNNING;
        return;
    }
#endif

#if (OS_CFG_ARG_CHK_EN > 0u)                                    /* ---------------- VALIDATE ARGUMENTS ---------------- */
    switch (opt) {                                              /* User must supply a valid option                      */
        case OS_OPT_POST_FIFO:
        case OS_OPT_POST_LIFO:
        case OS_OPT_POST_FIFO | OS_OPT_POST_NO_SCHED:
        case OS_OPT_POST_LIFO | OS_OPT_POST_NO_SCHED:
             break;

        default:
             OS_TRACE_TASK_MSG_Q_POST_FAILED(&p_tcb->MsgQ);
             OS_TRACE_TASK_MSG_Q_POST_EXIT(OS_ERR_OPT_INVALID);
            *p_err = OS_ERR_OPT_INVALID;
             return;
    }
#endif

#if (OS_CFG_TS_EN > 0u)
    ts = OS_TS_GET();                                           /* Get timestamp                                        */
#else
    ts = 0u;
#endif

    OS_TRACE_TASK_MSG_Q_POST(&p_tcb->MsgQ);

   *p_err = OS_ERR_NONE;                                        /* Assume we won't have any errors                      */
    CPU_CRITICAL_ENTER();
    if (p_tcb == (OS_TCB *)0) {                                 /* Post msg to 'self'?                                  */
        p_tcb = OSTCBCurPtr;
    }
    switch (p_tcb->TaskState) {
        case OS_TASK_STATE_RDY:
        case OS_TASK_STATE_DLY:
        case OS_TASK_STATE_SUSPENDED:
        case OS_TASK_STATE_DLY_SUSPENDED:
             OS_MsgQPut(&p_tcb->MsgQ,                           /* Deposit the message in the queue                     */
                        p_void,
                        msg_size,
                        opt,
                        ts,
                        p_err);
             CPU_CRITICAL_EXIT();
             break;

        case OS_TASK_STATE_PEND:
        case OS_TASK_STATE_PEND_TIMEOUT:
        case OS_TASK_STATE_PEND_SUSPENDED:
        case OS_TASK_STATE_PEND_TIMEOUT_SUSPENDED:
             if (p_tcb->PendOn == OS_TASK_PEND_ON_TASK_Q) {     /* Is task waiting for a message to be sent to it?      */
                 OS_Post((OS_PEND_OBJ *)0,
                          p_tcb,
                          p_void,
                          msg_size,
                          ts);
                 CPU_CRITICAL_EXIT();
                 if ((opt & OS_OPT_POST_NO_SCHED) == 0u) {
                     OSSched();                                 /* Run the scheduler                                    */
                 }
             } else {
                 OS_MsgQPut(&p_tcb->MsgQ,                       /* No,  Task is pending on something else ...           */
                            p_void,                             /* ... Deposit the message in the task's queue          */
                            msg_size,
                            opt,
                            ts,
                            p_err);
                 CPU_CRITICAL_EXIT();
             }
             break;

        default:
             CPU_CRITICAL_EXIT();
            *p_err = OS_ERR_STATE_INVALID;
             break;
    }

    OS_TRACE_TASK_MSG_Q_POST_EXIT(*p_err);
}
#endif


/*
************************************************************************************************************************
*                                       GET THE CURRENT VALUE OF A TASK REGISTER
*
* Description: This function is called to obtain the current value of a task register.  Task registers are application
*              specific and can be used to store task specific values such as 'error numbers' (i.e. errno), statistics,
*              etc.
*
* Arguments  : p_tcb     is a pointer to the OS_TCB of the task you want to read the register from.  If 'p_tcb' is a
*                        NULL pointer then you will get the register of the current task.
*
*              id        is the 'id' of the desired task variable.  Note that the 'id' must be less than
*                        OS_CFG_TASK_REG_TBL_SIZE
*
*              p_err     is a pointer to a variable that will hold an error code related to this call.
*
*                            OS_ERR_NONE               If the call was successful
*                            OS_ERR_REG_ID_INVALID     If the 'id' is not between 0 and OS_CFG_TASK_REG_TBL_SIZE-1
*
* Returns    : The current value of the task's register or 0 if an error is detected.
*
* Note(s)    : none
************************************************************************************************************************
*/

#if (OS_CFG_TASK_REG_TBL_SIZE > 0u)
OS_REG  OSTaskRegGet (OS_TCB     *p_tcb,
                      OS_REG_ID   id,
                      OS_ERR     *p_err)
{
    OS_REG     value;
    CPU_SR_ALLOC();



#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return (0u);
    }
#endif

#if (OS_CFG_ARG_CHK_EN > 0u)
    if (id >= OS_CFG_TASK_REG_TBL_SIZE) {
       *p_err = OS_ERR_REG_ID_INVALID;
        return (0u);
    }
#endif

    CPU_CRITICAL_ENTER();
    if (p_tcb == (OS_TCB *)0) {
        p_tcb = OSTCBCurPtr;
    }
    value = p_tcb->RegTbl[id];
    CPU_CRITICAL_EXIT();
   *p_err = OS_ERR_NONE;
    return (value);
}
#endif


/*
************************************************************************************************************************
*                                    ALLOCATE THE NEXT AVAILABLE TASK REGISTER ID
*
* Description: This function is called to obtain a task register ID.  This function thus allows task registers IDs to be
*              allocated dynamically instead of statically.
*
* Arguments  : p_err       is a pointer to a variable that will hold an error code related to this call.
*
*                            OS_ERR_NONE               If the call was successful
*                            OS_ERR_NO_MORE_ID_AVAIL   If you are attempting to assign more task register IDs than you
*                                                          have available through OS_CFG_TASK_REG_TBL_SIZE
*
* Returns    : The next available task register 'id' or OS_CFG_TASK_REG_TBL_SIZE if an error is detected.
*
* Note(s)    : none
************************************************************************************************************************
*/

#if (OS_CFG_TASK_REG_TBL_SIZE > 0u)
OS_REG_ID  OSTaskRegGetID (OS_ERR  *p_err)
{
    OS_REG_ID  id;
    CPU_SR_ALLOC();



#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return ((OS_REG_ID)OS_CFG_TASK_REG_TBL_SIZE);
    }
#endif

    CPU_CRITICAL_ENTER();
    if (OSTaskRegNextAvailID >= OS_CFG_TASK_REG_TBL_SIZE) {     /* See if we exceeded the number of IDs available       */
       *p_err = OS_ERR_NO_MORE_ID_AVAIL;                        /* Yes, cannot allocate more task register IDs          */
        CPU_CRITICAL_EXIT();
        return (OS_CFG_TASK_REG_TBL_SIZE);
    }

    id = OSTaskRegNextAvailID;                                  /* Assign the next available ID                         */
    OSTaskRegNextAvailID++;                                     /* Increment available ID for next request              */
    CPU_CRITICAL_EXIT();
   *p_err = OS_ERR_NONE;
    return (id);
}
#endif


/*
************************************************************************************************************************
*                                       SET THE CURRENT VALUE OF A TASK REGISTER
*
* Description: This function is called to change the current value of a task register.  Task registers are application
*              specific and can be used to store task specific values such as 'error numbers' (i.e. errno), statistics,
*              etc.
*
* Arguments  : p_tcb     is a pointer to the OS_TCB of the task you want to set the register for.  If 'p_tcb' is a NULL
*                        pointer then you will change the register of the current task.
*
*              id        is the 'id' of the desired task register.  Note that the 'id' must be less than
*                        OS_CFG_TASK_REG_TBL_SIZE
*
*              value     is the desired value for the task register.
*
*              p_err     is a pointer to a variable that will hold an error code related to this call.
*
*                            OS_ERR_NONE               If the call was successful
*                            OS_ERR_REG_ID_INVALID     If the 'id' is not between 0 and OS_CFG_TASK_REG_TBL_SIZE-1
*
* Returns    : none
*
* Note(s)    : none
************************************************************************************************************************
*/

#if (OS_CFG_TASK_REG_TBL_SIZE > 0u)
void  OSTaskRegSet (OS_TCB     *p_tcb,
                    OS_REG_ID   id,
                    OS_REG      value,
                    OS_ERR     *p_err)
{
    CPU_SR_ALLOC();



#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return;
    }
#endif

#if (OS_CFG_ARG_CHK_EN > 0u)
    if (id >= OS_CFG_TASK_REG_TBL_SIZE) {
       *p_err = OS_ERR_REG_ID_INVALID;
        return;
    }
#endif

    CPU_CRITICAL_ENTER();
    if (p_tcb == (OS_TCB *)0) {
        p_tcb = OSTCBCurPtr;
    }
    p_tcb->RegTbl[id] = value;
    CPU_CRITICAL_EXIT();
   *p_err             = OS_ERR_NONE;
}
#endif


/*
************************************************************************************************************************
*                                               RESUME A SUSPENDED TASK
*
* Description: This function is called to resume a previously suspended task.  This is the only call that will remove an
*              explicit task suspension.
*
* Arguments  : p_tcb      Is a pointer to the task's OS_TCB to resume
*
*              p_err      Is a pointer to a variable that will contain an error code returned by this function
*
*                             OS_ERR_NONE                  If the requested task is resumed
*                             OS_ERR_OS_NOT_RUNNING        If uC/OS-III is not running yet
*                             OS_ERR_STATE_INVALID         If the task is in an invalid state
*                             OS_ERR_TASK_NOT_SUSPENDED    If the task to resume has not been suspended
*                             OS_ERR_TASK_RESUME_ISR       If you called this function from an ISR
*                             OS_ERR_TASK_RESUME_SELF      You cannot resume 'self'
*
* Returns    : none
*
* Note(s)    : none
************************************************************************************************************************
*/

#if (OS_CFG_TASK_SUSPEND_EN > 0u)
void  OSTaskResume (OS_TCB  *p_tcb,
                    OS_ERR  *p_err)
{
    CPU_SR_ALLOC();


    OS_TRACE_TASK_RESUME_ENTER(p_tcb);

#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return;
    }
#endif

#if (OS_CFG_CALLED_FROM_ISR_CHK_EN > 0u)
    if (OSIntNestingCtr > 0u) {                                 /* Not allowed to call from an ISR                      */
       *p_err = OS_ERR_TASK_RESUME_ISR;
        OS_TRACE_TASK_RESUME_EXIT(OS_ERR_TASK_RESUME_ISR);
        return;
    }
#endif

#if (OS_CFG_INVALID_OS_CALLS_CHK_EN > 0u)
    if (OSRunning != OS_STATE_OS_RUNNING) {                     /* Is the kernel running?                               */
       *p_err = OS_ERR_OS_NOT_RUNNING;
        OS_TRACE_TASK_RESUME_EXIT(OS_ERR_OS_NOT_RUNNING);
        return;
    }
#endif


#if (OS_CFG_ARG_CHK_EN > 0u)
    CPU_CRITICAL_ENTER();
    if ((p_tcb == (OS_TCB *)0) ||                               /* We cannot resume 'self'                              */
        (p_tcb == OSTCBCurPtr)) {
        CPU_CRITICAL_EXIT();
       *p_err = OS_ERR_TASK_RESUME_SELF;
        OS_TRACE_TASK_RESUME_EXIT(OS_ERR_TASK_RESUME_SELF);
        return;
    }
    CPU_CRITICAL_EXIT();
#endif

    CPU_CRITICAL_ENTER();
   *p_err = OS_ERR_NONE;
    switch (p_tcb->TaskState) {
        case OS_TASK_STATE_RDY:
        case OS_TASK_STATE_DLY:
        case OS_TASK_STATE_PEND:
        case OS_TASK_STATE_PEND_TIMEOUT:
             CPU_CRITICAL_EXIT();
            *p_err = OS_ERR_TASK_NOT_SUSPENDED;
             OS_TRACE_TASK_RESUME_EXIT(OS_ERR_TASK_NOT_SUSPENDED);
             break;

        case OS_TASK_STATE_SUSPENDED:
             p_tcb->SuspendCtr--;
             if (p_tcb->SuspendCtr == 0u) {
                 p_tcb->TaskState = OS_TASK_STATE_RDY;
                 OS_RdyListInsert(p_tcb);                       /* Insert the task in the ready list                    */
                 OS_TRACE_TASK_RESUME(p_tcb);
             }
             CPU_CRITICAL_EXIT();
             break;

        case OS_TASK_STATE_DLY_SUSPENDED:
             p_tcb->SuspendCtr--;
             if (p_tcb->SuspendCtr == 0u) {
                 p_tcb->TaskState = OS_TASK_STATE_DLY;
             }
             CPU_CRITICAL_EXIT();
             break;

        case OS_TASK_STATE_PEND_SUSPENDED:
             p_tcb->SuspendCtr--;
             if (p_tcb->SuspendCtr == 0u) {
                 p_tcb->TaskState = OS_TASK_STATE_PEND;
             }
             CPU_CRITICAL_EXIT();
             break;

        case OS_TASK_STATE_PEND_TIMEOUT_SUSPENDED:
             p_tcb->SuspendCtr--;
             if (p_tcb->SuspendCtr == 0u) {
                 p_tcb->TaskState = OS_TASK_STATE_PEND_TIMEOUT;
             }
             CPU_CRITICAL_EXIT();
             break;

        default:
             CPU_CRITICAL_EXIT();
            *p_err = OS_ERR_STATE_INVALID;
             OS_TRACE_TASK_RESUME_EXIT(OS_ERR_STATE_INVALID);
             break;
    }

    if (*p_err != OS_ERR_NONE) {                                /* Don't schedule if task wasn't in a suspend state.    */
        return;
    }

    OSSched();
    OS_TRACE_TASK_RESUME_EXIT(OS_ERR_NONE);
}
#endif


/*
************************************************************************************************************************
*                                              WAIT FOR A TASK SEMAPHORE
*
* Description: This function is called to block the current task until a signal is sent by another task or ISR.
*
* Arguments  : timeout       is the amount of time you are will to wait for the signal
*
*              opt           determines whether the user wants to block if a semaphore post was not received:
*
*                                OS_OPT_PEND_BLOCKING
*                                OS_OPT_PEND_NON_BLOCKING
*
*              p_ts          is a pointer to a variable that will receive the timestamp of when the semaphore was posted
*                            or pend aborted.  If you pass a NULL pointer (i.e. (CPU_TS *)0) then you will not get the
*                            timestamp.  In other words, passing a NULL pointer is valid and indicates that you don't
*                            need the timestamp.
*
*              p_err         is a pointer to an error code that will be set by this function
*
*                                OS_ERR_NONE                The call was successful and your task received a message
*                                OS_ERR_OPT_INVALID         You specified an invalid option
*                                OS_ERR_OS_NOT_RUNNING      If uC/OS-III is not running yet
*                                OS_ERR_PEND_ABORT          If the pend was aborted
*                                OS_ERR_PEND_ISR            If you called this function from an ISR
*                                OS_ERR_PEND_WOULD_BLOCK    If you specified non-blocking but no signal was received
*                                OS_ERR_SCHED_LOCKED        If the scheduler is locked
*                                OS_ERR_STATUS_INVALID      If the pend status is invalid
*                                OS_ERR_TIMEOUT             A message was not received within the specified timeout
*
* Returns    : The current count of signals the task received, 0 if none.
*
* Note(s)    : This API 'MUST NOT' be called from a timer callback function.
************************************************************************************************************************
*/

OS_SEM_CTR  OSTaskSemPend (OS_TICK   timeout,
                           OS_OPT    opt,
                           CPU_TS   *p_ts,
                           OS_ERR   *p_err)
{
    OS_SEM_CTR    ctr;
    CPU_SR_ALLOC();


#if (OS_CFG_TS_EN == 0u)
    (void)p_ts;                                                 /* Prevent compiler warning for not using 'ts'          */
#endif

#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return (0u);
    }
#endif

    OS_TRACE_TASK_SEM_PEND_ENTER(OSTCBCurPtr, timeout, opt, p_ts);

#if (OS_CFG_TICK_EN == 0u)
    if (timeout != 0u) {
        OS_TRACE_TASK_SEM_PEND_FAILED(OSTCBCurPtr);
        OS_TRACE_TASK_SEM_PEND_EXIT(OS_ERR_TICK_DISABLED);
       *p_err = OS_ERR_TICK_DISABLED;
        return (0u);
    }
#endif

#if (OS_CFG_CALLED_FROM_ISR_CHK_EN > 0u)
    if (OSIntNestingCtr > 0u) {                                 /* Not allowed to call from an ISR                      */
        OS_TRACE_TASK_SEM_PEND_FAILED(OSTCBCurPtr);
        OS_TRACE_TASK_SEM_PEND_EXIT(OS_ERR_PEND_ISR);
       *p_err = OS_ERR_PEND_ISR;
        return (0u);
    }
#endif

#if (OS_CFG_INVALID_OS_CALLS_CHK_EN > 0u)
    if (OSRunning != OS_STATE_OS_RUNNING) {                     /* Is the kernel running?                               */
        OS_TRACE_TASK_SEM_PEND_EXIT(OS_ERR_OS_NOT_RUNNING);
       *p_err = OS_ERR_OS_NOT_RUNNING;
        return (0u);
    }
#endif

#if (OS_CFG_ARG_CHK_EN > 0u)
    switch (opt) {                                              /* Validate 'opt'                                       */
        case OS_OPT_PEND_BLOCKING:
        case OS_OPT_PEND_NON_BLOCKING:
             break;

        default:
             OS_TRACE_TASK_SEM_PEND_FAILED(OSTCBCurPtr);
             OS_TRACE_TASK_SEM_PEND_EXIT(OS_ERR_OPT_INVALID);
            *p_err = OS_ERR_OPT_INVALID;
             return (0u);
    }
#endif

    CPU_CRITICAL_ENTER();
    if (OSTCBCurPtr->SemCtr > 0u) {                             /* See if task already been signaled                    */
        OSTCBCurPtr->SemCtr--;
        ctr = OSTCBCurPtr->SemCtr;
#if (OS_CFG_TS_EN > 0u)
        if (p_ts != (CPU_TS *)0) {
           *p_ts  = OSTCBCurPtr->TS;
        }
#if (OS_CFG_TASK_PROFILE_EN > 0u)
#if (OS_CFG_TS_EN > 0u)
        OSTCBCurPtr->SemPendTime = OS_TS_GET() - OSTCBCurPtr->TS;
        if (OSTCBCurPtr->SemPendTimeMax < OSTCBCurPtr->SemPendTime) {
            OSTCBCurPtr->SemPendTimeMax = OSTCBCurPtr->SemPendTime;
        }
#endif
#endif
#endif
        OS_TRACE_TASK_SEM_PEND(OSTCBCurPtr);
        CPU_CRITICAL_EXIT();
        OS_TRACE_TASK_SEM_PEND_EXIT(OS_ERR_NONE);
       *p_err = OS_ERR_NONE;
        return (ctr);
    }

    if ((opt & OS_OPT_PEND_NON_BLOCKING) != 0u) {               /* Caller wants to block if not available?              */
        CPU_CRITICAL_EXIT();
#if (OS_CFG_TS_EN > 0u)
        if (p_ts != (CPU_TS *)0) {
            *p_ts  = 0u;
        }
#endif
        OS_TRACE_TASK_SEM_PEND_FAILED(OSTCBCurPtr);
        OS_TRACE_TASK_SEM_PEND_EXIT(OS_ERR_PEND_WOULD_BLOCK);
       *p_err = OS_ERR_PEND_WOULD_BLOCK;                        /* No                                                   */
        return (0u);
    } else {                                                    /* Yes                                                  */
        if (OSSchedLockNestingCtr > 0u) {                       /* Can't pend when the scheduler is locked              */
#if (OS_CFG_TS_EN > 0u)
            if (p_ts != (CPU_TS *)0) {
               *p_ts  = 0u;
            }
#endif
            CPU_CRITICAL_EXIT();
            OS_TRACE_TASK_SEM_PEND_FAILED(OSTCBCurPtr);
            OS_TRACE_TASK_SEM_PEND_EXIT(OS_ERR_SCHED_LOCKED);
           *p_err = OS_ERR_SCHED_LOCKED;
            return (0u);
        }
    }

    OS_Pend((OS_PEND_OBJ *)0,                                   /* Block task pending on Signal                         */
             OSTCBCurPtr,
             OS_TASK_PEND_ON_TASK_SEM,
             timeout);
    CPU_CRITICAL_EXIT();
    OS_TRACE_TASK_SEM_PEND_BLOCK(OSTCBCurPtr);
    OSSched();                                                  /* Find next highest priority task ready to run         */

    CPU_CRITICAL_ENTER();
    switch (OSTCBCurPtr->PendStatus) {                          /* See if we timed-out or aborted                       */
        case OS_STATUS_PEND_OK:
#if (OS_CFG_TS_EN > 0u)
             if (p_ts != (CPU_TS *)0) {
                *p_ts                    =  OSTCBCurPtr->TS;
#if (OS_CFG_TASK_PROFILE_EN > 0u)
#if (OS_CFG_TS_EN > 0u)
                OSTCBCurPtr->SemPendTime = OS_TS_GET() - OSTCBCurPtr->TS;
                if (OSTCBCurPtr->SemPendTimeMax < OSTCBCurPtr->SemPendTime) {
                    OSTCBCurPtr->SemPendTimeMax = OSTCBCurPtr->SemPendTime;
                }
#endif
#endif
             }
#endif
             OS_TRACE_TASK_SEM_PEND(OSTCBCurPtr);
            *p_err = OS_ERR_NONE;
             break;

        case OS_STATUS_PEND_ABORT:
#if (OS_CFG_TS_EN > 0u)
             if (p_ts != (CPU_TS *)0) {
                *p_ts = OSTCBCurPtr->TS;
             }
#endif
             OS_TRACE_TASK_SEM_PEND_FAILED(OSTCBCurPtr);
            *p_err = OS_ERR_PEND_ABORT;                         /* Indicate that we aborted                             */
             break;

        case OS_STATUS_PEND_TIMEOUT:
#if (OS_CFG_TS_EN > 0u)
             if (p_ts != (CPU_TS *)0) {
                *p_ts = 0u;
             }
#endif
             OS_TRACE_TASK_SEM_PEND_FAILED(OSTCBCurPtr);
            *p_err = OS_ERR_TIMEOUT;                            /* Indicate that we didn't get event within TO          */
             break;

        default:
             OS_TRACE_TASK_SEM_PEND_FAILED(OSTCBCurPtr);
            *p_err = OS_ERR_STATUS_INVALID;
             break;
    }
    ctr = OSTCBCurPtr->SemCtr;
    CPU_CRITICAL_EXIT();
    OS_TRACE_TASK_SEM_PEND_EXIT(*p_err);
    return (ctr);
}


/*
************************************************************************************************************************
*                                               ABORT WAITING FOR A SIGNAL
*
* Description: This function aborts & readies the task specified.  This function should be used to fault-abort the wait
*              for a signal, rather than to normally post the signal to the task via OSTaskSemPost().
*
* Arguments  : p_tcb     is a pointer to the task to pend abort
*
*              opt       provides options for this function:
*
*                            OS_OPT_POST_NONE         No option selected
*                            OS_OPT_POST_NO_SCHED     Indicates that the scheduler will not be called.
*
*              p_err     is a pointer to a variable that will contain an error code returned by this function.
*
*                            OS_ERR_NONE               If the task was readied and informed of the aborted wait
*                            OS_ERR_OPT_INVALID        You specified an invalid option
*                            OS_ERR_OS_NOT_RUNNING     If uC/OS-III is not running yet
*                            OS_ERR_PEND_ABORT_ISR     If you tried calling this function from an ISR
*                            OS_ERR_PEND_ABORT_NONE    If the task was not waiting for a signal
*                            OS_ERR_PEND_ABORT_SELF    If you attempted to pend abort the calling task.  This is not
*                                                      possible since the calling task cannot be pending because it's
*                                                      running
*
* Returns    : == OS_FALSE   if task was not waiting for a message, or upon error.
*              == OS_TRUE    if task was waiting for a message and was readied and informed.
*
* Note(s)    : none
************************************************************************************************************************
*/

#if (OS_CFG_TASK_SEM_PEND_ABORT_EN > 0u)
CPU_BOOLEAN  OSTaskSemPendAbort (OS_TCB  *p_tcb,
                                 OS_OPT   opt,
                                 OS_ERR  *p_err)
{
    CPU_TS  ts;
    CPU_SR_ALLOC();



#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return (OS_FALSE);
    }
#endif

#if (OS_CFG_CALLED_FROM_ISR_CHK_EN > 0u)
    if (OSIntNestingCtr > 0u) {                                 /* See if called from ISR ...                           */
       *p_err = OS_ERR_PEND_ABORT_ISR;                          /* ... can't Pend Abort from an ISR                     */
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
    switch (opt) {                                              /* Validate 'opt'                                       */
        case OS_OPT_POST_NONE:
        case OS_OPT_POST_NO_SCHED:
             break;

        default:
            *p_err = OS_ERR_OPT_INVALID;
             return (OS_FALSE);
    }
#endif

    CPU_CRITICAL_ENTER();
    if ((p_tcb == (OS_TCB *)0) ||                               /* Pend abort self?                                     */
        (p_tcb == OSTCBCurPtr)) {
        CPU_CRITICAL_EXIT();                                    /* ... doesn't make sense!                              */
       *p_err = OS_ERR_PEND_ABORT_SELF;
        return (OS_FALSE);
    }

    if (p_tcb->PendOn != OS_TASK_PEND_ON_TASK_SEM) {            /* Is task waiting for a signal?                        */
        CPU_CRITICAL_EXIT();
       *p_err = OS_ERR_PEND_ABORT_NONE;
        return (OS_FALSE);
    }
    CPU_CRITICAL_EXIT();

    CPU_CRITICAL_ENTER();
#if (OS_CFG_TS_EN > 0u)
    ts = OS_TS_GET();
#else
    ts = 0u;
#endif
    OS_PendAbort(p_tcb,                                         /* Abort the pend                                       */
                 ts,
                 OS_STATUS_PEND_ABORT);
    CPU_CRITICAL_EXIT();
    if ((opt & OS_OPT_POST_NO_SCHED) == 0u) {
        OSSched();                                              /* Run the scheduler                                    */
    }
   *p_err = OS_ERR_NONE;
    return (OS_TRUE);
}
#endif


/*
************************************************************************************************************************
*                                                    SIGNAL A TASK
*
* Description: This function is called to signal a task waiting for a signal.
*
* Arguments  : p_tcb     is the pointer to the TCB of the task to signal.  A NULL pointer indicates that you are sending
*                        a signal to yourself.
*
*              opt       determines the type of POST performed:
*
*                             OS_OPT_POST_NONE         No option
*                             OS_OPT_POST_NO_SCHED     Do not call the scheduler
*
*              p_err     is a pointer to an error code returned by this function:
*
*                            OS_ERR_NONE              If the requested task is signaled
*                            OS_ERR_OPT_INVALID       If you specified an invalid option
*                            OS_ERR_OS_NOT_RUNNING    If uC/OS-III is not running yet
*                            OS_ERR_SEM_OVF           If the post would cause the semaphore count to overflow
*                            OS_ERR_STATE_INVALID     If the task is in an invalid state.  This should never happen
*                                                     and if it does, would be considered a system failure
*
* Returns    : The current value of the task's signal counter or 0 if called from an ISR
*
* Note(s)    : none
************************************************************************************************************************
*/

OS_SEM_CTR  OSTaskSemPost (OS_TCB  *p_tcb,
                           OS_OPT   opt,
                           OS_ERR  *p_err)
{
    OS_SEM_CTR  ctr;
    CPU_TS      ts;
    CPU_SR_ALLOC();


#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return (0u);
    }
#endif

    OS_TRACE_TASK_SEM_POST_ENTER(p_tcb, opt);

#if (OS_CFG_INVALID_OS_CALLS_CHK_EN > 0u)
    if (OSRunning != OS_STATE_OS_RUNNING) {                     /* Is the kernel running?                               */
        OS_TRACE_TASK_SEM_POST_EXIT(OS_ERR_OS_NOT_RUNNING);
       *p_err = OS_ERR_OS_NOT_RUNNING;
        return (0u);
    }
#endif

#if (OS_CFG_ARG_CHK_EN > 0u)
    switch (opt) {                                              /* Validate 'opt'                                       */
        case OS_OPT_POST_NONE:
        case OS_OPT_POST_NO_SCHED:
             break;

        default:
             OS_TRACE_TASK_SEM_POST_FAILED(p_tcb);
             OS_TRACE_TASK_SEM_POST_EXIT(OS_ERR_OPT_INVALID);
            *p_err =  OS_ERR_OPT_INVALID;
             return (0u);
    }
#endif

#if (OS_CFG_TS_EN > 0u)
    ts = OS_TS_GET();                                           /* Get timestamp                                        */
#else
    ts = 0u;
#endif

    OS_TRACE_TASK_SEM_POST(p_tcb);

    CPU_CRITICAL_ENTER();
    if (p_tcb == (OS_TCB *)0) {                                 /* Post signal to 'self'?                               */
        p_tcb = OSTCBCurPtr;
    }
#if (OS_CFG_TS_EN > 0u)
    p_tcb->TS = ts;
#endif
   *p_err     = OS_ERR_NONE;                                    /* Assume we won't have any errors                      */
    switch (p_tcb->TaskState) {
        case OS_TASK_STATE_RDY:
        case OS_TASK_STATE_DLY:
        case OS_TASK_STATE_SUSPENDED:
        case OS_TASK_STATE_DLY_SUSPENDED:
             if (p_tcb->SemCtr == (OS_SEM_CTR)-1) {
                 CPU_CRITICAL_EXIT();
                *p_err = OS_ERR_SEM_OVF;
                 OS_TRACE_SEM_POST_EXIT(*p_err);
                 return (0u);
             }
             p_tcb->SemCtr++;                                   /* Task signaled is not pending on anything             */
             ctr = p_tcb->SemCtr;
             CPU_CRITICAL_EXIT();
             break;

        case OS_TASK_STATE_PEND:
        case OS_TASK_STATE_PEND_TIMEOUT:
        case OS_TASK_STATE_PEND_SUSPENDED:
        case OS_TASK_STATE_PEND_TIMEOUT_SUSPENDED:
             if (p_tcb->PendOn == OS_TASK_PEND_ON_TASK_SEM) {   /* Is task signaled waiting for a signal?               */
                 OS_Post((OS_PEND_OBJ *)0,                      /* Task is pending on signal                            */
                          p_tcb,
                          (void *)0,
                          0u,
                          ts);
                 ctr = p_tcb->SemCtr;
                 CPU_CRITICAL_EXIT();
                 if ((opt & OS_OPT_POST_NO_SCHED) == 0u) {
                     OSSched();                                 /* Run the scheduler                                    */
                 }
             } else {
                 if (p_tcb->SemCtr == (OS_SEM_CTR)-1) {
                     CPU_CRITICAL_EXIT();
                    *p_err = OS_ERR_SEM_OVF;
                     OS_TRACE_SEM_POST_EXIT(*p_err);
                     return (0u);
                 }
                 p_tcb->SemCtr++;                               /* No,  Task signaled is NOT pending on semaphore ...   */
                 ctr = p_tcb->SemCtr;                           /* ... it must be waiting on something else             */
                 CPU_CRITICAL_EXIT();
             }
             break;

        default:
             CPU_CRITICAL_EXIT();
            *p_err = OS_ERR_STATE_INVALID;
             ctr   = 0u;
             break;
    }

    OS_TRACE_TASK_SEM_POST_EXIT(*p_err);

    return (ctr);
}


/*
************************************************************************************************************************
*                                            SET THE SIGNAL COUNTER OF A TASK
*
* Description: This function is called to clear the signal counter
*
* Arguments  : p_tcb      is the pointer to the TCB of the task to clear the counter.  If you specify a NULL pointer
*                         then the signal counter of the current task will be cleared.
*
*              cnt        is the desired value of the semaphore counter
*
*              p_err      is a pointer to an error code returned by this function
*
*                             OS_ERR_NONE             If the signal counter of the requested task is set
*                             OS_ERR_SET_ISR          If the function was called from an ISR
*                             OS_ERR_TASK_WAITING     One or more tasks were waiting on the semaphore
*
* Returns    : none
*
* Note(s)    : none
************************************************************************************************************************
*/

OS_SEM_CTR  OSTaskSemSet (OS_TCB      *p_tcb,
                          OS_SEM_CTR   cnt,
                          OS_ERR      *p_err)
{
    OS_SEM_CTR  ctr;
    CPU_SR_ALLOC();



#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return (0u);
    }
#endif

#if (OS_CFG_CALLED_FROM_ISR_CHK_EN > 0u)
    if (OSIntNestingCtr > 0u) {                                 /* Not allowed to call from an ISR                      */
       *p_err = OS_ERR_SET_ISR;
        return (0u);
    }
#endif

    CPU_CRITICAL_ENTER();
    if (p_tcb == (OS_TCB *)0) {
        p_tcb = OSTCBCurPtr;
    }

    if (((p_tcb->TaskState   & OS_TASK_STATE_PEND) != 0u) &&    /* Not allowed when a task is waiting.                  */
         (p_tcb->PendOn == OS_TASK_PEND_ON_TASK_SEM)) {
        CPU_CRITICAL_EXIT();
       *p_err = OS_ERR_TASK_WAITING;
        return (0u);
    }

    ctr           =  p_tcb->SemCtr;
    p_tcb->SemCtr = (OS_SEM_CTR)cnt;
    CPU_CRITICAL_EXIT();
   *p_err         =  OS_ERR_NONE;
    return (ctr);
}


/*
************************************************************************************************************************
*                                                    STACK CHECKING
*
* Description: This function is called to calculate the amount of free memory left on the specified task's stack.
*
* Arguments  : p_tcb       is a pointer to the TCB of the task to check.  If you specify a NULL pointer then
*                          you are specifying that you want to check the stack of the current task.
*
*              p_free      is a pointer to a variable that will receive the number of free 'entries' on the task's stack.
*
*              p_used      is a pointer to a variable that will receive the number of used 'entries' on the task's stack.
*
*              p_err       is a pointer to a variable that will contain an error code.
*
*                              OS_ERR_NONE               Upon success
*                              OS_ERR_PTR_INVALID        If either 'p_free' or 'p_used' are NULL pointers
*                              OS_ERR_TASK_NOT_EXIST     If the stack pointer of the task is a NULL pointer
*                              OS_ERR_TASK_OPT           If you did NOT specified OS_OPT_TASK_STK_CHK when the task
*                                                        was created
*                              OS_ERR_TASK_STK_CHK_ISR   You called this function from an ISR
*
* Returns    : none
*
* Note(s)    : none
************************************************************************************************************************
*/

#if (OS_CFG_STAT_TASK_STK_CHK_EN > 0u)
void  OSTaskStkChk (OS_TCB        *p_tcb,
                    CPU_STK_SIZE  *p_free,
                    CPU_STK_SIZE  *p_used,
                    OS_ERR        *p_err)
{
    CPU_STK_SIZE  free_stk;
    CPU_STK_SIZE  stk_size;
    CPU_STK      *p_stk;
    CPU_SR_ALLOC();


#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return;
    }
#endif

#if (OS_CFG_CALLED_FROM_ISR_CHK_EN > 0u)
    if (OSIntNestingCtr > 0u) {                                 /* See if trying to check stack from ISR                */
       *p_err = OS_ERR_TASK_STK_CHK_ISR;
        return;
    }
#endif

#if (OS_CFG_ARG_CHK_EN > 0u)
    if (p_free == (CPU_STK_SIZE *)0) {                          /* User must specify valid destinations for the sizes   */
       *p_err = OS_ERR_PTR_INVALID;
        return;
    }

    if (p_used == (CPU_STK_SIZE *)0) {
       *p_err = OS_ERR_PTR_INVALID;
        return;
    }
#endif

    CPU_CRITICAL_ENTER();
    if (p_tcb == (OS_TCB *)0) {                                 /* Check the stack of the current task?                 */
        p_tcb = OSTCBCurPtr;                                    /* Yes                                                  */
    }

    if (p_tcb->StkPtr == (CPU_STK *)0) {                        /* Make sure task exist                                 */
        CPU_CRITICAL_EXIT();
       *p_free = 0u;
       *p_used = 0u;
       *p_err  = OS_ERR_TASK_NOT_EXIST;
        return;
    }

    if ((p_tcb->Opt & OS_OPT_TASK_STK_CHK) == 0u) {             /* Make sure stack checking option is set               */
        CPU_CRITICAL_EXIT();
       *p_free = 0u;
       *p_used = 0u;
       *p_err  = OS_ERR_TASK_OPT;
        return;
    }

#if (CPU_CFG_STK_GROWTH == CPU_STK_GROWTH_HI_TO_LO)
    p_stk = p_tcb->StkBasePtr;                                  /* Start at the lowest memory and go up                 */
#if (OS_CFG_TASK_STK_REDZONE_EN > 0u)
    p_stk += OS_CFG_TASK_STK_REDZONE_DEPTH;
#endif
#else
    p_stk = p_tcb->StkBasePtr + p_tcb->StkSize - 1u;            /* Start at the highest memory and go down              */
#if (OS_CFG_TASK_STK_REDZONE_EN > 0u)
    p_stk -= OS_CFG_TASK_STK_REDZONE_DEPTH;
#endif
#endif

    stk_size = p_tcb->StkSize;
    CPU_CRITICAL_EXIT();

    free_stk = 0u;
                                                                /* Compute the number of zero entries on the stk        */
#if (CPU_CFG_STK_GROWTH == CPU_STK_GROWTH_HI_TO_LO)
    while ((free_stk  < stk_size) &&
           (*p_stk   ==       0u)) {
        p_stk++;
        free_stk++;
    }
#else
    while ((free_stk  < stk_size) &&
           (*p_stk   ==       0u)) {
        free_stk++;
        p_stk--;
    }
#endif
   *p_free = free_stk;
   *p_used = (stk_size - free_stk);                             /* Compute number of entries used on the stack          */
   *p_err  = OS_ERR_NONE;
}
#endif


/*
************************************************************************************************************************
*                                            CHECK THE STACK REDZONE OF A TASK
*
* Description: Verify a task's stack redzone.
*
* Arguments  : p_tcb     is a pointer to the TCB of the task to check or null for the current task.
*
* Returns    : If the stack is corrupted (OS_FALSE) or not (OS_TRUE).
*
* Note(s)    : These functions are INTERNAL to uC/OS-III and your application should not call it.
************************************************************************************************************************
*/

#if (OS_CFG_TASK_STK_REDZONE_EN > 0u)
CPU_BOOLEAN  OSTaskStkRedzoneChk (OS_TCB  *p_tcb)
{
    CPU_BOOLEAN  stk_status;


    if (p_tcb == (OS_TCB *)0) {
        p_tcb = OSTCBCurPtr;
    }
                                                                /* Check if SP is valid:                                */
                                                                /*   StkBase <= SP < (StkBase + StkSize)                */
    if ((p_tcb->StkPtr <   p_tcb->StkBasePtr) ||
        (p_tcb->StkPtr >= (p_tcb->StkBasePtr + p_tcb->StkSize))) {
        return (OS_FALSE);
    }

    stk_status = OS_TaskStkRedzoneChk(p_tcb->StkBasePtr, p_tcb->StkSize);

    return (stk_status);
}
#endif


/*
************************************************************************************************************************
*                                                   SUSPEND A TASK
*
* Description: This function is called to suspend a task.  The task can be the calling task if 'p_tcb' is a NULL pointer
*              or the pointer to the TCB of the calling task.
*
* Arguments  : p_tcb    is a pointer to the TCB to suspend.
*                       If p_tcb is a NULL pointer then, suspend the current task.
*
*              p_err    is a pointer to a variable that will receive an error code from this function.
*
*                           OS_ERR_NONE                        If the requested task is suspended
*                           OS_ERR_OS_NOT_RUNNING              If uC/OS-III is not running yet
*                           OS_ERR_SCHED_LOCKED                You can't suspend the current task is the scheduler is
*                                                                  locked
*                           OS_ERR_STATE_INVALID               If the task is in an invalid state
*                           OS_ERR_TASK_SUSPEND_CTR_OVF        If the nesting counter overflowed.
*                           OS_ERR_TASK_SUSPEND_ISR            If you called this function from an ISR
*                           OS_ERR_TASK_SUSPEND_IDLE           If you attempted to suspend the idle task which is not
*                                                                  allowed
*                           OS_ERR_TASK_SUSPEND_INT_HANDLER    If you attempted to suspend the idle task which is not
*                                                                  allowed
*
* Returns    : none
*
* Note(s)    : 1) You should use this function with great care.  If you suspend a task that is waiting for an event
*                 (i.e. a message, a semaphore, a queue ...) you will prevent this task from running when the event
*                 arrives.
************************************************************************************************************************
*/

#if (OS_CFG_TASK_SUSPEND_EN > 0u)
void   OSTaskSuspend (OS_TCB  *p_tcb,
                      OS_ERR  *p_err)
{
    CPU_SR_ALLOC();


    OS_TRACE_TASK_SUSPEND_ENTER(p_tcb);

#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return;
    }
#endif

#if (OS_CFG_CALLED_FROM_ISR_CHK_EN > 0u)
    if (OSIntNestingCtr > 0u) {                                 /* Not allowed to call from an ISR                      */
       *p_err = OS_ERR_TASK_SUSPEND_ISR;
        OS_TRACE_TASK_SUSPEND_EXIT(OS_ERR_TASK_SUSPEND_ISR);
        return;
    }
#endif

#if (OS_CFG_TASK_IDLE_EN > 0u)
    if (p_tcb == &OSIdleTaskTCB) {                              /* Make sure not suspending the idle task               */
       *p_err = OS_ERR_TASK_SUSPEND_IDLE;
        OS_TRACE_TASK_SUSPEND_EXIT(OS_ERR_TASK_SUSPEND_IDLE);
        return;
    }
#endif

    CPU_CRITICAL_ENTER();
    if (p_tcb == (OS_TCB *)0) {                                 /* See if specified to suspend self                     */
        if (OSRunning != OS_STATE_OS_RUNNING) {                 /* Can't suspend self when the kernel isn't running     */
            CPU_CRITICAL_EXIT();
           *p_err = OS_ERR_OS_NOT_RUNNING;
            OS_TRACE_TASK_SUSPEND_EXIT(OS_ERR_OS_NOT_RUNNING);
            return;
        }
        p_tcb = OSTCBCurPtr;
    }

    if (p_tcb == OSTCBCurPtr) {
        if (OSSchedLockNestingCtr > 0u) {                       /* Can't suspend when the scheduler is locked           */
            CPU_CRITICAL_EXIT();
           *p_err = OS_ERR_SCHED_LOCKED;
            OS_TRACE_TASK_SUSPEND_EXIT(OS_ERR_SCHED_LOCKED);
            return;
        }
    }

   *p_err = OS_ERR_NONE;
    switch (p_tcb->TaskState) {
        case OS_TASK_STATE_RDY:
             p_tcb->TaskState  =  OS_TASK_STATE_SUSPENDED;
             p_tcb->SuspendCtr = 1u;
             OS_RdyListRemove(p_tcb);
             OS_TRACE_TASK_SUSPEND(p_tcb);
             CPU_CRITICAL_EXIT();
             break;

        case OS_TASK_STATE_DLY:
             p_tcb->TaskState  = OS_TASK_STATE_DLY_SUSPENDED;
             p_tcb->SuspendCtr = 1u;
             CPU_CRITICAL_EXIT();
             break;

        case OS_TASK_STATE_PEND:
             p_tcb->TaskState  = OS_TASK_STATE_PEND_SUSPENDED;
             p_tcb->SuspendCtr = 1u;
             CPU_CRITICAL_EXIT();
             break;

        case OS_TASK_STATE_PEND_TIMEOUT:
             p_tcb->TaskState  = OS_TASK_STATE_PEND_TIMEOUT_SUSPENDED;
             p_tcb->SuspendCtr = 1u;
             CPU_CRITICAL_EXIT();
             break;

        case OS_TASK_STATE_SUSPENDED:
        case OS_TASK_STATE_DLY_SUSPENDED:
        case OS_TASK_STATE_PEND_SUSPENDED:
        case OS_TASK_STATE_PEND_TIMEOUT_SUSPENDED:
             if (p_tcb->SuspendCtr == (OS_NESTING_CTR)-1) {
                 CPU_CRITICAL_EXIT();
                *p_err = OS_ERR_TASK_SUSPEND_CTR_OVF;
                 OS_TRACE_TASK_SUSPEND_EXIT(OS_ERR_TASK_SUSPEND_CTR_OVF);
                 return;
             }
             p_tcb->SuspendCtr++;
             CPU_CRITICAL_EXIT();
             break;

        default:
             CPU_CRITICAL_EXIT();
            *p_err = OS_ERR_STATE_INVALID;
             OS_TRACE_TASK_SUSPEND_EXIT(OS_ERR_STATE_INVALID);
             return;
    }

    if (OSRunning == OS_STATE_OS_RUNNING) {                     /* Only schedule when the kernel is running             */
        OSSched();
        OS_TRACE_TASK_SUSPEND_EXIT(OS_ERR_NONE);
    }
}
#endif


/*
************************************************************************************************************************
*                                                CHANGE A TASK'S TIME SLICE
*
* Description: This function is called to change the value of the task's specific time slice.
*
* Arguments  : p_tcb        is the pointer to the TCB of the task to change. If you specify an NULL pointer, the current
*                           task is assumed.
*
*              time_quanta  is the number of ticks before the CPU is taken away when round-robin scheduling is enabled.
*
*              p_err        is a pointer to an error code returned by this function:
*
*                               OS_ERR_NONE       Upon success
*                               OS_ERR_SET_ISR    If you called this function from an ISR
*
* Returns    : none
*
* Note(s)    : none
************************************************************************************************************************
*/

#if (OS_CFG_SCHED_ROUND_ROBIN_EN > 0u)
void  OSTaskTimeQuantaSet (OS_TCB   *p_tcb,
                           OS_TICK   time_quanta,
                           OS_ERR   *p_err)
{
    CPU_SR_ALLOC();



#ifdef OS_SAFETY_CRITICAL
    if (p_err == (OS_ERR *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return;
    }
#endif

#if (OS_CFG_CALLED_FROM_ISR_CHK_EN > 0u)
    if (OSIntNestingCtr > 0u) {                                 /* Can't call this function from an ISR                 */
       *p_err = OS_ERR_SET_ISR;
        return;
    }
#endif

    CPU_CRITICAL_ENTER();
    if (p_tcb == (OS_TCB *)0) {
        p_tcb = OSTCBCurPtr;
    }

    if (time_quanta == 0u) {
        p_tcb->TimeQuanta    = OSSchedRoundRobinDfltTimeQuanta;
    } else {
        p_tcb->TimeQuanta    = time_quanta;
    }
    if (p_tcb->TimeQuanta > p_tcb->TimeQuantaCtr) {
        p_tcb->TimeQuantaCtr = p_tcb->TimeQuanta;
    }
    CPU_CRITICAL_EXIT();
   *p_err = OS_ERR_NONE;
}
#endif


/*
************************************************************************************************************************
*                                            ADD/REMOVE TASK TO/FROM DEBUG LIST
*
* Description: These functions are called by uC/OS-III to add or remove an OS_TCB from the debug list.
*
* Arguments  : p_tcb     is a pointer to the OS_TCB to add/remove
*
* Returns    : none
*
* Note(s)    : These functions are INTERNAL to uC/OS-III and your application should not call it.
************************************************************************************************************************
*/

#if (OS_CFG_DBG_EN > 0u)
void  OS_TaskDbgListAdd (OS_TCB  *p_tcb)
{
    p_tcb->DbgPrevPtr                = (OS_TCB *)0;
    if (OSTaskDbgListPtr == (OS_TCB *)0) {
        p_tcb->DbgNextPtr            = (OS_TCB *)0;
    } else {
        p_tcb->DbgNextPtr            =  OSTaskDbgListPtr;
        OSTaskDbgListPtr->DbgPrevPtr =  p_tcb;
    }
    OSTaskDbgListPtr                 =  p_tcb;
}



void  OS_TaskDbgListRemove (OS_TCB  *p_tcb)
{
    OS_TCB  *p_tcb_next;
    OS_TCB  *p_tcb_prev;


    p_tcb_prev = p_tcb->DbgPrevPtr;
    p_tcb_next = p_tcb->DbgNextPtr;

    if (p_tcb_prev == (OS_TCB *)0) {
        OSTaskDbgListPtr = p_tcb_next;
        if (p_tcb_next != (OS_TCB *)0) {
            p_tcb_next->DbgPrevPtr = (OS_TCB *)0;
        }
        p_tcb->DbgNextPtr = (OS_TCB *)0;

    } else if (p_tcb_next == (OS_TCB *)0) {
        p_tcb_prev->DbgNextPtr = (OS_TCB *)0;
        p_tcb->DbgPrevPtr      = (OS_TCB *)0;

    } else {
        p_tcb_prev->DbgNextPtr =  p_tcb_next;
        p_tcb_next->DbgPrevPtr =  p_tcb_prev;
        p_tcb->DbgNextPtr      = (OS_TCB *)0;
        p_tcb->DbgPrevPtr      = (OS_TCB *)0;
    }
}
#endif


/*
************************************************************************************************************************
*                                             TASK MANAGER INITIALIZATION
*
* Description: This function is called by OSInit() to initialize the task management.
*

* Argument(s): p_err        is a pointer to a variable that will contain an error code returned by this function.
*
*                                OS_ERR_NONE     the call was successful
*
* Returns    : none
*
* Note(s)    : This function is INTERNAL to uC/OS-III and your application should not call it.
************************************************************************************************************************
*/

void  OS_TaskInit (OS_ERR  *p_err)
{
#if (OS_CFG_DBG_EN > 0u)
    OSTaskDbgListPtr = (OS_TCB *)0;
#endif

    OSTaskQty        = 0u;                                      /* Clear the number of tasks                            */

#if ((OS_CFG_TASK_PROFILE_EN > 0u) || (OS_CFG_DBG_EN > 0u))
    OSTaskCtxSwCtr   = 0u;                                      /* Clear the context switch counter                     */
#endif

   *p_err            = OS_ERR_NONE;
}


/*
************************************************************************************************************************
*                                               INITIALIZE TCB FIELDS
*
* Description: This function is called to initialize a TCB to default values
*
* Arguments  : p_tcb    is a pointer to the TCB to initialize
*
* Returns    : none
*
* Note(s)    : This function is INTERNAL to uC/OS-III and your application should not call it.
************************************************************************************************************************
*/

void  OS_TaskInitTCB (OS_TCB  *p_tcb)
{
#if (OS_CFG_TASK_REG_TBL_SIZE > 0u)
    OS_REG_ID   reg_id;
#endif
#if defined(OS_CFG_TLS_TBL_SIZE) && (OS_CFG_TLS_TBL_SIZE > 0u)
    OS_TLS_ID   id;
#endif


    p_tcb->StkPtr               = (CPU_STK          *)0;
    p_tcb->StkLimitPtr          = (CPU_STK          *)0;

    p_tcb->ExtPtr               = (void             *)0;

    p_tcb->NextPtr              = (OS_TCB           *)0;
    p_tcb->PrevPtr              = (OS_TCB           *)0;

#if (OS_CFG_TICK_EN > 0u)
    p_tcb->TickNextPtr          = (OS_TCB           *)0;
    p_tcb->TickPrevPtr          = (OS_TCB           *)0;
#endif

#if (OS_CFG_DBG_EN > 0u)
    p_tcb->NamePtr              = (CPU_CHAR *)((void *)"?Task");
#endif

#if ((OS_CFG_DBG_EN > 0u) || (OS_CFG_STAT_TASK_STK_CHK_EN > 0u))
    p_tcb->StkBasePtr           = (CPU_STK          *)0;
#endif

#if (OS_CFG_DBG_EN > 0u)
    p_tcb->TaskEntryAddr        = (OS_TASK_PTR       )0;
    p_tcb->TaskEntryArg         = (void             *)0;
#endif

#if (OS_CFG_TS_EN > 0u)
    p_tcb->TS                   =                     0u;
#endif

#if (OS_MSG_EN > 0u)
    p_tcb->MsgPtr               = (void             *)0;
    p_tcb->MsgSize              =                     0u;
#endif

#if (OS_CFG_TASK_Q_EN > 0u)
    OS_MsgQInit(&p_tcb->MsgQ,
                 0u);
#if (OS_CFG_TASK_PROFILE_EN > 0u)
    p_tcb->MsgQPendTime         =                     0u;
    p_tcb->MsgQPendTimeMax      =                     0u;
#endif
#endif

#if (OS_CFG_FLAG_EN > 0u)
    p_tcb->FlagsPend            =                     0u;
    p_tcb->FlagsOpt             =                     0u;
    p_tcb->FlagsRdy             =                     0u;
#endif

#if (OS_CFG_TASK_REG_TBL_SIZE > 0u)
    for (reg_id = 0u; reg_id < OS_CFG_TASK_REG_TBL_SIZE; reg_id++) {
        p_tcb->RegTbl[reg_id]   =                     0u;
    }
#endif

#if defined(OS_CFG_TLS_TBL_SIZE) && (OS_CFG_TLS_TBL_SIZE > 0u)
    for (id = 0u; id < OS_CFG_TLS_TBL_SIZE; id++) {
        p_tcb->TLS_Tbl[id]      =                     0u;
    }
#endif

    p_tcb->SemCtr               =                     0u;
#if (OS_CFG_TASK_PROFILE_EN > 0u)
    p_tcb->SemPendTime          =                     0u;
    p_tcb->SemPendTimeMax       =                     0u;
#endif

#if ((OS_CFG_DBG_EN > 0u) || (OS_CFG_STAT_TASK_STK_CHK_EN > 0u))
    p_tcb->StkSize              =                     0u;
#endif


#if (OS_CFG_TASK_SUSPEND_EN > 0u)
    p_tcb->SuspendCtr           =                     0u;
#endif

#if (OS_CFG_STAT_TASK_STK_CHK_EN > 0u)
    p_tcb->StkFree              =                     0u;
    p_tcb->StkUsed              =                     0u;
#endif

    p_tcb->Opt                  =                     0u;

#if (OS_CFG_TICK_EN > 0u)
    p_tcb->TickRemain           =                     0u;
    p_tcb->TickCtrPrev          =                     0u;
#endif

#if (OS_CFG_SCHED_ROUND_ROBIN_EN > 0u)
    p_tcb->TimeQuanta           =                     0u;
    p_tcb->TimeQuantaCtr        =                     0u;
#endif

#if (OS_CFG_TASK_PROFILE_EN > 0u)
    p_tcb->CPUUsage             =                     0u;
    p_tcb->CPUUsageMax          =                     0u;
    p_tcb->CtxSwCtr             =                     0u;
    p_tcb->CyclesDelta          =                     0u;
#if (OS_CFG_TS_EN > 0u)
    p_tcb->CyclesStart          =  OS_TS_GET();                 /* Read the current timestamp and save                  */
#else
    p_tcb->CyclesStart          =                     0u;
#endif
    p_tcb->CyclesTotal          =                     0u;
#endif

#ifdef CPU_CFG_INT_DIS_MEAS_EN
    p_tcb->IntDisTimeMax        =                     0u;
#endif
#if (OS_CFG_SCHED_LOCK_TIME_MEAS_EN > 0u)
    p_tcb->SchedLockTimeMax     =                     0u;
#endif

    p_tcb->PendNextPtr          = (OS_TCB           *)0;
    p_tcb->PendPrevPtr          = (OS_TCB           *)0;
    p_tcb->PendObjPtr           = (OS_PEND_OBJ      *)0;
    p_tcb->PendOn               =  OS_TASK_PEND_ON_NOTHING;
    p_tcb->PendStatus           =  OS_STATUS_PEND_OK;
    p_tcb->TaskState            =  OS_TASK_STATE_RDY;

    p_tcb->Prio                 =  OS_PRIO_INIT;
#if (OS_CFG_MUTEX_EN > 0u)
    p_tcb->BasePrio             =  OS_PRIO_INIT;
    p_tcb->MutexGrpHeadPtr      = (OS_MUTEX         *)0;
#endif

#if (OS_CFG_DBG_EN > 0u)
    p_tcb->DbgPrevPtr           = (OS_TCB           *)0;
    p_tcb->DbgNextPtr           = (OS_TCB           *)0;
    p_tcb->DbgNamePtr           = (CPU_CHAR *)((void *)" ");
#endif
}


/*
************************************************************************************************************************
*                                              CATCH ACCIDENTAL TASK RETURN
*
* Description: This function is called if a task accidentally returns without deleting itself.  In other words, a task
*              should either be an infinite loop or delete itself if it's done.
*
* Arguments  : none
*
* Returns    : none
*
* Note(s)    : This function is INTERNAL to uC/OS-III and your application should not call it.
************************************************************************************************************************
*/

void  OS_TaskReturn (void)
{
    OS_ERR  err;



    OSTaskReturnHook(OSTCBCurPtr);                              /* Call hook to let user decide on what to do           */
#if (OS_CFG_TASK_DEL_EN > 0u)
    OSTaskDel((OS_TCB *)0,                                      /* Delete task if it accidentally returns!              */
              &err);
#else
    for (;;) {
        OSTimeDly(OSCfg_TickRate_Hz,
                  OS_OPT_TIME_DLY,
                  &err);
    }
#endif
}


/*
************************************************************************************************************************
*                                          CHECK THE STACK REDZONE OF A TASK
*
* Description: Verify a task's stack redzone.
*
* Arguments  : p_tcb        is a pointer to the base of the stack.
*
*              stk_size     is the size of the stack.
*
* Returns    : If the stack is corrupted (OS_FALSE) or not (OS_TRUE).
*
* Note(s)    : 1) This function is INTERNAL to uC/OS-III and your application should not call it.
************************************************************************************************************************
*/

#if (OS_CFG_TASK_STK_REDZONE_EN > 0u)
CPU_BOOLEAN   OS_TaskStkRedzoneChk (CPU_STK       *p_base,
                                    CPU_STK_SIZE   stk_size)
{
    CPU_INT32U  i;


#if (CPU_CFG_STK_GROWTH == CPU_STK_GROWTH_HI_TO_LO)
   (void)stk_size;                                              /* Prevent compiler warning for not using 'stk_size'    */

    for (i = 0u; i < OS_CFG_TASK_STK_REDZONE_DEPTH; i++) {
        if (*p_base != (CPU_DATA)OS_STACK_CHECK_VAL) {
            return (OS_FALSE);
        }
        p_base++;
    }
#else
    p_base = p_base + stk_size - 1u;
    for (i = 0u; i < OS_CFG_TASK_STK_REDZONE_DEPTH; i++) {
        if (*p_base != (CPU_DATA)OS_STACK_CHECK_VAL) {
            return (OS_FALSE);
        }
        p_base--;
    }
#endif

    return (OS_TRUE);
}
#endif


/*
************************************************************************************************************************
*                                          INITIALIZE A REDZONE ENABLED STACK
*
* Description: This functions is used to initialize a stack with Redzone checking.
*
* Arguments  : p_tcb        is a pointer to the base of the stack.
*
*              stk_size     is the size of the stack.
*
* Returns    : none.
*
* Note(s)    : 1) This function is INTERNAL to uC/OS-III and your application should not call it.
************************************************************************************************************************
*/

#if (OS_CFG_TASK_STK_REDZONE_EN > 0u)
void  OS_TaskStkRedzoneInit (CPU_STK       *p_base,
                             CPU_STK_SIZE   stk_size)
{
    CPU_STK_SIZE   i;


#if (CPU_CFG_STK_GROWTH == CPU_STK_GROWTH_HI_TO_LO)
   (void)stk_size;                                              /* Prevent compiler warning for not using 'stk_size'    */

    for (i = 0u; i < OS_CFG_TASK_STK_REDZONE_DEPTH; i++) {
        *(p_base + i) = (CPU_DATA)OS_STACK_CHECK_VAL;
    }
#else
    for (i = 0u; i < OS_CFG_TASK_STK_REDZONE_DEPTH; i++) {
        *(p_base + stk_size - 1u - i) = (CPU_DATA)OS_STACK_CHECK_VAL;
    }
#endif
}
#endif


/*
************************************************************************************************************************
*                                               CHANGE PRIORITY OF A TASK
*
* Description: This function is called by the kernel to perform the actual operation of changing a task's priority.
*              Priority inheritance is updated if necessary.
*
*
*
* Argument(s): p_tcb        is a pointer to the tcb of the task to change the priority.
*
*              prio_new     is the new priority to give to the task.
*
*
* Returns    : none.
*
* Note(s)    : 1) This function is INTERNAL to uC/OS-III and your application MUST NOT call it.
************************************************************************************************************************
*/

void  OS_TaskChangePrio(OS_TCB  *p_tcb,
                        OS_PRIO  prio_new)
{
    OS_TCB  *p_tcb_owner;
#if (OS_CFG_MUTEX_EN > 0u)
    OS_PRIO  prio_cur;
#endif


    do {
        p_tcb_owner = (OS_TCB *)0;
#if (OS_CFG_MUTEX_EN > 0u)
        prio_cur    =  p_tcb->Prio;
#endif
        switch (p_tcb->TaskState) {
            case OS_TASK_STATE_RDY:
                 OS_RdyListRemove(p_tcb);                       /* Remove from current priority                         */
                 p_tcb->Prio = prio_new;                        /* Set new task priority                                */
                 OS_PrioInsert(p_tcb->Prio);
                 if (p_tcb == OSTCBCurPtr) {
                     OS_RdyListInsertHead(p_tcb);
                 } else {
                     OS_RdyListInsertTail(p_tcb);
                 }
                 break;

            case OS_TASK_STATE_DLY:                             /* Nothing to do except change the priority in the OS_TCB*/
            case OS_TASK_STATE_SUSPENDED:
            case OS_TASK_STATE_DLY_SUSPENDED:
                 p_tcb->Prio = prio_new;                        /* Set new task priority                                */
                 break;

            case OS_TASK_STATE_PEND:
            case OS_TASK_STATE_PEND_TIMEOUT:
            case OS_TASK_STATE_PEND_SUSPENDED:
            case OS_TASK_STATE_PEND_TIMEOUT_SUSPENDED:
                 p_tcb->Prio = prio_new;                        /* Set new task priority                                */
                 switch (p_tcb->PendOn) {                       /* What to do depends on what we are pending on         */
                     case OS_TASK_PEND_ON_FLAG:
                     case OS_TASK_PEND_ON_Q:
                     case OS_TASK_PEND_ON_SEM:
                          OS_PendListChangePrio(p_tcb);
                          break;

                     case OS_TASK_PEND_ON_MUTEX:
#if (OS_CFG_MUTEX_EN > 0u)
                          OS_PendListChangePrio(p_tcb);
                          p_tcb_owner = ((OS_MUTEX *)((void *)p_tcb->PendObjPtr))->OwnerTCBPtr;
                          if (prio_cur > prio_new) {            /* Are we increasing the priority?                      */
                              if (p_tcb_owner->Prio <= prio_new) { /* Yes, do we need to give this prio to the owner?   */
                                  p_tcb_owner = (OS_TCB *)0;
                              } else {
                                                                /* Block is empty when trace is disabled.               */
                                 OS_TRACE_MUTEX_TASK_PRIO_INHERIT(p_tcb_owner, prio_new);
                              }
                          } else {
                              if (p_tcb_owner->Prio == prio_cur) { /* No, is it required to check for a lower prio?     */
                                  prio_new = OS_MutexGrpPrioFindHighest(p_tcb_owner);
                                  prio_new = (prio_new > p_tcb_owner->BasePrio) ? p_tcb_owner->BasePrio : prio_new;
                                  if (prio_new == p_tcb_owner->Prio) {
                                      p_tcb_owner = (OS_TCB *)0;
                                  } else {
                                                                /* Block is empty when trace is disabled.               */
                                     OS_TRACE_MUTEX_TASK_PRIO_DISINHERIT(p_tcb_owner, prio_new);
                                  }
                              }
                          }
#endif
                          break;

                     case OS_TASK_PEND_ON_TASK_Q:
                     case OS_TASK_PEND_ON_TASK_SEM:
                     default:
                                                                /* Default case.                                        */
                          break;
                 }
                 break;

            default:
                 return;
        }
        p_tcb = p_tcb_owner;
    } while (p_tcb != (OS_TCB *)0);
}
