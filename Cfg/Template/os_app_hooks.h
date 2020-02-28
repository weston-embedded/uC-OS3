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
*
*                                           APPLICATION HOOKS
*
* Filename : os_app_hooks.h
* Version  : V3.08.00
*********************************************************************************************************
*/

#ifndef  OS_APP_HOOKS_H
#define  OS_APP_HOOKS_H


#ifdef   OS_APP_HOOKS_H_GLOBALS
#define  OS_APP_HOOKS_H_EXT
#else
#define  OS_APP_HOOKS_H_EXT  extern
#endif

/*
************************************************************************************************************************
*                                                 INCLUDE HEADER FILES
************************************************************************************************************************
*/

#include <os.h>

/*
************************************************************************************************************************
*                                                 FUNCTION PROTOTYPES
************************************************************************************************************************
*/

void  App_OS_SetAllHooks   (void);
void  App_OS_ClrAllHooks   (void);


                                                                /* ---------------------- HOOKS --------------------- */
void  App_OS_IdleTaskHook  (void);

#if (OS_CFG_TASK_STK_REDZONE_EN > 0u)
void  App_OS_RedzoneHitHook(OS_TCB  *p_tcb);
#endif

void  App_OS_StatTaskHook  (void);

void  App_OS_TaskCreateHook(OS_TCB  *p_tcb);

void  App_OS_TaskDelHook   (OS_TCB  *p_tcb);

void  App_OS_TaskReturnHook(OS_TCB  *p_tcb);

void  App_OS_TaskSwHook    (void);

void  App_OS_TimeTickHook  (void);

#endif
