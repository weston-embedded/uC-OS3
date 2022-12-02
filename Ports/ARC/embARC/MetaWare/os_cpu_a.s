;********************************************************************************************************
;                                              uC/OS-III
;                                        The Real-Time Kernel
;
;                    Copyright 2009-2022 Silicon Laboratories Inc. www.silabs.com
;
;                                 SPDX-License-Identifier: APACHE-2.0
;
;               This software is subject to an open source license and is distributed by
;                Silicon Laboratories Inc. pursuant to the terms of the Apache License,
;                    Version 2.0 available at www.apache.org/licenses/LICENSE-2.0.
;
;********************************************************************************************************

;********************************************************************************************************
;
;                                       ASSEMBLY LANGUAGE PORT
;
;                                                 ARC
;                                              MetaWare
;
; Filename : os_cpu_a.asm
; Version  : V3.08.02
;********************************************************************************************************

#define __ASSEMBLY__
#include "arc/arc.h"
#include "arc/arc_asm_common.h"

    .text
    .align 4
;********************************************************************************************************
;                                           PUBLIC FUNCTIONS
;********************************************************************************************************

    .global   task_startup_routine
    .global   dispatch
    .global   OSStartHighRdy             ; Public functions

    .extern   OSTCBCurPtr                ; Declared as OS_TCB *               , 32-bit long
    .extern   OSTCBHighRdyPtr            ; Declared as OS_TCB *               , 32-bit long
    .extern   OSPrioCur                  ; Declared as INT8U                  ,  8-bit long
    .extern   OSPrioHighRdy              ; Declared as INT8U                  ,  8-bit long
    .extern   OSTaskSwHook
    .extern   OS_Set_StackCheck

;********************************************************************************************************
;                                      CODE GENERATION DIRECTIVES
;********************************************************************************************************

    .text
    .align 4
/*
 *  task startup routine
 *
 */
task_startup_routine:
	seti	/* unlock cpu */
	mov	blink, OS_TaskReturn	/* set return address, in normal cases a task should not return */
	POP	r1			/* get task function body */
	POP	r0			/* get task parameters */
	j	[r1]

;*********************************************************************************************************
;                                       TASK LEVEL CONTEXT SWITCH
;
; Description : This function is called when a task makes a higher priority task ready-to-run.
;               The pseudo code is:
;
;                   OS_CTX_SAVE
;                   OSTCBCurPtr->SP = SP;
;                   OSTaskSwHook();
;                   OSPrioCur       = OSPrioHighRdy;
;                   OSTCBCurPtr     = OSTCBHighRdyPtr;
;                   SP              = OSTCBCurPtr->StkPtr;
;                   OS_CTX_RESTORE
;                   Return from interrupt/exception
;*********************************************************************************************************

    .text
    .align 4
dispatch:
/*
 *  the pre-conditions of this routine are task context, CPU is
 *  locked, dispatch is enabled.
 */
    SAVE_NONSCRATCH_REGS        /* save callee save registers */
    mov r1, dispatch_r
    PUSH    r1          /* save return address */
    ld  r0, [OSTCBCurPtr]
    bl  dispatcher

/* return routine when task dispatch happened in task context */
dispatch_r:
    RESTORE_NONSCRATCH_REGS     /* recover registers */
    j   [blink]

;/*$PAGE*/
;*********************************************************************************************************
;                                         START MULTITASKING
;
; Description : This function is called by OSStart() to start the highest priority task that was created
;               by your application before calling OSStart().
;
; Arguments   : none
;
; Note(s)     : 1) The stack frame is assumed to look as follows:
;
;                  OSTCBHighRdy->OSTCBStkPtr +  0  ---->
;
;               2) OSStartHighRdy() MUST:
;                      a) Call OSTaskSwHook() then,
;                      b) Switch to the highest priority task.
;*********************************************************************************************************

OSStartHighRdy:
    clri
    mov r0, 0
    st  r0, [exc_nest_count]
    b   dispatcher_0
dispatcher:
    ld  r1, [ulCriticalNesting]
    PUSH    r1          /* save critical nesting */
    st  sp, [r0]        /* save stack pointer of current task, r0->OSTCBCurPtr */
dispatcher_0:
    jl  OSTaskSwHook              ; Call OSTaskSwHook()
    mov  r1, OSPrioHighRdy        ; OSPrioCur   = OSPrioHighRdy
    mov  r0, OSPrioCur
    ldb  r2, [r1]
    stb  r2, [r0]
    ld  r1, [OSTCBHighRdyPtr]     ; OSTCBCurPtr = OSTCBHighRdyPtr
    st  r1, [OSTCBCurPtr]
    ld  r1, [OSTCBCurPtr]         ; SP = OSTCBCurPtr->StkPtr
    ld  sp, [r1]
