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
*                                            TICK MANAGEMENT
*
* File    : os_tick.c
* Version : V3.08.00
*********************************************************************************************************
*/

#define  MICRIUM_SOURCE
#include "os.h"

#ifdef VSC_INCLUDE_SOURCE_FILE_NAMES
const  CPU_CHAR  *os_tick__c = "$Id: $";
#endif

#if (OS_CFG_TICK_EN > 0u)
/*
************************************************************************************************************************
*                                                 FUNCTION PROTOTYPES
************************************************************************************************************************
*/

static  void  OS_TickListUpdate (OS_TICK  ticks);


/*
************************************************************************************************************************
*                                                      TICK INIT
*
* Description: This function initializes the variables related to the tick handler.
*              The function is internal to uC/OS-III.
*
* Arguments  : p_err          is a pointer to a variable that will contain an error code returned by this function.
*              -----
*                                 OS_ERR_NONE           the tick variables were initialized successfully
*
* Returns    : none
*
* Note(s)    : This function is INTERNAL to uC/OS-III and your application should not call it.
************************************************************************************************************************
*/

void  OS_TickInit (OS_ERR  *p_err)
{
    *p_err                = OS_ERR_NONE;

    OSTickCtr             = 0u;                               /* Clear the tick counter                               */

#if (OS_CFG_DYN_TICK_EN > 0u)
    OSTickCtrStep         = 0u;
#endif

    OSTickList.TCB_Ptr    = (OS_TCB *)0;

#if (OS_CFG_DBG_EN > 0u)
    OSTickList.NbrEntries = 0u;
    OSTickList.NbrUpdated = 0u;
#endif
}

/*
************************************************************************************************************************
*                                                      TICK UPDATE
*
* Description: This function updates the list of task either delayed pending with timeout.
*              The function is internal to uC/OS-III.
*
* Arguments  : ticks          the number of ticks which have elapsed
*              -----
*
* Returns    : none
*
* Note(s)    : This function is INTERNAL to uC/OS-III and your application should not call it.
************************************************************************************************************************
*/

void  OS_TickUpdate (OS_TICK  ticks)
{
#if (OS_CFG_TS_EN > 0u)
    CPU_TS  ts_start;
#endif
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();

    OSTickCtr += ticks;                                         /* Keep track of the number of ticks                    */

    OS_TRACE_TICK_INCREMENT(OSTickCtr);

#if (OS_CFG_TS_EN > 0u)
    ts_start   = OS_TS_GET();
    OS_TickListUpdate(ticks);
    OSTickTime = OS_TS_GET() - ts_start;
    if (OSTickTimeMax < OSTickTime) {
        OSTickTimeMax = OSTickTime;
    }
#else
    OS_TickListUpdate(ticks);
#endif

#if (OS_CFG_DYN_TICK_EN > 0u)
    if (OSTickList.TCB_Ptr != (OS_TCB *)0) {
        OSTickCtrStep = OSTickList.TCB_Ptr->TickRemain;
    } else {
        OSTickCtrStep = 0u;
    }

    OS_DynTickSet(OSTickCtrStep);
#endif
    CPU_CRITICAL_EXIT();
}

/*
************************************************************************************************************************
*                                                      INSERT
*
* Description: This task is internal to uC/OS-III and allows the insertion of a task in a tick list.
*
* Arguments  : p_tcb       is a pointer to the TCB to insert in the list
*
*              elapsed     is the number of elapsed ticks since the last tick interrupt
*
*              tick_base   is value of OSTickCtr from which time is offset
*
*              time        is the amount of time remaining (in ticks) for the task to become ready
*
* Returns    : OS_TRUE     if time is valid for the given tick base
*
*              OS_FALSE    if time is invalid (i.e. zero delay)
*
* Note(s)    : 1) This function is INTERNAL to uC/OS-III and your application should not call it.
*
*              2) This function supports both Periodic Tick Mode (PTM) and Dynamic Tick Mode (DTM).
*
*              3) PTM should always call this function with elapsed == 0u.
************************************************************************************************************************
*/

