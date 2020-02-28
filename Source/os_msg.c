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
*                                       MESSAGE HANDLING SERVICES
*
* File    : os_msg.c
* Version : V3.08.00
*********************************************************************************************************
*/

#define  MICRIUM_SOURCE
#include "os.h"

#ifdef VSC_INCLUDE_SOURCE_FILE_NAMES
const  CPU_CHAR  *os_msg__c = "$Id: $";
#endif


#if (OS_MSG_EN > 0u)

/*
************************************************************************************************************************
*                                            INITIALIZE THE POOL OF 'OS_MSG'
*
* Description: This function is called by OSInit() to initialize the free list of OS_MSGs.
*
* Argument(s): p_err     is a pointer to a variable that will contain an error code returned by this function.
*
*                            OS_ERR_MSG_POOL_NULL_PTR
*                            OS_ERR_MSG_POOL_EMPTY
*                            OS_ERR_NONE
*
* Returns    : none
*
* Note(s)    : 1) This function is INTERNAL to uC/OS-III and your application MUST NOT call it.
************************************************************************************************************************
*/

void  OS_MsgPoolInit (OS_ERR  *p_err)
{
    OS_MSG      *p_msg1;
    OS_MSG      *p_msg2;
    OS_MSG_QTY   i;
    OS_MSG_QTY   loops;


#if (OS_CFG_ARG_CHK_EN > 0u)
    if (OSCfg_MsgPoolBasePtr == (OS_MSG *)0) {
       *p_err = OS_ERR_MSG_POOL_NULL_PTR;
        return;
    }
    if (OSCfg_MsgPoolSize == 0u) {
       *p_err = OS_ERR_MSG_POOL_EMPTY;
        return;
    }
#endif

    p_msg1 = OSCfg_MsgPoolBasePtr;
    p_msg2 = OSCfg_MsgPoolBasePtr;
    p_msg2++;
    loops  = OSCfg_MsgPoolSize - 1u;
    for (i = 0u; i < loops; i++) {                              /* Init. list of free OS_MSGs                           */
        p_msg1->NextPtr = p_msg2;
        p_msg1->MsgPtr  = (void *)0;
        p_msg1->MsgSize =         0u;
#if (OS_CFG_TS_EN > 0u)
        p_msg1->MsgTS   =         0u;
#endif
        p_msg1++;
        p_msg2++;
    }
    p_msg1->NextPtr = (OS_MSG *)0;                              /* Last OS_MSG                                          */
    p_msg1->MsgPtr  = (void   *)0;
    p_msg1->MsgSize =           0u;
#if (OS_CFG_TS_EN > 0u)
    p_msg1->MsgTS   =           0u;
#endif

    OSMsgPool.NextPtr    = OSCfg_MsgPoolBasePtr;
    OSMsgPool.NbrFree    = OSCfg_MsgPoolSize;
    OSMsgPool.NbrUsed    = 0u;
#if (OS_CFG_DBG_EN > 0u)
    OSMsgPool.NbrUsedMax = 0u;
#endif
   *p_err                = OS_ERR_NONE;
}


/*
************************************************************************************************************************
*                                        RELEASE ALL MESSAGE IN MESSAGE QUEUE
*
* Description: This function returns all the messages in a message queue to the free list.
*
* Arguments  : p_msg_q       is a pointer to the OS_MSG_Q structure containing messages to free.
*              -------
*
* Returns    : the number of OS_MSGs returned to the free list
*
* Note(s)    : 1) This function is INTERNAL to uC/OS-III and your application MUST NOT call it.
************************************************************************************************************************
*/

OS_MSG_QTY  OS_MsgQFreeAll (OS_MSG_Q  *p_msg_q)
{
    OS_MSG      *p_msg;
    OS_MSG_QTY   qty;



    qty = p_msg_q->NbrEntries;                                  /* Get the number of OS_MSGs being freed                */
    if (p_msg_q->NbrEntries > 0u) {
        p_msg                   = p_msg_q->InPtr;               /* Point to end of message chain                        */
        p_msg->NextPtr          = OSMsgPool.NextPtr;
        OSMsgPool.NextPtr       = p_msg_q->OutPtr;              /* Point to beginning of message chain                  */
        OSMsgPool.NbrUsed      -= p_msg_q->NbrEntries;          /* Update statistics for free list of messages          */
        OSMsgPool.NbrFree      += p_msg_q->NbrEntries;
        p_msg_q->NbrEntries     =           0u;                 /* Flush the message queue                              */
#if (OS_CFG_DBG_EN > 0u)
        p_msg_q->NbrEntriesMax  =           0u;
#endif
        p_msg_q->InPtr          = (OS_MSG *)0;
        p_msg_q->OutPtr         = (OS_MSG *)0;
    }
    return (qty);
}


