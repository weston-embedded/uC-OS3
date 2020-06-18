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
* File    : os_cpu_a.S
* Version : V3.08.00
*********************************************************************************************************
*/

/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2009 Altera Corporation, San Jose, California, USA.           *
* All rights reserved.                                                        *
*                                                                             *
* Permission is hereby granted, free of charge, to any person obtaining a     *
* copy of this software and associated documentation files (the "Software"),  *
* to deal in the Software without restriction, including without limitation   *
* the rights to use, copy, modify, merge, publish, distribute, sublicense,    *
* and/or sell copies of the Software, and to permit persons to whom the       *
* Software is furnished to do so, subject to the following conditions:        *
*                                                                             *
* The above copyright notice and this permission notice shall be included in  *
* all copies or substantial portions of the Software.                         *
*                                                                             *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,    *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE *
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING     *
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER         *
* DEALINGS IN THE SOFTWARE.                                                   *
*                                                                             *
* This agreement shall be governed in all respects by the laws of the State   *
* of California and by the laws of the United States of America.              *
*                                                                             *
* Altera does not recommend, suggest or require that this reference design    *
* file be used in conjunction or combination with any other product.          *
******************************************************************************/

#include "os_cfg.h"

        .text

/*********************************************************************************************************
 *                                PERFORM A CONTEXT SWITCH
 *                                           void OSCtxSw(void)    - from task level
 *                                           void OSIntCtxSw(void) - from interrupt level
 *
 * Note(s): 1) Upon entry,
 *             OSTCBCurPtr      points to the OS_TCB of the task to suspend
 *             OSTCBHighRdyPtr  points to the OS_TCB of the task to resume
 *
 *********************************************************************************************************/
        .global OSIntCtxSw
        .global OSCtxSw

OSIntCtxSw:
OSCtxSw:

      /*
       * Save the remaining registers to the stack.
       */

      addi sp, sp, -44

#ifdef ALT_STACK_CHECK

      bltu  sp, et, .Lstack_overflow

#endif

#if (OS_THREAD_SAFE_NEWLIB > 0u)
      ldw r3, %gprel(_impure_ptr)(gp)   /* load the pointer */
#endif /* OS_THREAD_SAFE_NEWLIB */

      ldw r4, %gprel(OSTCBCurPtr)(gp)

      stw ra,  0(sp)
      stw fp,  4(sp)
      stw r23, 8(sp)
      stw r22, 12(sp)
      stw r21, 16(sp)
      stw r20, 20(sp)
      stw r19, 24(sp)
      stw r18, 28(sp)
      stw r17, 32(sp)
      stw r16, 36(sp)

#if (OS_THREAD_SAFE_NEWLIB > 0u)
      /*
       * store the current value of _impure_ptr so it can be restored
       * later; _impure_ptr is asigned on a per task basis. It is used
       * by Newlib to achieve reentrancy.
       */

      stw r3, 40(sp)                  /* save the impure pointer */
#endif /* OS_THREAD_SAFE_NEWLIB */

      /*
       * Save the current tasks stack pointer into the current tasks OS_TCB.
       * i.e. OSTCBCurPtr->OSTCBStkPtr = sp;
       */

      stw sp, (r4)                  /* save the stack pointer (OSTCBStkPtr */
                                    /* is the first element in the OS_TCB  */
                                    /* structure.                          */

      /*
       * Call the user definable OSTaskSWHook()
       */

      call OSTaskSwHook

0:

9:

      /*
       * OSTCBCurPtr = OSTCBHighRdyPtr;
       * OSPrioCur   = OSPrioHighRdy;
       */

      ldw r4, %gprel(OSTCBHighRdyPtr)(gp)
      ldb r5, %gprel(OSPrioHighRdy)(gp)

      stw r4, %gprel(OSTCBCurPtr)(gp)  /* set the current task to be the new task */
      stb r5, %gprel(OSPrioCur)(gp)    /* store the new task's priority as the current */
                                       /* task's priority                              */

      /*
       * Set the stack pointer to point to the new task's stack
       */

      ldw sp, (r4) /* the stack pointer is the first entry in the OS_TCB structure */

#if defined(ALT_STACK_CHECK) && (OS_TASK_CREATE_EXT_EN > 0)

      ldw et, 8(r4)                  /* load the new stack limit */

#endif

#if (OS_THREAD_SAFE_NEWLIB > 0u)
      /*
       * restore the value of _impure_ptr ; _impure_ptr is asigned on a
       * per task basis. It is used by Newlib to achieve reentrancy.
       */

      ldw r3, 40(sp)                  /* load the new impure pointer */
#endif /* OS_THREAD_SAFE_NEWLIB */

      /*
       * Restore the saved registers for the new task.
       */

      ldw ra,  0(sp)
      ldw fp,  4(sp)
      ldw r23, 8(sp)
      ldw r22, 12(sp)
      ldw r21, 16(sp)
      ldw r20, 20(sp)
      ldw r19, 24(sp)
      ldw r18, 28(sp)
      ldw r17, 32(sp)
      ldw r16, 36(sp)

#if (OS_THREAD_SAFE_NEWLIB > 0u)

      stw r3, %gprel(_impure_ptr)(gp) /* update _impure_ptr */

#endif /* OS_THREAD_SAFE_NEWLIB */

#if defined(ALT_STACK_CHECK) && (OS_TASK_CREATE_EXT_EN > 0)

      stw et, %gprel(alt_stack_limit_value)(gp)

#endif

      addi sp, sp, 44

      /*
       * resume execution of the new task.
       */

      ret

#ifdef ALT_STACK_CHECK

.Lstack_overflow:
        break 3

#endif

.set OSCtxSw_SWITCH_PC,0b-OSCtxSw

/*********************************************************************************************************
 *                                        START THE HIGHEST PRIORITY TASK
 *                                           void OSStartHighRdy(void)
 *
 * Note(s): 1) Upon entry,
 *             OSTCBCurPtr      points to the OS_TCB of the task to suspend
 *             OSTCBHighRdyPtr  points to the OS_TCB of the task to resume
 *
 *********************************************************************************************************/
      .global OSStartHighRdy

OSStartHighRdy:

      /*
       * disable interrupts so that the scheduler doesn't run while
       * we're initialising this task.
       */
      rdctl r18, status
      subi  r17, zero, 2   /* r17 = 0xfffffffe */
      and   r18, r18, r17
      wrctl status, r18

      /*
       * Call the user definable OSTaskSWHook()
       */

      call OSTaskSwHook

      /*
       * set OSRunning = TRUE.
       */

      movi r18, 1                    /* set r18 to the value 'TRUE' */
      stb r18, %gprel(OSRunning)(gp) /* save this to OSRunning */

#if defined(ALT_STACK_CHECK) && (OS_TASK_CREATE_EXT_EN == 0)

      mov et, zero                   /* Don't check stack limits */
      stw et, %gprel(alt_stack_limit_value)(gp)

#endif

      /*
       * start execution of the new task.
       */

      br 9b

/*********************************************************************************************************
 *                                        CALL THE TASK INITILISATION FUNCTION
 *                                           void OSStartTsk(void)
 *********************************************************************************************************/

      .global OSStartTsk

OSStartTsk:
      /* This instruction is never executed.  Its here to make the
       * backtrace work right
       */
      movi sp, 0

      /* Enable interrupts */
      rdctl r2, status
      ori   r2, r2, 0x1
      wrctl status, r2

      ldw r2, 4(sp)
      ldw r4, 0(sp)

      addi sp, sp, 8

      callr r2

      nop
