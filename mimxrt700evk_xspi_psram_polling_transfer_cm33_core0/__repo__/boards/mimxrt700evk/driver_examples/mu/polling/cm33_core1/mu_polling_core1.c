/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017, 2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_common.h"
#include "fsl_mu.h"
#include "board.h"
#include "app.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/* Flag indicates Core Boot Up*/
#define BOOT_FLAG 0x01U

/* Channel transmit and receive register */
#define CHN_MU_REG_NUM kMU_MsgReg0

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
static uint32_t g_msgRecv[CONFIG_MSG_LENGTH];
/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief Function to clear the g_msgRecv array.
 * This function set g_msgRecv to be 0.
 */
static void ClearMsgRecv(void)
{
    uint32_t i;
    for (i = 0U; i < CONFIG_MSG_LENGTH; i++)
    {
        g_msgRecv[i] = 0U;
    }
}

/*!
 * @brief Function to create delay for Led blink.
 */
void delay(void)
{
    volatile uint32_t i = 0;
    for (i = 0; i < 5000000; ++i)
    {
        __NOP();
    }
}

/*!
 * @brief Main function
 */
int main(void)
{
    uint32_t i;

    /* Init board hardware.*/
    BOARD_InitHardware();
    /* Initialize LED */
    LED_INIT();

    /* MUB init */
    MU_Init(APP_MU);
    /* Send flag to Core 0 to indicate Core 1 has startup */
    MU_SetFlags(APP_MU, BOOT_FLAG);

    /* Clear the g_msgRecv array before receive */
    ClearMsgRecv();
    /* Core 1 receive message from Core 0 */
    for (i = 0U; i < CONFIG_MSG_LENGTH; i++)
    {
        g_msgRecv[i] = MU_ReceiveMsg(APP_MU, CHN_MU_REG_NUM);
    }
    /* Core 1 send message back to Core 0 */
    for (i = 0U; i < CONFIG_MSG_LENGTH; i++)
    {
        MU_SendMsg(APP_MU, CHN_MU_REG_NUM, g_msgRecv[i]);
    }

    while (1)
    {
        delay();
        LED_TOGGLE();
    }
}