CPU_BOOLEAN  OS_TickListInsert (OS_TCB   *p_tcb,
                                OS_TICK   elapsed,
                                OS_TICK   tick_base,
                                OS_TICK   time)
{
    OS_TCB        *p_tcb1;
    OS_TCB        *p_tcb2;
    OS_TICK_LIST  *p_list;
    OS_TICK        delta;
    OS_TICK        remain;


    delta = (time + tick_base) - (OSTickCtr + elapsed);         /* How many ticks until our delay expires?              */

    if (delta == 0u) {
        p_tcb->TickRemain = 0u;
        return (OS_FALSE);
    }

    OS_TRACE_TASK_DLY(delta);

    p_list = &OSTickList;
    if (p_list->TCB_Ptr == (OS_TCB *)0) {                       /* Is the list empty?                                   */
        p_tcb->TickRemain   = delta;                            /* Yes, Store time in TCB                               */
        p_tcb->TickNextPtr  = (OS_TCB *)0;
        p_tcb->TickPrevPtr  = (OS_TCB *)0;
        p_list->TCB_Ptr     = p_tcb;                            /* Point to TCB of task to place in the list            */
#if (OS_CFG_DYN_TICK_EN > 0u)
        if (elapsed != 0u) {
            OSTickCtr      += elapsed;                          /* Update OSTickCtr before we set a new tick step.      */
            OS_TRACE_TICK_INCREMENT(OSTickCtr);
        }

        OSTickCtrStep       = delta;
        OS_DynTickSet(OSTickCtrStep);
#endif
#if (OS_CFG_DBG_EN > 0u)
        p_list->NbrEntries  = 1u;                               /* List contains 1 entry                                */
#endif
        return (OS_TRUE);
    }


#if (OS_CFG_DBG_EN > 0u)
    p_list->NbrEntries++;                                       /* Update debug counter to reflect the new entry.       */
#endif

    p_tcb2 = p_list->TCB_Ptr;
    remain = p_tcb2->TickRemain - elapsed;                      /* How many ticks until the head's delay expires?       */

    if ((delta               <   remain) &&                     /* If our entry is the new head of the tick list    ... */
        (p_tcb2->TickPrevPtr == (OS_TCB *)0)) {
        p_tcb->TickRemain    =  delta;                          /* ... the delta is equivalent to the full delay    ... */
        p_tcb2->TickRemain   =  remain - delta;                 /* ... the previous head's delta is now relative to it. */

        p_tcb->TickPrevPtr   = (OS_TCB *)0;
        p_tcb->TickNextPtr   =  p_tcb2;
        p_tcb2->TickPrevPtr  =  p_tcb;
        p_list->TCB_Ptr      =  p_tcb;
#if (OS_CFG_DYN_TICK_EN > 0u)
        if (elapsed != 0u) {
            OSTickCtr       +=  elapsed;                        /* Update OSTickCtr before we set a new tick step.      */
            OS_TRACE_TICK_INCREMENT(OSTickCtr);
        }
                                                                /* In DTM, a new list head must update the tick     ... */
        OSTickCtrStep        =  delta;                          /* ... timer to interrupt at the new delay value.       */
        OS_DynTickSet(OSTickCtrStep);
#endif

        return (OS_TRUE);
    }

                                                                /* Our entry comes after the current list head.         */
    delta  -= remain;                                           /* Make delta relative to the head.                     */
    p_tcb1  = p_tcb2;
    p_tcb2  = p_tcb1->TickNextPtr;

    while ((p_tcb2 !=        (OS_TCB *)0) &&                    /* Find the appropriate position in the delta list.     */
           (delta  >= p_tcb2->TickRemain)) {
        delta  -= p_tcb2->TickRemain;
        p_tcb1  = p_tcb2;
        p_tcb2  = p_tcb2->TickNextPtr;
    }

    if (p_tcb2 != (OS_TCB *)0) {                                /* Our entry is not the last element in the list.       */
        p_tcb1               = p_tcb2->TickPrevPtr;
        p_tcb->TickRemain    = delta;                           /* Store remaining time                                 */
        p_tcb->TickPrevPtr   = p_tcb1;
        p_tcb->TickNextPtr   = p_tcb2;
        p_tcb2->TickRemain  -= delta;                           /* Reduce time of next entry in the list                */
        p_tcb2->TickPrevPtr  = p_tcb;
        p_tcb1->TickNextPtr  = p_tcb;

    } else {                                                    /* Our entry belongs at the end of the list.            */
        p_tcb->TickRemain    = delta;
        p_tcb->TickPrevPtr   = p_tcb1;
        p_tcb->TickNextPtr   = (OS_TCB *)0;
        p_tcb1->TickNextPtr  = p_tcb;
    }

    return (OS_TRUE);
}

