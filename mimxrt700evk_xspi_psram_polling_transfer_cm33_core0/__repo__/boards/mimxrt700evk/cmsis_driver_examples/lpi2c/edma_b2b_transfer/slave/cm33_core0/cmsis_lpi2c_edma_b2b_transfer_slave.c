/*
 * Copyright 2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*  Standard C Included Files */
#include <stdio.h>
#include <string.h>
#include "board.h"
#include "fsl_debug_console.h"
#include "fsl_lpi2c.h"
#include "app.h"
#include "fsl_lpi2c_cmsis.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define I2C_MASTER_SLAVE_ADDR_7BIT (0x7EU)
#define I2C_DATA_LENGTH            (32) /* MAX is 256 */

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

uint8_t g_slave_buff[I2C_DATA_LENGTH];
volatile bool g_SlaveCompletionFlag = false;

/*******************************************************************************
 * Code
 ******************************************************************************/

static void lpi2c_slave_callback(uint32_t event)
{
    switch (event)
    {
        /* The master has sent a stop transition on the bus */
        case ARM_I2C_EVENT_TRANSFER_DONE:
            g_SlaveCompletionFlag = true;
            break;

        default:
            break;
    }
}

/*!
 * @brief Main function
 */
int main(void)
{
    /*Init BOARD*/
    BOARD_InitHardware();

    PRINTF("\r\nCMSIS LPI2C board2board EDMA example -- Slave transfer.\r\n\r\n");

    /* Initialize the LPI2C slave peripheral */
    EXAMPLE_I2C_SLAVE.Initialize(lpi2c_slave_callback);
    EXAMPLE_I2C_SLAVE.PowerControl(ARM_POWER_FULL);

    /* Change the slave address */
    EXAMPLE_I2C_SLAVE.Control(ARM_I2C_OWN_ADDRESS, I2C_MASTER_SLAVE_ADDR_7BIT);

    memset(g_slave_buff, 0, sizeof(g_slave_buff));

    /* Start accepting I2C transfers on the LPI2C slave peripheral */
    EXAMPLE_I2C_SLAVE.SlaveReceive(g_slave_buff, I2C_DATA_LENGTH);

    /*  Wait for transfer completed. */
    while (!g_SlaveCompletionFlag)
    {
    }

    /*  Reset slave completion flag to false. */
    g_SlaveCompletionFlag = false;

    /* Start transmitting I2C transfers on the LPI2C slave peripheral */
    EXAMPLE_I2C_SLAVE.SlaveTransmit(g_slave_buff, I2C_DATA_LENGTH);

    /* Wait for master receive completed.*/

    PRINTF("Slave received data :");
    for (uint32_t i = 0U; i < I2C_DATA_LENGTH; i++)
    {
        if (i % 8 == 0)
        {
            PRINTF("\r\n");
        }
        PRINTF("0x%2x  ", g_slave_buff[i]);
    }
    PRINTF("\r\n\r\n");

    while (!g_SlaveCompletionFlag)
    {
    }
    g_SlaveCompletionFlag = false;

    PRINTF("\r\nEnd of LPI2C example .\r\n");
    while (1)
    {
    }
}
