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
*                                          CONFIGURATION FILE
*
* Filename : os_cfg.h
* Version  : V3.08.00
*********************************************************************************************************
*/

#ifndef OS_CFG_H
#define OS_CFG_H

                                                                /* --------------------------- MISCELLANEOUS --------------------------- */
#define OS_CFG_APP_HOOKS_EN                        1u           /* Enable (1) or Disable (0) application specific hooks                  */
#define OS_CFG_ARG_CHK_EN                          1u           /* Enable (1) or Disable (0) argument checking                           */
#define OS_CFG_CALLED_FROM_ISR_CHK_EN              1u           /* Enable (1) or Disable (0) check for called from ISR                   */
#define OS_CFG_DBG_EN                              0u           /* Enable (1) or Disable (0) debug code/variables                        */
#define OS_CFG_TICK_EN                             1u           /* Enable (1) or Disable (0) the kernel tick                             */
#define OS_CFG_DYN_TICK_EN                         0u           /* Enable (1) or Disable (0) the Dynamic Tick                            */
#define OS_CFG_INVALID_OS_CALLS_CHK_EN             1u           /* Enable (1) or Disable (0) checks for invalid kernel calls             */
#define OS_CFG_OBJ_TYPE_CHK_EN                     1u           /* Enable (1) or Disable (0) object type checking                        */
#define OS_CFG_TS_EN                               0u           /* Enable (1) or Disable (0) time stamping                               */

#define OS_CFG_PRIO_MAX                           64u           /* Defines the maximum number of task priorities (see OS_PRIO data type) */

#define OS_CFG_SCHED_LOCK_TIME_MEAS_EN             0u           /* Include code to measure scheduler lock time                           */
#define OS_CFG_SCHED_ROUND_ROBIN_EN                1u           /* Include code for Round-Robin scheduling                               */

#define OS_CFG_STK_SIZE_MIN                       64u           /* Minimum allowable task stack size                                     */


                                                                /* --------------------------- EVENT FLAGS ----------------------------- */
#define OS_CFG_FLAG_EN                             1u           /* Enable (1) or Disable (0) code generation for EVENT FLAGS             */
#define OS_CFG_FLAG_DEL_EN                         1u           /*     Include code for OSFlagDel()                                      */
#define OS_CFG_FLAG_MODE_CLR_EN                    1u           /*     Include code for Wait on Clear EVENT FLAGS                        */
#define OS_CFG_FLAG_PEND_ABORT_EN                  1u           /*     Include code for OSFlagPendAbort()                                */


                                                                /* ------------------------ MEMORY MANAGEMENT -------------------------  */
#define OS_CFG_MEM_EN                              1u           /* Enable (1) or Disable (0) code generation for the MEMORY MANAGER      */


                                                                /* ------------------- MUTUAL EXCLUSION SEMAPHORES --------------------  */
#define OS_CFG_MUTEX_EN                            1u           /* Enable (1) or Disable (0) code generation for MUTEX                   */
#define OS_CFG_MUTEX_DEL_EN                        1u           /*     Include code for OSMutexDel()                                     */
#define OS_CFG_MUTEX_PEND_ABORT_EN                 1u           /*     Include code for OSMutexPendAbort()                               */


                                                                /* -------------------------- MESSAGE QUEUES --------------------------  */
#define OS_CFG_Q_EN                                1u           /* Enable (1) or Disable (0) code generation for QUEUES                  */
#define OS_CFG_Q_DEL_EN                            1u           /*     Include code for OSQDel()                                         */
#define OS_CFG_Q_FLUSH_EN                          1u           /*     Include code for OSQFlush()                                       */
#define OS_CFG_Q_PEND_ABORT_EN                     1u           /*     Include code for OSQPendAbort()                                   */


                                                                /* ---------------------------- SEMAPHORES ----------------------------- */
#define OS_CFG_SEM_EN                              1u           /* Enable (1) or Disable (0) code generation for SEMAPHORES              */
#define OS_CFG_SEM_DEL_EN                          1u           /*     Include code for OSSemDel()                                       */
#define OS_CFG_SEM_PEND_ABORT_EN                   1u           /*     Include code for OSSemPendAbort()                                 */
#define OS_CFG_SEM_SET_EN                          1u           /*     Include code for OSSemSet()                                       */


                                                                /* -------------------------- TASK MANAGEMENT -------------------------- */
#define OS_CFG_STAT_TASK_EN                        1u           /* Enable (1) or Disable (0) the statistics task                         */
#define OS_CFG_STAT_TASK_STK_CHK_EN                1u           /*     Check task stacks from the statistic task                         */

#define OS_CFG_TASK_CHANGE_PRIO_EN                 1u           /* Include code for OSTaskChangePrio()                                   */
#define OS_CFG_TASK_DEL_EN                         1u           /* Include code for OSTaskDel()                                          */
#define OS_CFG_TASK_IDLE_EN                        1u           /* Include the idle task                                                 */
#define OS_CFG_TASK_PROFILE_EN                     1u           /* Include variables in OS_TCB for profiling                             */
#define OS_CFG_TASK_Q_EN                           1u           /* Include code for OSTaskQXXXX()                                        */
#define OS_CFG_TASK_Q_PEND_ABORT_EN                1u           /* Include code for OSTaskQPendAbort()                                   */
#define OS_CFG_TASK_REG_TBL_SIZE                   1u           /* Number of task specific registers                                     */

#define OS_CFG_TASK_STK_REDZONE_EN                 0u           /* Enable (1) or Disable (0) stack redzone                               */
#define OS_CFG_TASK_STK_REDZONE_DEPTH              8u           /* Depth of the stack redzone                                            */

#define OS_CFG_TASK_SEM_PEND_ABORT_EN              1u           /* Include code for OSTaskSemPendAbort()                                 */
#define OS_CFG_TASK_SUSPEND_EN                     1u           /* Include code for OSTaskSuspend() and OSTaskResume()                   */


                                                                /* ------------------ TASK LOCAL STORAGE MANAGEMENT -------------------  */
#define OS_CFG_TLS_TBL_SIZE                        0u           /* Include code for Task Local Storage (TLS) registers                   */


                                                                /* ------------------------- TIME MANAGEMENT --------------------------  */
#define OS_CFG_TIME_DLY_HMSM_EN                    1u           /* Include code for OSTimeDlyHMSM()                                      */
#define OS_CFG_TIME_DLY_RESUME_EN                  1u           /* Include code for OSTimeDlyResume()                                    */


                                                                /* ------------------------- TIMER MANAGEMENT -------------------------- */
#define OS_CFG_TMR_EN                              1u           /* Enable (1) or Disable (0) code generation for TIMERS                  */
#define OS_CFG_TMR_DEL_EN                          1u           /* Enable (1) or Disable (0) code generation for OSTmrDel()              */


                                                                /* ------------------------- TRACE RECORDER ---------------------------- */
#define OS_CFG_TRACE_EN                            0u           /* Enable (1) or Disable (0) uC/OS-III Trace instrumentation             */
#define OS_CFG_TRACE_API_ENTER_EN                  0u           /* Enable (1) or Disable (0) uC/OS-III Trace API enter instrumentation   */
#define OS_CFG_TRACE_API_EXIT_EN                   0u           /* Enable (1) or Disable (0) uC/OS-III Trace API exit  instrumentation   */

#endif