/*
************************************************************************************************************************
*                                            ADD DELAYED TASK TO TICK LIST
*
* Description: This function is called to place a task in a list of task waiting for time to expire
*
* Arguments  : p_tcb          is a pointer to the OS_TCB of the task to add to the tick list
*              -----
*
*              time           represents either the 'match' value of OSTickCtr or a relative time from the current
*                             system time as specified by the 'opt' argument..
*
*                             relative when 'opt' is set to OS_OPT_TIME_DLY
*                             relative when 'opt' is set to OS_OPT_TIME_TIMEOUT
*                             match    when 'opt' is set to OS_OPT_TIME_MATCH
*                             periodic when 'opt' is set to OS_OPT_TIME_PERIODIC
*
*              opt            is an option specifying how to calculate time.  The valid values are:
*              ---
*                                 OS_OPT_TIME_DLY
*                                 OS_OPT_TIME_TIMEOUT
*                                 OS_OPT_TIME_PERIODIC
*                                 OS_OPT_TIME_MATCH
*
*              p_err          is a pointer to a variable that will contain an error code returned by this function.
*              -----
*                                 OS_ERR_NONE           the call was successful and the time delay was scheduled.
*                                 OS_ERR_TIME_ZERO_DLY  if the effective delay is zero
*
* Returns    : None
*
* Note(s)    : 1) This function is INTERNAL to uC/OS-III and your application MUST NOT call it.
*
*              2) This function is assumed to be called with interrupts disabled.
************************************************************************************************************************
*/

void  OS_TickListInsertDly (OS_TCB   *p_tcb,
                            OS_TICK   time,
                            OS_OPT    opt,
                            OS_ERR   *p_err)
{
    OS_TICK      elapsed;
    OS_TICK      tick_base;
    OS_TICK      base_offset;
    CPU_BOOLEAN  valid_dly;


#if (OS_CFG_DYN_TICK_EN > 0u)
    elapsed  = OS_DynTickGet();
#else
    elapsed  = 0u;
#endif

    if (opt == OS_OPT_TIME_MATCH) {                             /* MATCH to absolute tick ctr value mode                */
        tick_base = 0u;                                         /* tick_base + time == time                             */

    } else if (opt == OS_OPT_TIME_PERIODIC) {                   /* PERIODIC mode.                                       */
        if (time == 0u) {
           *p_err = OS_ERR_TIME_ZERO_DLY;                       /* Infinite frequency is invalid.                       */
            return;
        }

        tick_base = p_tcb->TickCtrPrev;

#if (OS_CFG_DYN_TICK_EN > 0u)                                   /* How far is our tick-base from the system time?       */
        base_offset = OSTickCtr + elapsed - tick_base;
#else
        base_offset = OSTickCtr - tick_base;
#endif

        if (base_offset >= time) {                              /* If our task missed the last period, move         ... */
            tick_base += time * (base_offset / time);           /* ... tick_base up to the next one.                    */
            if ((base_offset % time) != 0u) {
                tick_base += time;                              /* Account for rounding errors with integer division    */
            }

            p_tcb->TickCtrPrev = tick_base;                     /* Adjust the periodic tick base                        */
        }

        p_tcb->TickCtrPrev += time;                             /* Update for the next time we perform a periodic dly.  */

    } else {                                                    /* RELATIVE time delay mode                             */
#if (OS_CFG_DYN_TICK_EN > 0u)                                   /* Our base is always the current system time.          */
        tick_base = OSTickCtr + elapsed;
#else
        tick_base = OSTickCtr;
#endif
    }

    valid_dly = OS_TickListInsert(p_tcb, elapsed, tick_base, time);

    if (valid_dly == OS_TRUE) {
        p_tcb->TaskState = OS_TASK_STATE_DLY;
       *p_err            = OS_ERR_NONE;
    } else {
       *p_err = OS_ERR_TIME_ZERO_DLY;
    }
}

/*
************************************************************************************************************************
*                                         REMOVE A TASK FROM THE TICK LIST
*
* Description: This function is called to remove a task from the tick list
*
* Arguments  : p_tcb          Is a pointer to the OS_TCB to remove.
*              -----
*
* Returns    : none
*
* Note(s)    : 1) This function is INTERNAL to uC/OS-III and your application MUST NOT call it.
*
*              2) This function is assumed to be called with interrupts disabled.
************************************************************************************************************************
*/