#if ARC_FEATURE_STACK_CHECK
#if ARC_FEATURE_SEC_PRESENT
    lr r0, [AUX_SEC_STAT]
    bclr r0, r0, AUX_SEC_STAT_BIT_SSC
    sflag r0
#else
    lr r0, [AUX_STATUS32]
    bclr r0, r0, AUX_STATUS_BIT_SC
    kflag r0
#endif
    jl  OS_Set_StackCheck
#if ARC_FEATURE_SEC_PRESENT
    lr r0, [AUX_SEC_STAT]
    bset r0, r0, AUX_SEC_STAT_BIT_SSC
    sflag r0
#else
    lr r0, [AUX_STATUS32]
    bset r0, r0, AUX_STATUS_BIT_SC
    kflag r0
#endif
#endif
    POP r0                        ;get critical nesting
    st  r0, [ulCriticalNesting]
    POP r0                        ;return address
    j   [r0]



/****** exceptions and interrupts handing ******/
/****** entry for exception handling ******/
    .global exc_entry_cpu
    .align 4
exc_entry_cpu:

    EXCEPTION_PROLOGUE

    mov blink,  sp
    mov r3, sp      /* as exception handler's para(p_excinfo) */

    ld  r0, [exc_nest_count]
    add r1, r0, 1
    st  r1, [exc_nest_count]
    brne    r0, 0, exc_handler_1
/* change to exception stack if interrupt happened in task context */
    mov sp, _e_stack
exc_handler_1:
    PUSH    blink

    lr  r0, [AUX_ECR]
    lsr r0, r0, 16
    mov r1, exc_int_handler_table
    ld.as   r2, [r1, r0]

    mov r0, r3
    jl  [r2]        /* !!!!jump to exception handler where interrupts are not allowed! */

/* interrupts are not allowed */
ret_exc:
    POP sp
    mov r1, exc_nest_count
    ld  r0, [r1]
    sub r0, r0, 1
    st  r0, [r1]
    brne    r0, 0, ret_exc_1 /* nest exception case */
    lr  r1, [AUX_IRQ_ACT] /* nest interrupt case */
    brne    r1, 0, ret_exc_1

    ld  r0, [context_switch_reqflg]
    brne    r0, 0, ret_exc_2
ret_exc_1:  /* return from non-task context, interrupts or exceptions are nested */

    EXCEPTION_EPILOGUE
    rtie

/* there is a dispatch request */
ret_exc_2:
    /* clear dispatch request */
    mov r0, 0
    st  r0, [context_switch_reqflg]

    ld  r0, [OSTCBCurPtr]
    breq    r0, 0, ret_exc_1

    SAVE_CALLEE_REGS    /* save callee save registers */

    lr  r0, [AUX_STATUS32]
    bclr    r0, r0, AUX_STATUS_BIT_AE   /* clear exception bit */
    kflag   r0

    mov r1, ret_exc_r   /* save return address */
    PUSH    r1

    bl  dispatcher  /* r0->OSTCBCurPtr */

ret_exc_r:
    /* recover exception status */
    lr  r0, [AUX_STATUS32]
    bset    r0, r0, AUX_STATUS_BIT_AE
    kflag   r0

    RESTORE_CALLEE_REGS /* recover registers */
    EXCEPTION_EPILOGUE
    rtie

/****** entry for normal interrupt exception handling ******/
    .global exc_entry_int   /* entry for interrupt handling */
    .align 4
exc_entry_int:
#if ARC_FEATURE_FIRQ == 1
#if ARC_FEATURE_RGF_NUM_BANKS > 1
    lr  r0, [AUX_IRQ_ACT]           /*  check whether it is P0 interrupt */
    btst    r0, 0
    jnz exc_entry_firq
#else
    PUSH    r10
    lr  r10, [AUX_IRQ_ACT]
    btst    r10, 0
    POP r10
    jnz exc_entry_firq
#endif
#endif
    INTERRUPT_PROLOGUE

    mov blink, sp

    clri    /* disable interrupt */
    ld  r3, [exc_nest_count]
    add r2, r3, 1
    st  r2, [exc_nest_count]
    seti    /* enable higher priority interrupt */

    brne    r3, 0, irq_handler_1
/* change to exception stack if interrupt happened in task context */
    mov sp, _e_stack
#if ARC_FEATURE_STACK_CHECK
#if ARC_FEATURE_SEC_PRESENT
    lr r0, [AUX_SEC_STAT]
    bclr r0, r0, AUX_SEC_STAT_BIT_SSC
    sflag r0
