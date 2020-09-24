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
*                                            POSIX GNU Port
*
* File      : os_cpu_c.c
* Version   : V3.08.00
*********************************************************************************************************
* For       : POSIX
* Toolchain : GNU
*********************************************************************************************************
*/

/* 1U: sigaddset, sigemptyset, sigset_t
 * 199506UL: sigwait
 */
#define _POSIX_C_SOURCE 199506UL

/* syscall */
#define _GNU_SOURCE

#define   OS_CPU_GLOBALS

#ifdef VSC_INCLUDE_SOURCE_FILE_NAMES
const  CPU_CHAR  *os_cpu_c__c = "$Id: $";
#endif

/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include  "../../../Source/os.h"
#include  <os_cfg_app.h>


#include  <stdio.h>
#include  <pthread.h>
#include  <stdint.h>
#include  <signal.h>
#include  <semaphore.h>
#include  <time.h>
#include  <string.h>
#include  <unistd.h>
#include  <stdlib.h>
#include  <sys/types.h>
#include  <sys/syscall.h>
#include  <sys/resource.h>
#include  <errno.h>


#ifdef __cplusplus
extern  "C" {
#endif


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define  THREAD_CREATE_PRIO       50u                           /* Tasks underlying posix threads prio.                 */

                                                                /* Err handling convenience macro.                      */
#define  ERR_CHK(func)            do {int res = func; \
                                      if (res != 0u) { \
                                          printf("Error in call '%s' from %s(): %s­\r\n", #func, __FUNCTION__, strerror(res)); \
                                          perror(" \\->'errno' indicates (might not be relevant if function doesn't use 'errno')"); \
                                          raise(SIGABRT); \
                                      } \
                                  } while(0)

/*
*********************************************************************************************************
*                                          LOCAL DATA TYPES
*********************************************************************************************************
*/

typedef  struct  os_tcb_ext_posix {
    pthread_t  Thread;
    pid_t      ProcessId;
    sem_t      InitSem;
    sem_t      Sem;
} OS_TCB_EXT_POSIX;


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void       *OSTaskPosix           (void       *p_arg);

static  void        OSTaskTerminate       (OS_TCB     *p_tcb);

static  void        OSThreadCreate        (pthread_t  *p_thread,
                                           void     *(*p_task) (void *),
                                           void       *p_arg,
                                           int         prio);

static  void        OSTimeTickHandler     (void);


/*
*********************************************************************************************************
*                                           LOCAL VARIABLES
*********************************************************************************************************
*/

                                                                                            /* Tick timer cfg.          */
static  CPU_TMR_INTERRUPT  OSTickTmrInterrupt = { .Interrupt.NamePtr  = "Tick tmr interrupt",
                                                  .Interrupt.Prio     =  10u,
                                                  .Interrupt.TraceEn  =  0u,
                                                  .Interrupt.ISR_Fnct =  OSTimeTickHandler,
                                                  .Interrupt.En       =  1u,
                                                  .OneShot            =  0u,
                                                  .PeriodSec          =  0u,
                                                  .PeriodMuSec        = (1000000u / OS_CFG_TICK_RATE_HZ)
                                                };


/*
*********************************************************************************************************
*                                     LOCAL CONFIGURATION ERRORS
*********************************************************************************************************
*/

#if (OS_CFG_TICK_RATE_HZ > 100u)
#pragma message("Time accuracy cannot be maintained with OS_CFG_TICK_RATE_HZ > 100u.")
#endif


/*
*********************************************************************************************************
*                                           IDLE TASK HOOK
*
* Description: This function is called by the idle task.  This hook has been added to allow you to do
*              such things as STOP the CPU to conserve power.
*
* Arguments  : None.
*
* Note(s)    : None.
*********************************************************************************************************
*/

void  OSIdleTaskHook (void)
{
#if OS_CFG_APP_HOOKS_EN > 0u
    if (OS_AppIdleTaskHookPtr != (OS_APP_HOOK_VOID)0) {
        (*OS_AppIdleTaskHookPtr)();
    }
#endif

    sleep(1u);                                                  /* Reduce CPU utilization.                              */
}


/*
*********************************************************************************************************
*                                       OS INITIALIZATION HOOK
*
* Description: This function is called by OSInit() at the beginning of OSInit().
*
* Arguments  : None.
*
* Note(s)    : 1) Interrupts should be disabled during this call.
*********************************************************************************************************
*/


void  OSInitHook (void)
{
    struct  rlimit  rtprio_limits;

    ERR_CHK(getrlimit(RLIMIT_RTPRIO, &rtprio_limits));
    if (rtprio_limits.rlim_cur != RLIM_INFINITY) {
        if (rtprio_limits.rlim_max != RLIM_INFINITY) {
            fputs("Error: RTPRIO limit is too low. Set to 'unlimited' via 'ulimit -r' or /etc/security/limits.conf\r\n", stderr);
            exit(EXIT_FAILURE);
        }

        rtprio_limits.rlim_cur = RLIM_INFINITY;
        ERR_CHK(setrlimit(RLIMIT_RTPRIO, &rtprio_limits));
    }

    CPU_IntInit();                                              /* Initialize critical section objects.                 */
}


/*
*********************************************************************************************************
*                                         STATISTIC TASK HOOK
*
* Description: This function is called every second by uC/OS-III's statistics task.  This allows your
*              application to add functionality to the statistics task.
*
* Arguments  : None.
*
* Note(s)    : None.
*********************************************************************************************************
*/

void  OSStatTaskHook (void)
{
#if OS_CFG_APP_HOOKS_EN > 0u
    if (OS_AppStatTaskHookPtr != (OS_APP_HOOK_VOID)0) {
        (*OS_AppStatTaskHookPtr)();
    }
#endif
}


/*
*********************************************************************************************************
*                                         TASK CREATION HOOK
*
* Description: This function is called when a task is created.
*
* Arguments  : p_tcb        Pointer to the task control block of the task being created.
*
* Note(s)    : 1) Interrupts are disabled during this call.
*********************************************************************************************************
*/

void  OSTaskCreateHook (OS_TCB  *p_tcb)
{
    OS_TCB_EXT_POSIX  *p_tcb_ext;
    int                ret;


#if OS_CFG_APP_HOOKS_EN > 0u
    if (OS_AppTaskCreateHookPtr != (OS_APP_HOOK_TCB)0) {
        (*OS_AppTaskCreateHookPtr)(p_tcb);
    }
#endif

    p_tcb_ext = malloc(sizeof(OS_TCB_EXT_POSIX));
    p_tcb->ExtPtr = p_tcb_ext;

    ERR_CHK(sem_init(&p_tcb_ext->InitSem, 0u, 0u));
    ERR_CHK(sem_init(&p_tcb_ext->Sem, 0u, 0u));

    OSThreadCreate(&p_tcb_ext->Thread, OSTaskPosix, p_tcb, THREAD_CREATE_PRIO);

    do {
        ret = sem_wait(&p_tcb_ext->InitSem);                    /* Wait for init.                                       */
        if (ret != 0 && errno != EINTR) {
            raise(SIGABRT);
        }
    } while (ret != 0);
}


/*
*********************************************************************************************************
*                                         TASK DELETION HOOK
*
* Description: This function is called when a task is deleted.
*
* Arguments  : p_tcb        Pointer to the task control block of the task being deleted.
*
* Note(s)    : 1) Interrupts are disabled during this call.
*********************************************************************************************************
*/

void  OSTaskDelHook (OS_TCB  *p_tcb)
{
    OS_TCB_EXT_POSIX  *p_tcb_ext = (OS_TCB_EXT_POSIX *)p_tcb->ExtPtr;
    pthread_t          self;
    CPU_BOOLEAN        same;


#if OS_CFG_APP_HOOKS_EN > 0u
    if (OS_AppTaskDelHookPtr != (OS_APP_HOOK_TCB)0) {
        (*OS_AppTaskDelHookPtr)(p_tcb);
    }
#endif

     self = pthread_self();
     same = (pthread_equal(self, p_tcb_ext->Thread) != 0u);
     if (same != 1u) {
         ERR_CHK(pthread_cancel(p_tcb_ext->Thread));
     }

     OSTaskTerminate(p_tcb);
}


/*
*********************************************************************************************************
*                                          TASK RETURN HOOK
*
* Description: This function is called if a task accidentally returns.  In other words, a task should
*              either be an infinite loop or delete itself when done.
*
* Arguments  : p_tcb        Pointer to the task control block of the task that is returning.
*
* Note(s)    : None.
*********************************************************************************************************
*/

void  OSTaskReturnHook (OS_TCB  *p_tcb)
{
#if OS_CFG_APP_HOOKS_EN > 0u
    if (OS_AppTaskReturnHookPtr != (OS_APP_HOOK_TCB)0) {
        (*OS_AppTaskReturnHookPtr)(p_tcb);
    }
#else
    (void)p_tcb;                                                /* Prevent compiler warning                             */
#endif
}


/*
*********************************************************************************************************
*                                      INITIALIZE A TASK'S STACK
*
* Description: This function is called by OS_Task_Create() or OSTaskCreateExt() to initialize the stack
*              frame of the task being created. This function is highly processor specific.
*
* Arguments  : p_task       Pointer to the task entry point address.
*
*              p_arg        Pointer to a user supplied data area that will be passed to the task
*                               when the task first executes.
*
*              p_stk_base   Pointer to the base address of the stack.
*
*              stk_size     Size of the stack, in number of CPU_STK elements.
*
*              opt          Options used to alter the behavior of OS_Task_StkInit().
*                            (see OS.H for OS_TASK_OPT_xxx).
*
* Returns    : Always returns the location of the new top-of-stack' once the processor registers have
*              been placed on the stack in the proper order.
*********************************************************************************************************
*/

CPU_STK  *OSTaskStkInit (OS_TASK_PTR    p_task __attribute__((__unused__)),
                         void          *p_arg __attribute__((__unused__)),
                         CPU_STK       *p_stk_base,
                         CPU_STK       *p_stk_limit __attribute__((__unused__)),
                         CPU_STK_SIZE   stk_size __attribute__((__unused__)),
                         OS_OPT         opt __attribute__((__unused__)))
{
    return (p_stk_base);
}


/*
*********************************************************************************************************
*                                          TASK SWITCH HOOK
*
* Description: This function is called when a task switch is performed.  This allows you to perform other
*              operations during a context switch.
*
* Arguments  : None.
*
* Note(s)    : 1) Interrupts are disabled during this call.
*              2) It is assumed that the global pointer 'OSTCBHighRdyPtr' points to the TCB of the task
*                 that will be 'switched in' (i.e. the highest priority task) and, 'OSTCBCurPtr' points
*                 to the task being switched out (i.e. the preempted task).
*********************************************************************************************************
*/

void  OSTaskSwHook (void)
{
#if OS_CFG_TASK_PROFILE_EN > 0u
    CPU_TS  ts;
#endif
#ifdef  CPU_CFG_INT_DIS_MEAS_EN
    CPU_TS  int_dis_time;
#endif


#if OS_CFG_APP_HOOKS_EN > 0u
    if (OS_AppTaskSwHookPtr != (OS_APP_HOOK_VOID)0) {
        (*OS_AppTaskSwHookPtr)();
    }
#endif

#if OS_CFG_TASK_PROFILE_EN > 0u
    ts = OS_TS_GET();
    if (OSTCBCurPtr != OSTCBHighRdyPtr) {
        OSTCBCurPtr->CyclesDelta  = ts - OSTCBCurPtr->CyclesStart;
        OSTCBCurPtr->CyclesTotal += (OS_CYCLES)OSTCBCurPtr->CyclesDelta;
    }

    OSTCBHighRdyPtr->CyclesStart = ts;
#endif

#ifdef  CPU_CFG_INT_DIS_MEAS_EN
    int_dis_time = CPU_IntDisMeasMaxCurReset();             /* Keep track of per-task interrupt disable time          */
    if (OSTCBCurPtr->IntDisTimeMax < int_dis_time) {
        OSTCBCurPtr->IntDisTimeMax = int_dis_time;
    }
#endif

#if OS_CFG_SCHED_LOCK_TIME_MEAS_EN > 0u
                                                            /* Keep track of per-task scheduler lock time             */
    if (OSTCBCurPtr->SchedLockTimeMax < (CPU_TS)OSSchedLockTimeMaxCur) {
        OSTCBCurPtr->SchedLockTimeMax = (CPU_TS)OSSchedLockTimeMaxCur;
    }
    OSSchedLockTimeMaxCur = (CPU_TS)0;                      /* Reset the per-task value                               */
#endif
}


/*
*********************************************************************************************************
*                                              TICK HOOK
*
* Description: This function is called every tick.
*
* Arguments  : None.
*
* Note(s)    : 1) This function is assumed to be called from the Tick ISR.
*********************************************************************************************************
*/

void  OSTimeTickHook (void)
{
#if OS_CFG_APP_HOOKS_EN > 0u
    if (OS_AppTimeTickHookPtr != (OS_APP_HOOK_VOID)0) {
        (*OS_AppTimeTickHookPtr)();
    }
#endif
}


/*
*********************************************************************************************************
*                              START HIGHEST PRIORITY TASK READY-TO-RUN
*
* Description: This function is called by OSStart() to start the highest priority task that was created
*              by your application before calling OSStart().
*
* Arguments  : None.
*
* Note(s)    : 1) OSStartHighRdy() MUST:
*                      a) Call OSTaskSwHook() then,
*                      b) Switch to the highest priority task.
*********************************************************************************************************
*/

void  OSStartHighRdy (void)
{
    OS_TCB_EXT_POSIX  *p_tcb_ext;
    sigset_t           sig_set;
    int                signo;


    OSTaskSwHook();

    p_tcb_ext = (OS_TCB_EXT_POSIX *)OSTCBCurPtr->ExtPtr;

    CPU_INT_DIS();

    ERR_CHK(sem_post(&p_tcb_ext->Sem));

    ERR_CHK(sigemptyset(&sig_set));
    ERR_CHK(sigaddset(&sig_set, SIGTERM));
    ERR_CHK(sigwait(&sig_set, &signo));
}


/*
*********************************************************************************************************
*                                      TASK LEVEL CONTEXT SWITCH
*
* Description: This function is called when a task makes a higher priority task ready-to-run.
*
* Arguments  : None.
*
* Note(s)    : 1) Upon entry,
*                 OSTCBCur     points to the OS_TCB of the task to suspend
*                 OSTCBHighRdy points to the OS_TCB of the task to resume
*
*              2) OSCtxSw() MUST:
*                      a) Save processor registers then,
*                      b) Save current task's stack pointer into the current task's OS_TCB,
*                      c) Call OSTaskSwHook(),
*                      d) Set OSTCBCur = OSTCBHighRdy,
*                      e) Set OSPrioCur = OSPrioHighRdy,
*                      f) Switch to the highest priority task.
*
*                      pseudo-code:
*                           void  OSCtxSw (void)
*                           {
*                               Save processor registers;
*
*                               OSTCBCur->OSTCBStkPtr =  SP;
*
*                               OSTaskSwHook();
*
*                               OSTCBCur              =  OSTCBHighRdy;
*                               OSPrioCur             =  OSPrioHighRdy;
*
*                               Restore processor registers from (OSTCBHighRdy->OSTCBStkPtr);
*                           }
*********************************************************************************************************
*/

void  OSCtxSw (void)
{
    OS_TCB_EXT_POSIX  *p_tcb_ext_old;
    OS_TCB_EXT_POSIX  *p_tcb_ext_new;
    int                ret;
    CPU_BOOLEAN        detach = 0u;


    OSTaskSwHook();

    p_tcb_ext_new = (OS_TCB_EXT_POSIX *)OSTCBHighRdyPtr->ExtPtr;
    p_tcb_ext_old = (OS_TCB_EXT_POSIX *)OSTCBCurPtr->ExtPtr;

    if (OSTCBCurPtr->TaskState == OS_TASK_STATE_DEL) {
        detach = 1u;
    }

    OSTCBCurPtr = OSTCBHighRdyPtr;
    OSPrioCur   = OSPrioHighRdy;

    ERR_CHK(sem_post(&p_tcb_ext_new->Sem));

    if (detach == 0u) {
        do {
            ret = sem_wait(&p_tcb_ext_old->Sem);
            if (ret != 0 && errno != EINTR) {
                raise(SIGABRT);
            }
        } while (ret != 0);
    }
}


/*
*********************************************************************************************************
*                                   INTERRUPT LEVEL CONTEXT SWITCH
*
* Description: This function is called by OSIntExit() to perform a context switch from an ISR.
*
* Arguments  : None.
*
* Note(s)    : 1) OSIntCtxSw() MUST:
*                      a) Call OSTaskSwHook() then,
*                      b) Set OSTCBCurPtr = OSTCBHighRdyPtr,
*                      c) Set OSPrioCur   = OSPrioHighRdy,
*                      d) Switch to the highest priority task.
*
*              2) OSIntCurTaskSuspend() MUST be called prior to OSIntEnter().
*
*              3) OSIntCurTaskResume()  MUST be called after    OSIntExit() to switch to the highest
*                 priority task.
*********************************************************************************************************
*/

void  OSIntCtxSw (void)
{
    if (OSTCBCurPtr != OSTCBHighRdyPtr) {
        OSCtxSw();
    }
}

/*
*********************************************************************************************************
*                                         INITIALIZE SYS TICK
*
* Description: Initialize the SysTick.
*
* Arguments  : none.
*
* Note(s)    : 1) This function MUST be called after OSStart() & after processor initialization.
*********************************************************************************************************
*/

void  OS_CPU_SysTickInit (void)
{
    CPU_TmrInterruptCreate(&OSTickTmrInterrupt);
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

static  void  OSTimeTickHandler (void)
{
    OSIntEnter();
    OSTimeTick();
    CPU_ISR_End();
    OSIntExit();
}


/*
*********************************************************************************************************
*                                      OSTaskPosix()
*
* Description: This function is a generic POSIX task wrapper for uC/OS-III tasks.
*
* Arguments  : p_arg        Pointer to argument of the task's TCB.
*
* Note(s)    : 1) Priorities of these tasks are very important.
*********************************************************************************************************
*/

static void  *OSTaskPosix (void  *p_arg)
{
    OS_TCB_EXT_POSIX  *p_tcb_ext;
    OS_TCB            *p_tcb;
    OS_ERR             err;


    p_tcb     = (OS_TCB           *)p_arg;
    p_tcb_ext = (OS_TCB_EXT_POSIX *)p_tcb->ExtPtr;

    p_tcb_ext->ProcessId = (pid_t)syscall(SYS_gettid);
    ERR_CHK(sem_post(&p_tcb_ext->InitSem));

#ifdef OS_CFG_MSG_TRACE_EN
    if (p_tcb->NamePtr != (CPU_CHAR *)0) {
        printf("Task[%3.1d] '%-32s' running\n", p_tcb->Prio, p_tcb->NamePtr);
    }
#endif

    CPU_INT_DIS();
    {
        int ret = -1;
        while (ret != 0u) {
            ret = sem_wait(&p_tcb_ext->Sem);                    /* Wait until first CTX SW.                             */
            if ((ret != 0) && (ret != -EINTR)) {
                ERR_CHK(ret);
            }
        }
    }
    CPU_INT_EN();

#if (OS_CFG_DBG_EN > 0u)
    ((void (*)(void *))p_tcb->TaskEntryAddr)(p_tcb->TaskEntryArg);
#endif

    OSTaskDel(p_tcb, &err);                                     /* Thread may exit at OSCtxSw().                        */

    return (0u);
}


/*
*********************************************************************************************************
*                                          OSTaskTerminate()
*
* Description: This function handles task termination control signals.
*
* Arguments  : p_task       Pointer to the task information structure of the task to clear its control
*                           signals.
*********************************************************************************************************
*/

static  void  OSTaskTerminate (OS_TCB  *p_tcb)
{
#ifdef OS_CFG_MSG_TRACE_EN
    if (p_tcb->NamePtr != (CPU_CHAR *)0) {
        printf("Task[%3.1d] '%-32s' deleted\n", p_tcb->Prio, p_tcb->NamePtr);
    }
#endif

    free(p_tcb->ExtPtr);
}


/*
*********************************************************************************************************
*                                          OSThreadCreate()
*
* Description : Create new posix thread.
*
* Argument(s) : p_thread    Pointer to preallocated thread variable.
*
*               p_task      Pointer to associated function.
*
*               p_arg       Pointer to associated function's argument.
*
*               prio        Thread priority.
*
* Return(s)   : Thread's corresponding LWP pid.
*
* Note(s)     : none.
*
*********************************************************************************************************
*/

static  void  OSThreadCreate (pthread_t  *p_thread,
                            void       *(*p_task) (void *),
                            void         *p_arg,
                            int           prio)
{
    pthread_attr_t       attr;
    struct sched_param   param;


    if (prio < sched_get_priority_min(SCHED_RR) ||
        prio > sched_get_priority_max(SCHED_RR)) {
#ifdef OS_CFG_MSG_TRACE_EN
        printf("ThreadCreate(): Invalid prio arg.\n");
#endif
        raise(SIGABRT);
    }

    ERR_CHK(pthread_attr_init(&attr));
    ERR_CHK(pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED));
    param.__sched_priority = prio;
    ERR_CHK(pthread_attr_setschedpolicy(&attr, SCHED_RR));
    ERR_CHK(pthread_attr_setschedparam(&attr, &param));
    ERR_CHK(pthread_create(p_thread, &attr, p_task, p_arg));
}


#ifdef __cplusplus
}
#endif