/*
************************************************************************************************************************
*                                               INITIALIZE A MESSAGE QUEUE
*
* Description: This function is called to initialize a message queue
*
* Arguments  : p_msg_q      is a pointer to the message queue to initialize
*              -------
*
*              size          is the maximum number of entries that a message queue can have.
*
* Returns    : none
*
* Note(s)    : 1) This function is INTERNAL to uC/OS-III and your application MUST NOT call it.
************************************************************************************************************************
*/

void  OS_MsgQInit (OS_MSG_Q    *p_msg_q,
                   OS_MSG_QTY   size)
{
    p_msg_q->NbrEntriesSize = size;
    p_msg_q->NbrEntries     =           0u;
#if (OS_CFG_DBG_EN > 0u)
    p_msg_q->NbrEntriesMax  =           0u;
#endif
    p_msg_q->InPtr          = (OS_MSG *)0;
    p_msg_q->OutPtr         = (OS_MSG *)0;
}


/*
************************************************************************************************************************
*                                           RETRIEVE MESSAGE FROM MESSAGE QUEUE
*
* Description: This function retrieves a message from a message queue
*
* Arguments  : p_msg_q     is a pointer to the message queue where we want to extract the message from
*              -------
*
*              p_msg_size  is a pointer to where the size (in bytes) of the message will be placed
*
*              p_ts        is a pointer to where the time stamp will be placed
*
*              p_err       is a pointer to an error code that will be returned from this call.
*
*                              OS_ERR_Q_EMPTY
*                              OS_ERR_NONE
*
* Returns    : The message (a pointer)
*
* Note(s)    : 1) This function is INTERNAL to uC/OS-III and your application MUST NOT call it.
************************************************************************************************************************
*/

void  *OS_MsgQGet (OS_MSG_Q     *p_msg_q,
                   OS_MSG_SIZE  *p_msg_size,
                   CPU_TS       *p_ts,
                   OS_ERR       *p_err)
{
    OS_MSG  *p_msg;
    void    *p_void;


#if (OS_CFG_TS_EN == 0u)
    (void)p_ts;                                                 /* Prevent compiler warning for not using 'ts'          */
#endif

    if (p_msg_q->NbrEntries == 0u) {                            /* Is the queue empty?                                  */
       *p_msg_size = 0u;                                        /* Yes                                                  */
#if (OS_CFG_TS_EN > 0u)
        if (p_ts != (CPU_TS *)0) {
           *p_ts = 0u;
        }
#endif
       *p_err = OS_ERR_Q_EMPTY;
        return ((void *)0);
    }

    p_msg           = p_msg_q->OutPtr;                          /* No, get the next message to extract from the queue   */
    p_void          = p_msg->MsgPtr;
   *p_msg_size      = p_msg->MsgSize;
#if (OS_CFG_TS_EN > 0u)
    if (p_ts != (CPU_TS *)0) {
       *p_ts = p_msg->MsgTS;
    }
#endif

    p_msg_q->OutPtr = p_msg->NextPtr;                           /* Point to next message to extract                     */

    if (p_msg_q->OutPtr == (OS_MSG *)0) {                       /* Are there any more messages in the queue?            */
        p_msg_q->InPtr      = (OS_MSG *)0;                      /* No                                                   */
        p_msg_q->NbrEntries =           0u;
    } else {
        p_msg_q->NbrEntries--;                                  /* Yes, One less message in the queue                   */
    }

    p_msg->NextPtr    = OSMsgPool.NextPtr;                      /* Return message control block to free list            */
    OSMsgPool.NextPtr = p_msg;
    OSMsgPool.NbrFree++;
    OSMsgPool.NbrUsed--;

   *p_err             = OS_ERR_NONE;
    return (p_void);
}