#else
    lr r0, [AUX_STATUS32]
    bclr r0, r0, AUX_STATUS_BIT_SC
    kflag r0
#endif
#endif
irq_handler_1:
    PUSH    blink

    lr  r0, [AUX_IRQ_CAUSE]
    mov r1, exc_int_handler_table
    ld.as   r2, [r1, r0]    /* r2 = exc_int_handler_table + irqno *4 */
/* handle software triggered interrupt */
    lr  r3, [AUX_IRQ_HINT]
    cmp r3, r0
    bne.d irq_hint_handled
    xor r3, r3, r3
    sr  r3, [AUX_IRQ_HINT]
irq_hint_handled:

    jl  [r2]        /* jump to interrupt handler */
/* no interrupts are allowed from here */
ret_int:
    clri    /* disable interrupt */

    POP sp
    mov r1, exc_nest_count
    ld  r0, [r1]
    sub r0, r0, 1
    st  r0, [r1]
/* if there are multi-bits set in IRQ_ACT, it's still in nest interrupt */
    lr  r0, [AUX_IRQ_CAUSE]
    sr  r0, [AUX_IRQ_SELECT]
    lr  r3, [AUX_IRQ_PRIORITY]
    lr  r1, [AUX_IRQ_ACT]
    bclr    r2, r1, r3
    brne    r2, 0, ret_int_1

    ld  r0, [context_switch_reqflg]
    brne    r0, 0, ret_int_2
ret_int_1:  /* return from non-task context */
    INTERRUPT_EPILOGUE
    rtie
/* there is a dispatch request */
ret_int_2:
    /* clear dispatch request */
    mov r0, 0
    st  r0, [context_switch_reqflg]

    ld  r0, [OSTCBCurPtr]
    breq    r0, 0, ret_int_1

/* r1 has old AUX_IRQ_ACT */
    PUSH    r1
/* clear related bits in IRQ_ACT manually to simulate a irq return  */
    sr  r2, [AUX_IRQ_ACT]

    SAVE_CALLEE_REGS    /* save callee save registers */
    mov r1, ret_int_r   /* save return address */
    PUSH    r1

    bl  dispatcher  /* r0->OSTCBCurPtr */

ret_int_r:
    RESTORE_CALLEE_REGS /* recover registers */
    POPAX   AUX_IRQ_ACT
    INTERRUPT_EPILOGUE
    rtie

#if ARC_FEATURE_FIRQ == 1
    .global exc_entry_firq
    .align 4
exc_entry_firq:
#if ARC_FEATURE_STACK_CHECK && ARC_FEATURE_RGF_NUM_BANKS > 1
#if ARC_FEATURE_SEC_PRESENT
    lr r0, [AUX_SEC_STAT]
    bclr r0, r0, AUX_SEC_STAT_BIT_SSC
    sflag r0
#else
    lr r0, [AUX_STATUS32]
    bclr r0, r0, AUX_STATUS_BIT_SC
    kflag r0
#endif
#endif
    SAVE_FIQ_EXC_REGS

    mov blink, sp

    ld  r3, [exc_nest_count]
    add r2, r3, 1
    st  r2, [exc_nest_count]

    brne    r3, 0, firq_handler_1
#if ARC_FEATURE_STACK_CHECK && ARC_FEATURE_RGF_NUM_BANKS == 1
#if ARC_FEATURE_SEC_PRESENT
    lr r0, [AUX_SEC_STAT]
    bclr r0, r0, AUX_SEC_STAT_BIT_SSC
    sflag r0
#else
    lr r0, [AUX_STATUS32]
    bclr r0, r0, AUX_STATUS_BIT_SC
    kflag r0
#endif
#endif
/* change to exception stack if interrupt happened in task context */
    mov sp, _e_stack
firq_handler_1:
    PUSH    blink

    lr  r0, [AUX_IRQ_CAUSE]
    mov r1, exc_int_handler_table
    ld.as   r2, [r1, r0]    /* r2 = exc_int_handler_table + irqno *4 */
/* handle software triggered interrupt */
    lr  r3, [AUX_IRQ_HINT]
    brne    r3, r0, firq_hint_handled
    xor r3, r3, r3
    sr  r3, [AUX_IRQ_HINT]
firq_hint_handled:

    jl  [r2]        /* jump to interrupt handler */
/* no interrupts are allowed from here */
ret_firq:
    clri
    POP sp

    mov r1, exc_nest_count
    ld  r0, [r1]
    sub r0, r0, 1
    st  r0, [r1]