void  OS_TickListRemove (OS_TCB  *p_tcb)
{
    OS_TCB        *p_tcb1;
    OS_TCB        *p_tcb2;
    OS_TICK_LIST  *p_list;
#if (OS_CFG_DYN_TICK_EN > 0u)
    OS_TICK        elapsed;
#endif

#if (OS_CFG_DYN_TICK_EN > 0u)
    elapsed = OS_DynTickGet();
#endif

    p_tcb1 = p_tcb->TickPrevPtr;
    p_tcb2 = p_tcb->TickNextPtr;
    p_list = &OSTickList;
    if (p_tcb1 == (OS_TCB *)0) {
        if (p_tcb2 == (OS_TCB *)0) {                            /* Remove the ONLY entry in the list?                   */
            p_list->TCB_Ptr      = (OS_TCB *)0;
#if (OS_CFG_DBG_EN > 0u)
            p_list->NbrEntries   =           0u;
#endif
            p_tcb->TickRemain    =           0u;
#if (OS_CFG_DYN_TICK_EN > 0u)
            if (elapsed != 0u) {
                OSTickCtr       += elapsed;                     /* Keep track of time.                                  */
                OS_TRACE_TICK_INCREMENT(OSTickCtr);
            }
            OSTickCtrStep        =           0u;
            OS_DynTickSet(OSTickCtrStep);
#endif
        } else {
            p_tcb2->TickPrevPtr  = (OS_TCB *)0;
            p_tcb2->TickRemain  += p_tcb->TickRemain;           /* Add back the ticks to the delta                      */
            p_list->TCB_Ptr      = p_tcb2;
#if (OS_CFG_DBG_EN > 0u)
            p_list->NbrEntries--;
#endif

#if (OS_CFG_DYN_TICK_EN > 0u)
            if (p_tcb2->TickRemain != p_tcb->TickRemain) {      /* Only set a new tick if tcb2 had a longer delay.      */
                if (elapsed != 0u) {
                    OSTickCtr          += elapsed;              /* Keep track of time.                                  */
                    OS_TRACE_TICK_INCREMENT(OSTickCtr);
                    p_tcb2->TickRemain -= elapsed;              /* We must account for any time which has passed.       */
                }
                OSTickCtrStep           = p_tcb2->TickRemain;
                OS_DynTickSet(OSTickCtrStep);
            }
#endif
            p_tcb->TickNextPtr          = (OS_TCB *)0;
            p_tcb->TickRemain           =           0u;
        }
    } else {
        p_tcb1->TickNextPtr = p_tcb2;
        if (p_tcb2 != (OS_TCB *)0) {
            p_tcb2->TickPrevPtr  = p_tcb1;
            p_tcb2->TickRemain  += p_tcb->TickRemain;            /* Add back the ticks to the delta list                 */
        }
        p_tcb->TickPrevPtr       = (OS_TCB *)0;
#if (OS_CFG_DBG_EN > 0u)
        p_list->NbrEntries--;
#endif
        p_tcb->TickNextPtr       = (OS_TCB *)0;
        p_tcb->TickRemain        =           0u;
    }
}

/*
************************************************************************************************************************
*                                 UPDATE THE LIST OF TASKS DELAYED OR PENDING WITH TIMEOUT
*
* Description: This function updates the delta list which contains tasks that are delayed or pending with a timeout.
*
* Arguments  : ticks          the number of ticks which have elapsed.
*
* Returns    : none
*
* Note(s)    : 1) This function is INTERNAL to uC/OS-III and your application MUST NOT call it.
************************************************************************************************************************
*/

