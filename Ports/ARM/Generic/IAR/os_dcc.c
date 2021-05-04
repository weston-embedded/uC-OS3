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
*                                           Generic ARM Port
*                                           DCC Communication
*
* File      : os_dcc.c
* Version   : V3.08.01
*********************************************************************************************************
* For       : ARM7 or ARM9
* Mode      : ARM  or Thumb
* Toolchain : IAR EWARM V5.xx and higher
*********************************************************************************************************
*/

#include <os.h>
                                                     /* This directive suppresses warnings for non-... */
#pragma  diag_suppress=Pe940                         /* ...void functions with no return values.       */

#ifdef __cplusplus
extern  "C" {
#endif

#if OS_CPU_ARM_DCC_EN > 0

/*
*********************************************************************************************************
*                                           CONSTANTS
*********************************************************************************************************
*/

#define  OS_DCC_OP_READ_U32     0x01000000
#define  OS_DCC_OP_READ_U16     0x02000000
#define  OS_DCC_OP_READ_U8      0x04000000
#define  OS_DCC_OP_GET_CAPS     0x08000000
#define  OS_DCC_OP_WRITE_U32    0x10000000
#define  OS_DCC_OP_WRITE_U16    0x20000000
#define  OS_DCC_OP_WRITE_U8     0x40000000
#define  OS_DCC_OP_ODD_ADDR     0x80000000
#define  OS_DCC_OP_COMMAND      0x00000001

#define  OS_DCC_COMM_CTRL_RD    0x00000001
#define  OS_DCC_COMM_CTRL_WR    0x00000002

#define  OS_DCC_SIGNATURE       0x91CA0000
#define  OS_DCC_CONFIG          0x00000077

/*
*********************************************************************************************************
*                                          LOCAL VARIABLES
*********************************************************************************************************
*/

static  CPU_INT32U  OSDCC_Cmd;
static  CPU_INT32U  OSDCC_Addr;
static  CPU_INT32U  OSDCC_ItemCnt;
static  CPU_INT32U  OSDCC_Data;

/*
*********************************************************************************************************
*                                       OSDCC_ReadCtrl()
*
* Description: This function retrieves data from the comms control register.
*
* Arguments  : none
*
* Returns    : The contents of the comms control register
*
* Notes      : 1) This function uses a coprocessor register transfer instruction to place the contents
*                 of the comms control register in R0.  Thus, the function does not contain an
*                 explicit return statement.  "#pragma diag_suppress=Pe940", which appears at the
*                 top of this file, is used to suppress the warning that normally results from non-
*                 void functions lacking return statements.
*********************************************************************************************************
*/

static  __arm  CPU_INT32U  OSDCC_ReadCtrl (void)
{
    __asm("mrc  P14,0,R0,C0,C0");
}

/*
*********************************************************************************************************
*                                         OSDCC_Read()
*
* Description: This function retrieves data from the comms data read register.
*
* Arguments  : none
*
* Returns    : The contents of the comms data read register
*
* Notes      : 1) This function uses a coprocessor register transfer instruction to place the contents
*                 of the comms data read register in R0.  Thus, the function does not contain an
*                 explicit return statement.  "#pragma diag_suppress=Pe940", which appears at the
*                 top of this file, is used to suppress the warning that normally results from non-
*                 void functions lacking return statements.
*********************************************************************************************************
*/

static  __arm  CPU_INT32U  OSDCC_Read (void)
{
    __asm("mrc  P14,0,R0,C1,C0");
}

/*
*********************************************************************************************************
*                                        OSDCC_Write()
*
* Description: This function places data in the comms data write register.
*
* Arguments  : none
*
* Returns    : none
*********************************************************************************************************
*/

static  __arm  void  OSDCC_Write (CPU_INT32U data)
{
    __asm("mcr  P14,0,R0,C1,C0");
}

/*
*********************************************************************************************************
*                                        OSDCC_Handler()
*
* Description: This function reads commands from the DCC comms data read register.  Data may be
*              transferred to or from memory based on those commands.
*
* Arguments  : none
*
* Returns    : none
*
* Notes      : 1) This function should be called periodically.  If OS_CPU_ARM_DCC_EN is '1', this
*                 function will be called from both the idle task hook and the tick interrupt hook.
*********************************************************************************************************
*/

void  OSDCC_Handler (void)
{
    CPU_INT32U  reg_val;
    CPU_ALLOC();


    CPU_CRITICAL_ENTER();                            /* Disable interrupts                             */

                                                     /* Check for the presence of new data             */
    if ((OSDCC_ReadCtrl() & OS_DCC_COMM_CTRL_RD) != 0) {
        reg_val = OSDCC_Read();                      /* Read the new data                              */

        if ((reg_val & OS_DCC_OP_COMMAND) != 0) {    /* Determine whether a command has been received  */
            OSDCC_Cmd = reg_val;
                                                     /* Check for an odd address in the next operation */
            if ((OSDCC_Cmd & OS_DCC_OP_ODD_ADDR) != 0) {
                OSDCC_Addr |= 1;
            }
                                                     /* If data will be read, adjust OSDCC_ItemCnt     */
            if ((OSDCC_Cmd & (OS_DCC_OP_READ_U32 | OS_DCC_OP_READ_U16 | OS_DCC_OP_READ_U8
                                                 |  OS_DCC_OP_GET_CAPS)) != 0) {
                OSDCC_ItemCnt = (OSDCC_Cmd >> 2) & 0xffff;
            } else {                                 /* Data will be written; initialize OSDCC_Data    */
                if ((OSDCC_Cmd & OS_DCC_OP_WRITE_U32) != 0) {
                    OSDCC_Data |= (OSDCC_Cmd << 14) & 0xffff0000;
                } else {
                    OSDCC_Data = (OSDCC_Cmd >> 2) & 0xffff;
                }
                                                     /* Write a single byte                            */
                if ((OSDCC_Cmd & OS_DCC_OP_WRITE_U8) != 0) {
                    *(INT8U *)OSDCC_Addr = OSDCC_Data;
                    OSDCC_Addr += 1;
                }
                                                     /* Write two bytes                                */
                if ((OSDCC_Cmd & OS_DCC_OP_WRITE_U16) != 0) {
                    *(INT16U *)OSDCC_Addr = OSDCC_Data;
                    OSDCC_Addr += 2;
                }
                                                     /* Write four bytes                               */
                if ((OSDCC_Cmd & OS_DCC_OP_WRITE_U32) != 0) {
                    *(INT32U *)OSDCC_Addr =OSDCC_Data;
                    OSDCC_Addr += 4;
                }
            }
            OS_EXIT_CRITICAL();
            return;
        }
        OSDCC_Addr     = reg_val;                    /* An address was received; OSDCC_Addr is updated */
    }
                                                     /* Determine whether data must be read            */
    if (OSDCC_ItemCnt != 0) {
                                                     /* Confirm that the comms data write register...  */
                                                     /* ...is free from the processor point of view    */
        if ((OSDCC_ReadCtrl() & OS_DCC_COMM_CTRL_WR) == 0) {
            reg_val = (OS_DCC_CONFIG | OS_DCC_SIGNATURE);
                                                     /* Read a single byte                             */
            if ((OSDCC_Cmd & OS_DCC_OP_READ_U8) != 0) {
                reg_val = *(INT8U *)OSDCC_Addr;
                OSDCC_Addr += 1;
            }
                                                     /* Read two bytes                                 */
            if ((OSDCC_Cmd & OS_DCC_OP_READ_U16) != 0) {
                reg_val = *(INT16U *)OSDCC_Addr;
                OSDCC_Addr += 2;
            }
                                                     /* Read four bytes                                */
            if ((OSDCC_Cmd & OS_DCC_OP_READ_U32) != 0) {
                reg_val = *(INT32U *)OSDCC_Addr;
                OSDCC_Addr += 4;
            }

            OSDCC_Write(reg_val);                    /* Place data in the comms data write register    */
            OSDCC_ItemCnt--;                         /* Decrement the number of items to be read       */
        }
    }
    CPU_CRITICAL_EXIT();
}

#endif

#ifdef __cplusplus
}
#endif