/* if there are multi-bits set in IRQ_ACT, it's still in nest interrupt */
    lr  r1, [AUX_IRQ_ACT]
    bclr    r1, r1, 0
    brne    r1, 0, ret_firq_1

    ld  r0, [context_switch_reqflg]
    brne    r0, 0, ret_firq_2
ret_firq_1: /* return from non-task context */
    RESTORE_FIQ_EXC_REGS
    rtie
/* there is a dispatch request */
ret_firq_2:
    /* clear dispatch request */
    mov r0, 0
    st  r0, [context_switch_reqflg]

    ld  r0, [OSTCBCurPtr]
    breq    r0, 0, ret_firq_1

/* reconstruct the interruptted context
 * When ARC_FEATURE_RGF_BANKED_REGS >= 16 (16, 32), sp is banked
 * so need to restore the fast irq stack.
 */
#if ARC_FEATURE_RGF_BANKED_REGS >= 16
    RESTORE_LP_REGS
#if ARC_FEATURE_CODE_DENSITY
    RESTORE_CODE_DENSITY
#endif
    RESTORE_R58_R59
#endif

/* when BANKED_REGS == 16, r4-r9 wiil be also saved in fast irq stack
 * so pop them out
 */
#if  ARC_FEATURE_RGF_BANKED_REGS == 16 && !defined(ARC_FEATURE_RF16)
    POP     r9
    POP     r8
    POP     r7
    POP     r6
    POP     r5
    POP     r4
#endif

/* for other cases, unbanked regs are already in interrupted context's stack,
 * so just need to save and pop the banked regs
 */

/* save the interruptted context */
#if ARC_FEATURE_RGF_BANKED_REGS > 0
/* switch back to bank0  */
    lr r0, [AUX_STATUS32]
    bic     r0, r0, 0x70000
    kflag   r0
#endif

#if ARC_FEATURE_RGF_BANKED_REGS == 4
/* r4 - r12, gp, fp, r30, blink already saved */
    PUSH    r0
    PUSH    r1
    PUSH    r2
    PUSH    r3
#elif ARC_FEATURE_RGF_BANKED_REGS == 8
/* r4 - r9, r0, r11 gp, fp, r30, blink already saved */
    PUSH    r0
    PUSH    r1
    PUSH    r2
    PUSH    r3
    PUSH    r12
#elif ARC_FEATURE_RGF_BANKED_REGS >= 16
/* nothing is saved, */
    SAVE_R0_TO_R12

    SAVE_R58_R59
    PUSH    gp
    PUSH    fp
    PUSH    r30     /* general purpose */
    PUSH    blink

#if ARC_FEATURE_CODE_DENSITY
    SAVE_CODE_DENSITY
#endif
    SAVE_LP_REGS
#endif
    PUSH    ilink
    lr  r0, [AUX_STATUS32_P0]
    PUSH    r0
    lr  r0, [AUX_IRQ_ACT]
    PUSH    r0
    bclr    r0, r0, 0
    sr  r0, [AUX_IRQ_ACT]

    SAVE_CALLEE_REGS    /* save callee save registers */

    mov r1, ret_firq_r  /* save return address */
    PUSH    r1
    ld  r0, [OSTCBCurPtr]
    bl  dispatcher  /* r0->OSTCBCurPtr */

ret_firq_r:
    RESTORE_CALLEE_REGS /* recover registers */
    POPAX   AUX_IRQ_ACT
    POPAX   AUX_STATUS32_P0
    POP ilink

#if ARC_FEATURE_RGF_NUM_BANKS > 1
#if ARC_FEATURE_RGF_BANKED_REGS == 4
/* r4 - r12, gp, fp, r30, blink already saved */
    POP r3
    POP r2
    POP r1
    POP r0
    RESTORE_FIQ_EXC_REGS
#elif ARC_FEATURE_RGF_BANKED_REGS == 8
/* r4 - r9, gp, fp, r30, blink already saved */
    POP r12
    POP r3
    POP r2
    POP r1
    POP r0
    RESTORE_FIQ_EXC_REGS
#elif ARC_FEATURE_RGF_BANKED_REGS >= 16
    RESTORE_LP_REGS
#if ARC_FEATURE_CODE_DENSITY
    RESTORE_CODE_DENSITY
#endif
    POP blink
    POP r30
    POP fp
    POP gp

    RESTORE_R58_R59
    RESTORE_R0_TO_R12
#endif /* ARC_FEATURE_RGF_BANKED_REGS  */
#else
    RESTORE_FIQ_EXC_REGS
#endif /* ARC_FEATURE_RGF_NUM_BANKS */
    rtie
#endif
