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
*                                          Microsoft Win32 Port
*
* File      : os_cpu_c.c
* Version   : V3.08.01
*********************************************************************************************************
* For       : Win32
* Toolchain : Visual Studio
*********************************************************************************************************
*/

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

#define  _WIN32_WINNT  0x0600
#define   WIN32_LEAN_AND_MEAN

#include  <windows.h>
#include  <mmsystem.h>
#include  <stdio.h>


#ifdef __cplusplus
extern  "C" {
#endif

/*
*********************************************************************************************************
*                                         CONFIGURATION CHECKS
*********************************************************************************************************
*/

#if (OS_CFG_TICK_EN == 0u)
#error "OS_CFG.H, The Win32 port does not support tickless mode. OS_CFG_TICK_EN must be enabled."
#endif


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define  THREAD_EXIT_CODE                           0u      /* Thread exit code returned on task termination.         */

#ifdef  _MSC_VER
#define  MS_VC_EXCEPTION                   0x406D1388u      /* Visual Studio C Exception to change thread name.       */
#endif

#define  WIN_MM_MIN_RES                             1u      /* Minimum timer resolution.                              */


/*
*********************************************************************************************************
*                                          LOCAL DATA TYPES
*********************************************************************************************************
*/

typedef  enum  os_task_state {
    STATE_NONE = 0,
    STATE_CREATED,
    STATE_RUNNING,
    STATE_SUSPENDED,
    STATE_INTERRUPTED,
    STATE_TERMINATING,
    STATE_TERMINATED
} OS_TASK_STATE;


typedef  struct  os_task {
                                                            /* Links all created tasks.                               */
    struct  os_task          *NextPtr;
    struct  os_task          *PrevPtr;

    OS_TCB                   *OSTCBPtr;
    CPU_CHAR                 *OSTaskName;
                                                            /* ---------------- INTERNAL INFORMATION ---------------- */
    void                     *TaskArgPtr;
    OS_OPT                    TaskOpt;
    OS_TASK_PTR               TaskPtr;
    volatile  OS_TASK_STATE   TaskState;
    DWORD                     ThreadID;
    HANDLE                    ThreadHandle;
    HANDLE                    InitSignalPtr;                /* Task created         signal.                           */
    HANDLE                    SignalPtr;                    /* Task synchronization signal.                           */
} OS_TASK;


#ifdef _MSC_VER
#pragma pack(push, 8)
typedef  struct  threadname_info {
    DWORD   dwType;                                         /* Must be 0x1000.                                        */
    LPCSTR  szName;                                         /* Pointer to name (in user addr space).                  */
    DWORD   dwThreadID;                                     /* Thread ID (-1 = caller thread).                        */
    DWORD   dwFlags;                                        /* Reserved for future use, must be zero.                 */
} THREADNAME_INFO;
#pragma pack(pop)
#endif


#if (OS_CFG_TIMER_METHOD_WIN32 == WIN32_MM_TMR)
#ifdef _MSC_VER
#pragma  comment (lib, "winmm.lib")
#endif
#endif


/*
*********************************************************************************************************
*                                           LOCAL VARIABLES
*********************************************************************************************************
*/

static  OS_TASK   *OSTaskListPtr;
static  HANDLE     OSTerminate_SignalPtr;

static  HANDLE     OSTick_Thread;
static  DWORD      OSTick_ThreadId;
#if (OS_CFG_TIMER_METHOD_WIN32 == WIN32_MM_TMR)
static  HANDLE     OSTick_SignalPtr;
static  TIMECAPS   OSTick_TimerCap;
static  MMRESULT   OSTick_TimerId;
#endif


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  DWORD  WINAPI   OSTickW32         (LPVOID     p_arg);
static  DWORD  WINAPI   OSTaskW32         (LPVOID     p_arg);

static  OS_TASK        *OSTaskGet         (OS_TCB    *p_tcb);
static  void            OSTaskTerminate   (OS_TASK   *p_task);

static  BOOL   WINAPI   OSCtrlBreakHandler(DWORD      ctrl);

static  void            OSSetThreadName   (DWORD      thread_id,
                                           CPU_CHAR  *p_name);

#ifdef OS_CFG_MSG_TRACE_EN
static  int             OS_Printf         (char      *p_str, ...);
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

    Sleep(1u);                                              /* Reduce CPU utilization.                                */
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
*
*              2) Kernel objects must have unique names. Otherwise, a duplicate handle will be given for
*                 consecutive created objects. A GetLastError() ERROR_ALREADY_EXISTS can be checked when
*                 this case happens.
*********************************************************************************************************
*/

