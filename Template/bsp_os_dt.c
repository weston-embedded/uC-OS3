/*
*********************************************************************************************************
*                                              uC/OS-III
*                                        The Real-Time Kernel
*
*                    Copyright 2009-2021 Silicon Laboratories Inc. www.silabs.com
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
*                                   uC/OS-III BOARD SUPPORT PACKAGE
*                                          Dynamic Tick BSP
*
* Filename : bsp_os_dt.c
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#include  <cpu_core.h>
#include  <os.h>


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#if (OS_CFG_DYN_TICK_EN == DEF_ENABLED)
#define  TIMER_COUNT_HZ             ($$$$)                      /* Frequency of the Dynamic Tick Timer.                 */
#define  TIMER_TO_OSTICK(count)     (((CPU_INT64U)(count)  * OS_CFG_TICK_RATE_HZ) /      TIMER_COUNT_HZ)
#define  OSTICK_TO_TIMER(ostick)    (((CPU_INT64U)(ostick) * TIMER_COUNT_HZ)      / OS_CFG_TICK_RATE_HZ)

                                                                /* The max timer count should end on a 1 tick boundary. */
#define  TIMER_COUNT_MAX            (DEF_INT_32U_MAX_VAL - (DEF_INT_32U_MAX_VAL % OSTICK_TO_TIMER(1u)))
#endif


/*
*********************************************************************************************************
*                                           LOCAL VARIABLES
*********************************************************************************************************
*/

#if (OS_CFG_DYN_TICK_EN == DEF_ENABLED)
static  OS_TICK  TickDelta = 0u;                                /* Stored in OS Tick units.                             */
#endif


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

#if (OS_CFG_DYN_TICK_EN == DEF_ENABLED)
static  void  BSP_DynTick_ISRHandler(void);
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           GLOBAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                          BSP_OS_TickInit()
*
* Description : Initializes the tick interrupt for the OS.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Note(s)     : (1) Must be called prior to OSStart() in main().
*
*               (2) This function ensures that the tick interrupt is disabled until BSP_OS_TickEn() is
*                   called in the startup task.
*********************************************************************************************************
*/

void  BSP_OS_TickInit (void)
{
    /* $$$$ */                                                  /* Stop the Dynamic Tick Timer if running.              */

    /* $$$$ */                                                  /* Configure the timer.                                 */
    /* $$$$ */                                                  /* Frequency           : TIMER_COUNT_HZ                 */
    /* $$$$ */                                                  /* Counter Match Value : TIMER_COUNT_MAX                */
    /* $$$$ */                                                  /* Counter Value       : 0                              */
    /* $$$$ */                                                  /* Interrupt           : On counter match               */

    /* $$$$ */                                                  /* Install BSP_DynTick_ISRHandler as the int. handler.  */
    /* $$$$ */                                                  /* Start the timer.                                     */
}


/*
*********************************************************************************************************
*                                         BSP_OS_TickEnable()
*
* Description : Enable the OS tick interrupt.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Note(s)     : none
*********************************************************************************************************
*/

void  BSP_OS_TickEnable (void)
{
    /* $$$$ */                                                  /* Enable Timer interrupt.                              */
    /* $$$$ */                                                  /* Start the Timer count generation.                    */
}


/*
*********************************************************************************************************
*                                        BSP_OS_TickDisable()
*
* Description : Disable the OS tick interrupt.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Note(s)     : none
*********************************************************************************************************
*/

void  BSP_OS_TickDisable (void)
{
    /* $$$$ */                                                  /* Stop the Timer count generation.                     */
    /* $$$$ */                                                  /* Disable Timer interrupt.                             */
}


/*
*********************************************************************************************************
*********************************************************************************************************
**                                      uC/OS-III DYNAMIC TICK
*********************************************************************************************************
*********************************************************************************************************
*/

#if (OS_CFG_DYN_TICK_EN == DEF_ENABLED)
/*
*********************************************************************************************************
*                                            OS_DynTickGet()
*
* Description : Get the number of ticks which have elapsed since the last delta was set.
*
* Argument(s) : none.
*
* Return(s)   : An unsigned integer between 0 and TickDelta, inclusive.
*
* Note(s)     : 1) This function is an INTERNAL uC/OS-III function & MUST NOT be called by the
*                  application.
*
*               2) This function is called with kernel-aware interrupts disabled.
*********************************************************************************************************
*/

OS_TICK  OS_DynTickGet (void)
{
    CPU_INT32U  tmrcnt;


    tmrcnt = /* $$$$ */;                                        /* Read current timer count.                            */

    if (/* $$$$ */) {                                           /* Check timer interrupt flag.                          */
        return (TickDelta);                                     /* Counter Overflow has occured.                        */
    }

    tmrcnt = TIMER_TO_OSTICK(tmrcnt);                           /* Otherwise, the value we read is valid.               */

    return ((OS_TICK)tmrcnt);
}


/*
*********************************************************************************************************
*                                            OS_DynTickSet()
*
* Description : Sets the number of ticks that the kernel wants to expire before the next interrupt.
*
* Argument(s) : ticks       number of ticks the kernel wants to delay.
*                           0 indicates an indefinite delay.
*
* Return(s)   : The actual number of ticks which will elapse before the next interrupt.
*               (See Note #3).
*
* Note(s)     : 1) This function is an INTERNAL uC/OS-III function & MUST NOT be called by the
*                  application.
*
*               2) This function is called with kernel-aware interrupts disabled.
*
*               3) It is possible for the kernel to specify a delay that is too large for our
*                  hardware timer, or an indefinite delay. In these cases, we should program the timer
*                  to count the maximum number of ticks possible. The value we return should then
*                  be the timer maximum converted into units of OS_TICK.
*********************************************************************************************************
*/

OS_TICK  OS_DynTickSet (OS_TICK  ticks)
{
    CPU_INT32U  tmrcnt;


    tmrcnt = OSTICK_TO_TIMER(ticks);

    if ((tmrcnt >= TIMER_COUNT_MAX) ||                          /* If the delay exceeds our timer's capacity,       ... */
        (tmrcnt ==              0u)) {                          /* ... or the kernel wants an indefinite delay.         */
        tmrcnt = TIMER_COUNT_MAX;                               /* Count as many ticks as we are able.                  */
    }

    TickDelta = TIMER_TO_OSTICK(tmrcnt);                        /* Convert the timer count into OS_TICK units.          */

    /* $$$$ */                                                  /* Stop the timer.                                      */
    /* $$$$ */                                                  /* Clear the timer interrupt.                           */
                                                                /* Set new timeout.                                     */
    /* $$$$ */                                                  /* Counter Match Value : tmrcnt                         */
    /* $$$$ */                                                  /* Counter Value       : 0                              */
    /* $$$$ */                                                  /* Start the timer.                                     */

    return (TickDelta);                                         /* Return the number ticks that will elapse before  ... */
                                                                /* ... the next interrupt.                              */
}


/*
*********************************************************************************************************
*                                      BSP_DynTick_ISRHandler()
*
* Description : Dynamic Tick ISR handler.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  BSP_DynTick_ISRHandler (void)
{
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
    OSIntEnter();
    CPU_CRITICAL_EXIT();

    OSTimeDynTick(TickDelta);                                   /* Next delta will be set by the kernel.                */

    OSIntExit();
}
#endif                                                          /* End of uC/OS-III Dynamic Tick module.                */


/*
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*/