static  void  OS_TickListUpdate (OS_TICK  ticks)
{
    OS_TCB        *p_tcb;
    OS_TICK_LIST  *p_list;
#if (OS_CFG_DBG_EN > 0u)
    OS_OBJ_QTY     nbr_updated;
#endif
#if (OS_CFG_MUTEX_EN > 0u)
    OS_TCB        *p_tcb_owner;
    OS_PRIO        prio_new;
#endif



#if (OS_CFG_DBG_EN > 0u)
    nbr_updated = 0u;
#endif
    p_list      = &OSTickList;
    p_tcb       = p_list->TCB_Ptr;
    if (p_tcb != (OS_TCB *)0) {
        if (p_tcb->TickRemain <= ticks) {
            ticks              = ticks - p_tcb->TickRemain;
            p_tcb->TickRemain  = 0u;
        } else {
            p_tcb->TickRemain -= ticks;
        }

        while (p_tcb->TickRemain == 0u) {
#if (OS_CFG_DBG_EN > 0u)
            nbr_updated++;
#endif

            switch (p_tcb->TaskState) {
                case OS_TASK_STATE_DLY:
                     p_tcb->TaskState = OS_TASK_STATE_RDY;
                     OS_RdyListInsert(p_tcb);                            /* Insert the task in the ready list                    */
                     break;

                case OS_TASK_STATE_DLY_SUSPENDED:
                     p_tcb->TaskState = OS_TASK_STATE_SUSPENDED;
                     break;

                default:
#if (OS_CFG_MUTEX_EN > 0u)
                     p_tcb_owner = (OS_TCB *)0;
                     if (p_tcb->PendOn == OS_TASK_PEND_ON_MUTEX) {
                         p_tcb_owner = (OS_TCB *)((OS_MUTEX *)((void *)p_tcb->PendObjPtr))->OwnerTCBPtr;
                     }
#endif

#if (OS_MSG_EN > 0u)
                     p_tcb->MsgPtr  = (void *)0;
                     p_tcb->MsgSize = 0u;
#endif
#if (OS_CFG_TS_EN > 0u)
                     p_tcb->TS      = OS_TS_GET();
#endif
                     OS_PendListRemove(p_tcb);                           /* Remove task from pend list                           */

                     switch (p_tcb->TaskState) {
                         case OS_TASK_STATE_PEND_TIMEOUT:
                              OS_RdyListInsert(p_tcb);                   /* Insert the task in the ready list                    */
                              p_tcb->TaskState  = OS_TASK_STATE_RDY;
                              break;

                         case OS_TASK_STATE_PEND_TIMEOUT_SUSPENDED:
                              p_tcb->TaskState  = OS_TASK_STATE_SUSPENDED;
                              break;

                         default:
                              break;
                     }
                     p_tcb->PendStatus = OS_STATUS_PEND_TIMEOUT;         /* Indicate pend timed out                              */
                     p_tcb->PendOn     = OS_TASK_PEND_ON_NOTHING;        /* Indicate no longer pending                           */

#if (OS_CFG_MUTEX_EN > 0u)
                     if (p_tcb_owner != (OS_TCB *)0) {
                         if ((p_tcb_owner->Prio != p_tcb_owner->BasePrio) &&
                             (p_tcb_owner->Prio == p_tcb->Prio)) {       /* Has the owner inherited a priority?                  */
                             prio_new = OS_MutexGrpPrioFindHighest(p_tcb_owner);
                             prio_new = (prio_new > p_tcb_owner->BasePrio) ? p_tcb_owner->BasePrio : prio_new;
                             if (prio_new != p_tcb_owner->Prio) {
                                 OS_TaskChangePrio(p_tcb_owner, prio_new);
                                 OS_TRACE_MUTEX_TASK_PRIO_DISINHERIT(p_tcb_owner, p_tcb_owner->Prio);
                             }
                         }
                     }
#endif
                     break;
            }

            p_list->TCB_Ptr = p_tcb->TickNextPtr;
            p_tcb           = p_list->TCB_Ptr;                           /* Get 'p_tcb' again for loop                           */
            if (p_tcb == (OS_TCB *)0) {
#if (OS_CFG_DBG_EN > 0u)
                p_list->NbrEntries = 0u;
#endif
                break;
            } else {
#if (OS_CFG_DBG_EN > 0u)
                p_list->NbrEntries--;
#endif
                p_tcb->TickPrevPtr = (OS_TCB *)0;
                if (p_tcb->TickRemain <= ticks) {
                    ticks              = ticks - p_tcb->TickRemain;
                    p_tcb->TickRemain  = 0u;
                } else {
                    p_tcb->TickRemain -= ticks;
                }
            }
        }
    }
#if (OS_CFG_DBG_EN > 0u)
    p_list->NbrUpdated = nbr_updated;
#endif
}

#endif                                                                   /* #if OS_CFG_TICK_EN                                   */