void  OSInitHook (void)
{
    HANDLE  hProc;


#ifdef OS_CFG_MSG_TRACE_EN
#if (OS_CFG_TIMER_METHOD_WIN32 == WIN32_SLEEP)
    if (OSCfg_TickRate_Hz > 100u) {
        OS_Printf("OS_CFG_TIMER_METHOD_WIN32 Warning: Sleep timer method cannot maintain time accuracy with the current setting of OSCfg_TickRate_Hz (%du). Consider using Multimedia timer method.\n\n",
                  OSCfg_TickRate_Hz);
    }
#endif
#endif

    OSTaskListPtr         = NULL;
    OSTerminate_SignalPtr = NULL;
    OSTick_Thread         = NULL;
#if (OS_CFG_TIMER_METHOD_WIN32 == WIN32_MM_TMR)
    OSTick_SignalPtr      = NULL;
#endif


    CPU_IntInit();                                          /* Initialize Critical Section objects.                   */


    hProc = GetCurrentProcess();
    SetPriorityClass(hProc, HIGH_PRIORITY_CLASS);
    SetProcessAffinityMask(hProc, 1);

    OSSetThreadName(GetCurrentThreadId(), "main()");

                                                            /* Manual reset enabled to broadcast terminate signal.    */
    OSTerminate_SignalPtr = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (OSTerminate_SignalPtr == NULL) {
#ifdef OS_CFG_MSG_TRACE_EN
        OS_Printf("Error: CreateEvent [OSTerminate] failed.\n");
#endif
        return;
    }
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)OSCtrlBreakHandler, TRUE);

    OSTick_Thread = CreateThread(NULL, 0, OSTickW32, 0, CREATE_SUSPENDED, &OSTick_ThreadId);
    if (OSTick_Thread == NULL) {
#ifdef OS_CFG_MSG_TRACE_EN
        OS_Printf("Error: CreateThread [OSTickW32] failed.\n");
#endif
        CloseHandle(OSTerminate_SignalPtr);
        OSTerminate_SignalPtr = NULL;
        return;
    }

#ifdef OS_CFG_MSG_TRACE_EN
    OS_Printf("Thread    '%-32s' Created, Thread ID %5.0d\n",
              "OSTickW32",
              OSTick_ThreadId);
#endif

    SetThreadPriority(OSTick_Thread, THREAD_PRIORITY_HIGHEST);

#if (OS_CFG_TIMER_METHOD_WIN32 == WIN32_MM_TMR)
    if (timeGetDevCaps(&OSTick_TimerCap, sizeof(OSTick_TimerCap)) != TIMERR_NOERROR) {
#ifdef OS_CFG_MSG_TRACE_EN
        OS_Printf("Error: Cannot retrieve Timer capabilities.\n");
#endif
        CloseHandle(OSTick_Thread);
        CloseHandle(OSTerminate_SignalPtr);

        OSTick_Thread         = NULL;
        OSTerminate_SignalPtr = NULL;
        return;
    }

    if (OSTick_TimerCap.wPeriodMin < WIN_MM_MIN_RES) {
        OSTick_TimerCap.wPeriodMin = WIN_MM_MIN_RES;
    }

    if (timeBeginPeriod(OSTick_TimerCap.wPeriodMin) != TIMERR_NOERROR) {
#ifdef OS_CFG_MSG_TRACE_EN
        OS_Printf("Error: Cannot set Timer minimum resolution.\n");
#endif
        CloseHandle(OSTick_Thread);
        CloseHandle(OSTerminate_SignalPtr);

        OSTick_Thread         = NULL;
        OSTerminate_SignalPtr = NULL;
        return;
    }

    OSTick_SignalPtr = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (OSTick_SignalPtr == NULL) {
#ifdef OS_CFG_MSG_TRACE_EN
        OS_Printf("Error: CreateEvent [OSTick] failed.\n");
#endif
        timeEndPeriod(OSTick_TimerCap.wPeriodMin);
        CloseHandle(OSTick_Thread);
        CloseHandle(OSTerminate_SignalPtr);

        OSTick_Thread         = NULL;
        OSTerminate_SignalPtr = NULL;
        return;
    }

#ifdef _MSC_VER
#pragma warning (disable : 4055)
#endif
    OSTick_TimerId = timeSetEvent((UINT          )(1000u / OSCfg_TickRate_Hz),
                                  (UINT          ) OSTick_TimerCap.wPeriodMin,
                                  (LPTIMECALLBACK) OSTick_SignalPtr,
                                  (DWORD_PTR     ) NULL,
                                  (UINT          )(TIME_PERIODIC | TIME_CALLBACK_EVENT_SET));
#ifdef _MSC_VER
#pragma warning (default : 4055)
#endif

    if (OSTick_TimerId == 0u) {
#ifdef OS_CFG_MSG_TRACE_EN
        OS_Printf("Error: Cannot start Timer.\n");
#endif
        CloseHandle(OSTick_SignalPtr);
        timeEndPeriod(OSTick_TimerCap.wPeriodMin);
        CloseHandle(OSTick_Thread);
        CloseHandle(OSTerminate_SignalPtr);

        OSTick_SignalPtr      = NULL;
        OSTick_Thread         = NULL;
        OSTerminate_SignalPtr = NULL;
        return;
    }