/*
************************************************************************************************************************
*                                           DEPOSIT MESSAGE IN MESSAGE QUEUE
*
* Description: This function places a message in a message queue
*
* Arguments  : p_msg_q     is a pointer to the OS_TCB of the task to post the message to
*              -------
*
*              p_void      is a pointer to the message to send.
*
*              msg_size    is the size of the message (in bytes)
*
*              opt         specifies whether the message will be posted in FIFO or LIFO order
*
*                              OS_OPT_POST_FIFO
*                              OS_OPT_POST_LIFO
*
*              ts          is a timestamp as to when the message was posted
*
*              p_err       is a pointer to a variable that will contain an error code returned by this function.
*
*                              OS_ERR_Q_MAX           if the queue is full
*                              OS_ERR_MSG_POOL_EMPTY  if we no longer have any OS_MSG to use
*                              OS_ERR_NONE            the message was deposited in the queue
*
* Returns    : none
*
* Note(s)    : 1) This function is INTERNAL to uC/OS-III and your application MUST NOT call it.
************************************************************************************************************************
*/

void  OS_MsgQPut (OS_MSG_Q     *p_msg_q,
                  void         *p_void,
                  OS_MSG_SIZE   msg_size,
                  OS_OPT        opt,
                  CPU_TS        ts,
                  OS_ERR       *p_err)
{
    OS_MSG  *p_msg;
    OS_MSG  *p_msg_in;


#if (OS_CFG_TS_EN == 0u)
    (void)ts;                                                   /* Prevent compiler warning for not using 'ts'          */
#endif

    if (p_msg_q->NbrEntries >= p_msg_q->NbrEntriesSize) {
       *p_err = OS_ERR_Q_MAX;                                   /* Message queue cannot accept any more messages        */
        return;
    }

    if (OSMsgPool.NbrFree == 0u) {
       *p_err = OS_ERR_MSG_POOL_EMPTY;                          /* No more OS_MSG to use                                */
        return;
    }

    p_msg = OSMsgPool.NextPtr;                                  /* Remove message control block from free list          */
    OSMsgPool.NextPtr = p_msg->NextPtr;
    OSMsgPool.NbrFree--;
    OSMsgPool.NbrUsed++;

#if (OS_CFG_DBG_EN > 0u)
    if (OSMsgPool.NbrUsedMax < OSMsgPool.NbrUsed) {
        OSMsgPool.NbrUsedMax = OSMsgPool.NbrUsed;
    }
#endif

    if (p_msg_q->NbrEntries == 0u) {                            /* Is this first message placed in the queue?           */
        p_msg_q->InPtr         = p_msg;                         /* Yes                                                  */
        p_msg_q->OutPtr        = p_msg;
        p_msg_q->NbrEntries    =           1u;
        p_msg->NextPtr         = (OS_MSG *)0;
    } else {                                                    /* No                                                   */
        if ((opt & OS_OPT_POST_LIFO) == OS_OPT_POST_FIFO) {     /* Is it FIFO or LIFO?                                  */
            p_msg_in           = p_msg_q->InPtr;                /* FIFO, add to the head                                */
            p_msg_in->NextPtr  = p_msg;
            p_msg_q->InPtr     = p_msg;
            p_msg->NextPtr     = (OS_MSG *)0;
        } else {
            p_msg->NextPtr     = p_msg_q->OutPtr;               /* LIFO, add to the tail                                */
            p_msg_q->OutPtr    = p_msg;
        }
        p_msg_q->NbrEntries++;
    }

#if (OS_CFG_DBG_EN > 0u)
    if (p_msg_q->NbrEntriesMax < p_msg_q->NbrEntries) {
        p_msg_q->NbrEntriesMax = p_msg_q->NbrEntries;
    }
#endif

    p_msg->MsgPtr  = p_void;                                    /* Deposit message in the message queue entry           */
    p_msg->MsgSize = msg_size;
#if (OS_CFG_TS_EN > 0u)
    p_msg->MsgTS   = ts;
#endif
   *p_err          = OS_ERR_NONE;
}
#endif