#endif
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
*
*              2) Kernel objects must have unique names. Otherwise, a duplicate handle will be given for
*                 consecutive created objects. A GetLastError() ERROR_ALREADY_EXISTS can be checked when
*                 this case happens.
*********************************************************************************************************
*/

void  OSTaskCreateHook (OS_TCB  *p_tcb)
{
    OS_TASK  *p_task;
    CPU_SR_ALLOC();


#if OS_CFG_APP_HOOKS_EN > 0u
    if (OS_AppTaskCreateHookPtr != (OS_APP_HOOK_TCB)0) {
        (*OS_AppTaskCreateHookPtr)(p_tcb);
    }
#endif

    p_task             = OSTaskGet(p_tcb);
#if OS_CFG_DBG_EN > 0u
    p_task->OSTaskName = p_tcb->NamePtr;
#else
	p_task->OSTaskName = "";
#endif
                                                            /* See Note #2.                                           */
    p_task->SignalPtr = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (p_task->SignalPtr == NULL) {
#ifdef OS_CFG_MSG_TRACE_EN
        OS_Printf("Task[%3.1d] '%s' cannot allocate signal event.\n",
                  p_tcb->Prio,
                  p_task->OSTaskName);
#endif
        return;
    }
                                                            /* See Note #2.                                           */
    p_task->InitSignalPtr = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (p_task->InitSignalPtr == NULL) {
        CloseHandle(p_task->SignalPtr);
        p_task->SignalPtr = NULL;

#ifdef OS_CFG_MSG_TRACE_EN
        OS_Printf("Task[%3.1d] '%s' cannot allocate initialization complete signal event.\n",
                  p_tcb->Prio,
                  p_task->OSTaskName);
#endif
        return;
    }

    p_task->ThreadHandle = CreateThread(NULL, 0, OSTaskW32, p_tcb, CREATE_SUSPENDED, &p_task->ThreadID);
    if (p_task->ThreadHandle == NULL) {
        CloseHandle(p_task->InitSignalPtr);
        CloseHandle(p_task->SignalPtr);

        p_task->InitSignalPtr = NULL;
        p_task->SignalPtr     = NULL;
#ifdef OS_CFG_MSG_TRACE_EN
        OS_Printf("Task[%3.1d] '%s' failed to be created.\n",
                  p_tcb->Prio,
                  p_task->OSTaskName);
#endif
        return;
    }

#ifdef OS_CFG_MSG_TRACE_EN
    OS_Printf("Task[%3.1d] '%-32s' Created, Thread ID %5.0d\n",
              p_tcb->Prio,
              p_task->OSTaskName,
              p_task->ThreadID);
#endif

    p_task->TaskState = STATE_CREATED;
    p_task->OSTCBPtr  = p_tcb;

    CPU_CRITICAL_ENTER();
    p_task->PrevPtr     = (OS_TASK *)0;
    if (OSTaskListPtr  == (OS_TASK *)0) {
        p_task->NextPtr = (OS_TASK *)0;
    } else {
        p_task->NextPtr        = OSTaskListPtr;
        OSTaskListPtr->PrevPtr = p_task;
    }
    OSTaskListPtr = p_task;
    CPU_CRITICAL_EXIT();
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
    OS_TASK  *p_task;


#if OS_CFG_APP_HOOKS_EN > 0u
    if (OS_AppTaskDelHookPtr != (OS_APP_HOOK_TCB)0) {
        (*OS_AppTaskDelHookPtr)(p_tcb);
    }
#endif

    p_task = OSTaskGet(p_tcb);

    if (p_task == (OS_TASK *)0) {
        return;
    }

    switch (p_task->TaskState) {
        case STATE_RUNNING:
             if (GetCurrentThreadId() == p_task->ThreadID) {
                p_task->TaskState = STATE_TERMINATING;

             } else {

                 TerminateThread(p_task->ThreadHandle, THREAD_EXIT_CODE);
                 CloseHandle(p_task->ThreadHandle);

                 OSTaskTerminate(p_task);
             }
             break;


        case STATE_CREATED:
        case STATE_SUSPENDED:
        case STATE_INTERRUPTED:
             TerminateThread(p_task->ThreadHandle, THREAD_EXIT_CODE);
             CloseHandle(p_task->ThreadHandle);

             OSTaskTerminate(p_task);
             break;


        default:
             break;
    }
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
    (void)p_tcb;                                            /* Prevent compiler warning                               */
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

CPU_STK  *OSTaskStkInit (OS_TASK_PTR    p_task,
                         void          *p_arg,
                         CPU_STK       *p_stk_base,
                         CPU_STK       *p_stk_limit,
                         CPU_STK_SIZE   stk_size,
                         OS_OPT         opt)
{
    OS_TASK  *p_task_info;


    (void)p_stk_limit;                                      /* Prevent compiler warning                               */
    (void)stk_size;

                                                            /* Create task info struct into task's stack.             */
	p_task_info                = (OS_TASK *)(&p_stk_base[stk_size] - sizeof(OS_TASK));

    p_task_info->NextPtr       =  NULL;
    p_task_info->PrevPtr       =  NULL;
    p_task_info->OSTCBPtr      =  NULL;
    p_task_info->OSTaskName    =  NULL;

    p_task_info->TaskArgPtr    =  p_arg;
    p_task_info->TaskOpt       =  opt;
    p_task_info->TaskPtr       =  p_task;
    p_task_info->TaskState     =  STATE_NONE;

    p_task_info->ThreadID      =  0u;
    p_task_info->ThreadHandle  =  NULL;
    p_task_info->InitSignalPtr =  NULL;
    p_task_info->SignalPtr     =  NULL;

	return ((CPU_STK*)p_task_info);
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
    OS_TASK  *p_task;
    OS_TCB   *p_tcb;
    OS_ERR    err;
    CPU_SR_ALLOC();


    OSTaskSwHook();

    p_task = OSTaskGet(OSTCBHighRdyPtr);
    ResumeThread(p_task->ThreadHandle);
                                                            /* Wait while task is created and ready to run.           */
    SignalObjectAndWait(p_task->SignalPtr, p_task->InitSignalPtr, INFINITE, FALSE);
    ResumeThread(OSTick_Thread);                            /* Start OSTick Thread.                                   */
    WaitForSingleObject(OSTick_Thread, INFINITE);           /* Wait until OSTick Thread has terminated.               */


#if (OS_CFG_TIMER_METHOD_WIN32 == WIN32_MM_TMR)
    timeKillEvent(OSTick_TimerId);
    timeEndPeriod(OSTick_TimerCap.wPeriodMin);
    CloseHandle(OSTick_SignalPtr);
#endif

    CloseHandle(OSTick_Thread);
    CloseHandle(OSTerminate_SignalPtr);


#ifdef OS_CFG_MSG_TRACE_EN
    OS_Printf("\nDeleting uC/OS-III tasks...\n");
#endif
                                                            /* Delete all created tasks/threads.                      */
    OSSchedLock(&err);

    CPU_CRITICAL_ENTER();
    p_task = OSTaskListPtr;
    while (p_task != NULL) {
        p_tcb = p_task->OSTCBPtr;
        p_task = p_task->NextPtr;

        if (p_tcb == &OSIdleTaskTCB) {
            OSTaskDelHook(p_tcb);
        } else {
            OSTaskDel(p_tcb, &err);
        }

        Sleep(1);                                           /* Allow thread to be deleted.                            */
    }
    CPU_CRITICAL_EXIT();

#if 0                                                       /* Prevent scheduler from running when exiting app.       */
    OSSchedUnlock(&err);
#endif

    CPU_IntEnd();                                           /* Delete Critical Section objects.                       */
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
    OS_TASK  *p_task_cur;
    OS_TASK  *p_task_new;
#ifdef OS_CFG_MSG_TRACE_EN
    OS_TCB   *p_tcb_cur;
    OS_TCB   *p_tcb_new;
#endif
    CPU_SR_ALLOC();


#if (CPU_CFG_CRITICAL_METHOD == CPU_CRITICAL_METHOD_STATUS_LOCAL)
    cpu_sr = 0;
#endif


#ifdef OS_CFG_MSG_TRACE_EN
    p_tcb_cur = OSTCBCurPtr;
    p_tcb_new = OSTCBHighRdyPtr;
#endif


    p_task_cur = OSTaskGet(OSTCBCurPtr);

    OSTaskSwHook();

    OSTCBCurPtr = OSTCBHighRdyPtr;
    OSPrioCur   = OSPrioHighRdy;

    if (p_task_cur->TaskState == STATE_RUNNING) {
        p_task_cur->TaskState  = STATE_SUSPENDED;
    }
    p_task_new = OSTaskGet(OSTCBHighRdyPtr);
    switch (p_task_new->TaskState) {
        case STATE_CREATED:                                 /* TaskState updated to STATE_RUNNING once thread runs.   */
             ResumeThread(p_task_new->ThreadHandle);
                                                            /* Wait while task is created and ready to run.           */
             SignalObjectAndWait(p_task_new->SignalPtr, p_task_new->InitSignalPtr, INFINITE, FALSE);
             break;


        case STATE_SUSPENDED:
             p_task_new->TaskState = STATE_RUNNING;
             SetEvent(p_task_new->SignalPtr);
             break;


        case STATE_INTERRUPTED:
             p_task_new->TaskState = STATE_RUNNING;
             ResumeThread(p_task_new->ThreadHandle);
             break;


#ifdef OS_CFG_MSG_TRACE_EN
        case STATE_NONE:
             OS_Printf("[OSCtxSw] Error: Invalid state STATE_NONE\nCur    Task[%3.1d] Thread ID %5.0d: '%s'\nNew    Task[%3.1d] Thread ID %5.0d: '%s'\n\n",
                       p_tcb_cur->Prio,
                       p_task_cur->ThreadID,
                       p_task_cur->OSTaskName,
                       p_tcb_new->Prio,
                       p_task_new->ThreadID,
                       p_task_new->OSTaskName);
             return;


        case STATE_RUNNING:
             OS_Printf("[OSCtxSw] Error: Invalid state STATE_RUNNING\nCur    Task[%3.1d] Thread ID %5.0d: '%s'\nNew    Task[%3.1d] Thread ID %5.0d: '%s'\n\n",
                       p_tcb_cur->Prio,
                       p_task_cur->ThreadID,
                       p_task_cur->OSTaskName,
                       p_tcb_new->Prio,
                       p_task_new->ThreadID,
                       p_task_new->OSTaskName);
             return;


        case STATE_TERMINATING:
             OS_Printf("[OSCtxSw] Error: Invalid state STATE_TERMINATING\nCur    Task[%3.1d] Thread ID %5.0d: '%s'\nNew    Task[%3.1d] Thread ID %5.0d: '%s'\n\n",
                       p_tcb_cur->Prio,
                       p_task_cur->ThreadID,
                       p_task_cur->OSTaskName,
                       p_tcb_new->Prio,
                       p_task_new->ThreadID,
                       p_task_new->OSTaskName);
             return;


        case STATE_TERMINATED:
             OS_Printf("[OSCtxSw] Error: Invalid state STATE_TERMINATED\nCur    Task[%3.1d] Thread ID %5.0d: '%s'\nNew    Task[%3.1d] Thread ID %5.0d: '%s'\n\n",
                       p_tcb_cur->Prio,
                       p_task_cur->ThreadID,
                       p_task_cur->OSTaskName,
                       p_tcb_new->Prio,
                       p_task_new->ThreadID,
                       p_task_new->OSTaskName);
             return;


#endif
        default:
             return;
    }


    if (p_task_cur->TaskState == STATE_TERMINATING) {
        OSTaskTerminate(p_task_cur);

        CPU_CRITICAL_EXIT();

        ExitThread(THREAD_EXIT_CODE);                       /* ExitThread() never returns.                            */
        return;
    }
    CPU_CRITICAL_EXIT();
    WaitForSingleObject(p_task_cur->SignalPtr, INFINITE);
    CPU_CRITICAL_ENTER();
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
    OSTaskSwHook();

    OSTCBCurPtr = OSTCBHighRdyPtr;
    OSPrioCur   = OSPrioHighRdy;
}


/*
*********************************************************************************************************
*                                        OSIntCurTaskSuspend()
*
* Description: This function suspends current task for context switch.
*
* Arguments  : None.
*
* Returns    : OS_TRUE,  current task     suspended successfully.
*              OS_FALSE, current task NOT suspended.
*
* Notes      : 1) Current task MUST be suspended before OSIntEnter().
*
*              2) Suspending current task before OSIntEnter() and resuming it after OSIntExit() prevents
*                 task-level code to run concurrently with ISR-level code
*********************************************************************************************************
*/

CPU_BOOLEAN  OSIntCurTaskSuspend (void)
{
    OS_TCB       *p_tcb;
    OS_TASK      *p_task;
    CPU_BOOLEAN   ret;


    p_tcb =  OSTCBCurPtr;
    p_task = OSTaskGet(p_tcb);
    switch (p_task->TaskState) {
        case STATE_RUNNING:
             SuspendThread(p_task->ThreadHandle);
             SwitchToThread();

             p_task->TaskState = STATE_INTERRUPTED;

             ret = OS_TRUE;
             break;


        case STATE_TERMINATING:                             /* Task terminated (run-to-completion or deleted itself). */
             TerminateThread(p_task->ThreadHandle, THREAD_EXIT_CODE);
             CloseHandle(p_task->ThreadHandle);

             OSTaskTerminate(p_task);

             ret = OS_TRUE;
             break;


#ifdef OS_CFG_MSG_TRACE_EN
        case STATE_NONE:
             OS_Printf("[OSIntCtxSw Suspend] Error: Invalid state STATE_NONE\nCur    Task[%3.1d] '%s' Thread ID %5.0d\n",
                       p_tcb->Prio,
                       p_task->OSTaskName,
                       p_task->ThreadID);

             ret = OS_FALSE;
             break;


        case STATE_CREATED:
             OS_Printf("[OSIntCtxSw Suspend] Error: Invalid state STATE_CREATED\nCur    Task[%3.1d] '%s' Thread ID %5.0d\n",
                       p_tcb->Prio,
                       p_task->OSTaskName,
                       p_task->ThreadID);

             ret = OS_FALSE;
             break;


        case STATE_INTERRUPTED:
             OS_Printf("[OSIntCtxSw Suspend] Error: Invalid state STATE_INTERRUPTED\nCur    Task[%3.1d] '%s' Thread ID %5.0d\n",
                       p_tcb->Prio,
                       p_task->OSTaskName,
                       p_task->ThreadID);

             ret = OS_FALSE;
             break;


        case STATE_SUSPENDED:
             OS_Printf("[OSIntCtxSw Suspend] Error: Invalid state STATE_SUSPENDED\nCur    Task[%3.1d] '%s' Thread ID %5.0d\n",
                       p_tcb->Prio,
                       p_task->OSTaskName,
                       p_task->ThreadID);

             ret = OS_FALSE;
             break;


        case STATE_TERMINATED:
             OS_Printf("[OSIntCtxSw Suspend] Error: Invalid state STATE_TERMINATED\nCur    Task[%3.1d] '%s' Thread ID %5.0d\n",
                       p_tcb->Prio,
                       p_task->OSTaskName,
                       p_task->ThreadID);

             ret = OS_FALSE;
             break;


#endif
        default:
             ret = OS_FALSE;
             break;
    }

    return (ret);
}


/*
*********************************************************************************************************
*                                        OSIntCurTaskResume()
*
* Description: This function resumes current task for context switch.
*
* Arguments  : None.
*
* Returns    : OS_TRUE,  current task     resumed successfully.
*              OS_FALSE, current task NOT resumed.
*
* Notes      : 1) Current task MUST be resumed after OSIntExit().
*
*              2) Suspending current task before OSIntEnter() and resuming it after OSIntExit() prevents
*                 task-level code to run concurrently with ISR-level code
*********************************************************************************************************
*/

CPU_BOOLEAN  OSIntCurTaskResume (void)
{
    OS_TCB       *p_tcb;
    OS_TASK      *p_task;
    CPU_BOOLEAN   ret;


    p_tcb  = OSTCBHighRdyPtr;
    p_task = OSTaskGet(p_tcb);
    switch (p_task->TaskState) {
        case STATE_CREATED:
             ResumeThread(p_task->ThreadHandle);
                                                            /* Wait while task is created and ready to run.           */
             SignalObjectAndWait(p_task->SignalPtr, p_task->InitSignalPtr, INFINITE, FALSE);
             ret = OS_TRUE;
             break;


        case STATE_INTERRUPTED:
             p_task->TaskState = STATE_RUNNING;
             ResumeThread(p_task->ThreadHandle);
             ret = OS_TRUE;
             break;


        case STATE_SUSPENDED:
             p_task->TaskState = STATE_RUNNING;
             SetEvent(p_task->SignalPtr);
             ret = OS_TRUE;
             break;


#ifdef OS_CFG_MSG_TRACE_EN
        case STATE_NONE:
             OS_Printf("[OSIntCtxSw Resume] Error: Invalid state STATE_NONE\nNew    Task[%3.1d] '%s' Thread ID %5.0d\n",
                       p_tcb->Prio,
                       p_task->OSTaskName,
                       p_task->ThreadID);
             ret = OS_FALSE;
             break;


        case STATE_RUNNING:
             OS_Printf("[OSIntCtxSw Resume] Error: Invalid state STATE_RUNNING\nNew    Task[%3.1d] '%s' Thread ID %5.0d\n",
                       p_tcb->Prio,
                       p_task->OSTaskName,
                       p_task->ThreadID);
             ret = OS_FALSE;
             break;


        case STATE_TERMINATING:
             OS_Printf("[OSIntCtxSw Resume] Error: Invalid state STATE_TERMINATING\nNew    Task[%3.1d] '%s' Thread ID %5.0d\n",
                       p_tcb->Prio,
                       p_task->OSTaskName,
                       p_task->ThreadID);
             ret = OS_FALSE;
             break;


        case STATE_TERMINATED:
             OS_Printf("[OSIntCtxSw Resume] Error: Invalid state STATE_TERMINATED\nNew    Task[%3.1d] '%s' Thread ID %5.0d\n",
                       p_tcb->Prio,
                       p_task->OSTaskName,
                       p_task->ThreadID);
             ret = OS_FALSE;
             break;


#endif
        default:
             ret = OS_FALSE;
             break;
    }

    return (ret);
}


/*
*********************************************************************************************************
*                                      WIN32 TASK - OSTickW32()
*
* Description: This functions is the Win32 task that generates the tick interrupts for uC/OS-III.
*
* Arguments  : p_arg        Pointer to argument of the task.
*
* Note(s)    : 1) Priorities of these tasks are very important.
*********************************************************************************************************
*/

static  DWORD  WINAPI  OSTickW32 (LPVOID  p_arg)
{
    CPU_BOOLEAN  terminate;
    CPU_BOOLEAN  suspended;
#if (OS_CFG_TIMER_METHOD_WIN32 == WIN32_MM_TMR)
    HANDLE       wait_signal[2];
#endif
    CPU_SR_ALLOC();


#if (OS_CFG_TIMER_METHOD_WIN32 == WIN32_MM_TMR)
    wait_signal[0] = OSTerminate_SignalPtr;
    wait_signal[1] = OSTick_SignalPtr;
#endif


    (void)p_arg;                                            /* Prevent compiler warning                               */

    terminate = OS_FALSE;
    while (!terminate) {
#if   (OS_CFG_TIMER_METHOD_WIN32 == WIN32_MM_TMR)
        switch (WaitForMultipleObjects(2, wait_signal, FALSE, INFINITE)) {
            case WAIT_OBJECT_0 + 1u:
                 ResetEvent(OSTick_SignalPtr);
#elif (OS_CFG_TIMER_METHOD_WIN32 == WIN32_SLEEP)
        switch (WaitForSingleObject(OSTerminate_SignalPtr, 1000u / OSCfg_TickRate_Hz)) {
            case WAIT_TIMEOUT:
#endif
                 CPU_CRITICAL_ENTER();

                 suspended = OSIntCurTaskSuspend();
                 if (suspended == OS_TRUE) {
                     OSIntEnter();
                     OSTimeTick();
                     OSIntExit();
                     OSIntCurTaskResume();
                 }

                 CPU_CRITICAL_EXIT();
                 break;


            case WAIT_OBJECT_0 + 0u:
                 terminate = OS_TRUE;
                 break;


            default:
#ifdef OS_CFG_MSG_TRACE_EN
                 OS_Printf("Thread    '%-32s' Error: Invalid signal.\n", "OSTickW32");
#endif
                 terminate = OS_TRUE;
                 break;
        }
    }

#ifdef OS_CFG_MSG_TRACE_EN
    OS_Printf("Thread    '%-32s' Terminated.\n", "OSTickW32");
#endif

    return (0u);
}


/*
*********************************************************************************************************
*                                      WIN32 TASK - OSTaskW32()
*
* Description: This function is a generic Win32 task wrapper for uC/OS-III tasks.
*
* Arguments  : p_arg        Pointer to argument of the task.
*
* Note(s)    : 1) Priorities of these tasks are very important.
*********************************************************************************************************
*/

static  DWORD  WINAPI  OSTaskW32 (LPVOID  p_arg)
{
    OS_TASK  *p_task;
    OS_TCB   *p_tcb;
    OS_ERR    err;


    p_tcb  = (OS_TCB *)p_arg;
    p_task =  OSTaskGet(p_tcb);

    p_task->TaskState = STATE_SUSPENDED;
    WaitForSingleObject(p_task->SignalPtr, INFINITE);

    OSSetThreadName(p_task->ThreadID, p_task->OSTaskName);

#ifdef OS_CFG_MSG_TRACE_EN
    OS_Printf("Task[%3.1d] '%-32s' Running\n",
              p_tcb->Prio,
              p_task->OSTaskName);
#endif

    p_task->TaskState = STATE_RUNNING;
    SetEvent(p_task->InitSignalPtr);                        /* Indicate task has initialized successfully.            */

    p_task->TaskPtr(p_task->TaskArgPtr);

    OSTaskDel(p_tcb, &err);                                 /* Thread may exit at OSCtxSw().                          */

    return (0u);
}


/*
*********************************************************************************************************
*                                             OSTaskGet()
*
* Description: This function retrieve the task information structure associated with a task control block.
*
* Arguments  : p_tcb        Pointer to the task control block to retrieve the task information structure.
*********************************************************************************************************
*/

static  OS_TASK  *OSTaskGet (OS_TCB  *p_tcb)
{
    OS_TASK  *p_task;


    p_task = (OS_TASK *)p_tcb->StkPtr;                      /* Ptr to task info struct is stored into TCB's .StkPtr.  */
    if (p_task != NULL) {
        return (p_task);
    }

    p_task = OSTaskListPtr;                                 /* Task info struct not in TCB's .StkPtr.                 */
    while (p_task != NULL) {                                /* Search all tasks.                                      */
        if (p_task->OSTCBPtr == p_tcb) {
            return (p_task);
        }
        p_task = p_task->NextPtr;
    }

    return (NULL);
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

static  void  OSTaskTerminate (OS_TASK  *p_task)
{
#ifdef OS_CFG_MSG_TRACE_EN
    OS_TCB   *p_tcb;
#endif
    OS_TASK  *p_task_next;
    OS_TASK  *p_task_prev;
    CPU_SR_ALLOC();


#ifdef OS_CFG_MSG_TRACE_EN
    p_tcb = p_task->OSTCBPtr;
    if (p_tcb->Prio != OS_PRIO_INIT) {
        OS_Printf("Task[%3.1d] '%-32s' Deleted\n",
                  p_tcb->Prio,
                  p_task->OSTaskName);
    } else {
        OS_Printf("Task      '%-32s' Deleted\n",
                  p_task->OSTaskName);
    }
#endif
    CloseHandle(p_task->InitSignalPtr);
    CloseHandle(p_task->SignalPtr);

    p_task->OSTCBPtr      = NULL;
    p_task->OSTaskName    = NULL;
    p_task->TaskArgPtr    = NULL;
    p_task->TaskOpt       = OS_OPT_NONE;
    p_task->TaskPtr       = NULL;
    p_task->TaskState     = STATE_TERMINATED;
    p_task->ThreadID      = 0u;
    p_task->ThreadHandle  = NULL;
    p_task->InitSignalPtr = NULL;
    p_task->SignalPtr     = NULL;

    CPU_CRITICAL_ENTER();
    p_task_prev = p_task->PrevPtr;
    p_task_next = p_task->NextPtr;

    if (p_task_prev == (OS_TASK *)0) {
        OSTaskListPtr = p_task_next;
        if (p_task_next != (OS_TASK *)0) {
            p_task_next->PrevPtr = (OS_TASK *)0;
        }
        p_task->NextPtr = (OS_TASK *)0;

    } else if (p_task_next == (OS_TASK *)0) {
        p_task_prev->NextPtr = (OS_TASK *)0;
        p_task->PrevPtr      = (OS_TASK *)0;

    } else {
        p_task_prev->NextPtr =  p_task_next;
        p_task_next->PrevPtr =  p_task_prev;
        p_task->NextPtr      = (OS_TASK *)0;
        p_task->PrevPtr      = (OS_TASK *)0;
    }
    CPU_CRITICAL_EXIT();
}


/*
*********************************************************************************************************
*                                        OSCtrlBreakHandler()
*
* Description: This function handles control signals sent to the console window.
*
* Arguments  : ctrl         Control signal type.
*
* Returns    : TRUE,  control signal was     handled.
*              FALSE, control signal was NOT handled.
*********************************************************************************************************
*/

static  BOOL  WINAPI  OSCtrlBreakHandler (DWORD  ctrl)
{
    BOOL  ret;


    ret = FALSE;

    switch (ctrl) {
        case CTRL_C_EVENT:                                  /* CTRL-C pressed.                                        */
        case CTRL_BREAK_EVENT:                              /* CTRL-BREAK pressed.                                    */
        case CTRL_CLOSE_EVENT:                              /* Console window is closing.                             */
        case CTRL_LOGOFF_EVENT:                             /* Logoff has started.                                    */
        case CTRL_SHUTDOWN_EVENT:                           /* System shutdown in process.                            */
#ifdef OS_CFG_MSG_TRACE_EN
             OS_Printf("\nTerminating Scheduler...\n");
#endif
             SetEvent(OSTerminate_SignalPtr);

             if (ctrl == CTRL_CLOSE_EVENT) {
                 Sleep(500);                                /* Give a chance to OSTickW32 to terminate.               */
             } else {
                 ret = TRUE;
             }
             break;


        default:
             break;
    }

    return (ret);
}


/*
*********************************************************************************************************
*                                             OS_Printf()
*
* Description: This function is analog of printf.
*
* Arguments  : p_str        Pointer to format string output.
*
* Returns    : Number of characters written.
*********************************************************************************************************
*/
#ifdef OS_CFG_MSG_TRACE_EN
static  int  OS_Printf (char  *p_str, ...)
{
    va_list  param;
    int      ret;


    va_start(param, p_str);
#ifdef _MSC_VER
    ret = vprintf_s(p_str, param);
#else
    ret = vprintf(p_str, param);
#endif
    va_end(param);

    return (ret);
}
#endif


/*
*********************************************************************************************************
*                                          OSDebuggerBreak()
*
* Description: This function throws a breakpoint exception when a debugger is present.
*
* Arguments  : None.
*********************************************************************************************************
*/

void  OSDebuggerBreak (void)
{
#ifdef _MSC_VER
    __try {
        DebugBreak();
    }
    __except(GetExceptionCode() == EXCEPTION_BREAKPOINT ?
                                   EXCEPTION_EXECUTE_HANDLER :
                                   EXCEPTION_CONTINUE_SEARCH) {
        return;
    }
#else
#ifdef _DEBUG
    DebugBreak();
#endif
#endif
}


/*
*********************************************************************************************************
*                                          OSSetThreadName()
*
* Description: This function sets thread names.
*
* Arguments  : thread_id    Thread ID.
*
*              p_name       Pointer to name of the thread string.
*********************************************************************************************************
*/

static  void  OSSetThreadName (DWORD  thread_id, CPU_CHAR  *p_name)
{
#ifdef _MSC_VER
    THREADNAME_INFO  info;


    info.dwType     = (DWORD )0x1000u;
    info.szName     = (LPCSTR)p_name;
    info.dwThreadID = (DWORD )thread_id;
    info.dwFlags    = (DWORD )0u;

    __try {
        RaiseException(MS_VC_EXCEPTION, 0u, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info);
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
    }
#endif
}


#ifdef __cplusplus
}
#endif
