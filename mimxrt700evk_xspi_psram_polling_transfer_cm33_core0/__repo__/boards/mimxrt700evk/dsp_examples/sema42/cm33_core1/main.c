/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_common.h"
#include "fsl_debug_console.h"
#include "fsl_sema42.h"
#include "fsl_mu.h"
#include "app.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/* Flag indicates Core Boot Up*/
#define BOOT_FLAG 0x01U
/* Flag indicates CM33 core has locked the sema42 gate. */
#define SEMA42_LOCK_FLAG 0x02U
/* Flag indicates DSP core has locked the sema42 gate. */
#define SEMA42_DSP_LOCK_FLAG 0x03U
/* The SEMA42 gate */
#define SEMA42_GATE 0U

/*
 * Use static domain ID or dynamic domain ID.
 */
#ifndef USE_STATIC_DOMAIN_ID
#define USE_STATIC_DOMAIN_ID 1
#endif

/*
 * The board has LED to show the status.
 */
#ifndef APP_BOARD_HAS_LED
#define APP_BOARD_HAS_LED 1
#endif
/*******************************************************************************
 * Prototypes
 ******************************************************************************/
#if USE_STATIC_DOMAIN_ID
uint8_t APP_GetMCoreDomainID(void)
{
    return 0U;
}
#else
uint8_t APP_GetMCoreDomainID(void);
#endif

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/
/*!
 * @brief Main function
 */
int main(void)
{
    uint8_t domainId;
    /* Init board hardware.*/
    BOARD_InitHardware();

#if APP_BOARD_HAS_LED
    /* Initialize LED */
    LED_INIT();
#endif

    /* MUA init */
    MU_Init(APP_MU);

    /* Print the initial banner */
    PRINTF("\r\nSema42 example!\r\n");

    /* Copy DSP image to RAM and start DSP core. */
    BOARD_DSP_Init();

    /* SEMA42 init */
    SEMA42_Init(APP_SEMA42);
    /* Reset the sema42 gate */
    SEMA42_ResetAllGates(APP_SEMA42);

    APP_GetMCoreDomainID();

    MU_SetFlags(APP_MU, BOOT_FLAG);

    /* Wait DSP core is Boot Up */
    while (BOOT_FLAG != MU_GetFlags(APP_MU))
    {
    }

    domainId = APP_GetMCoreDomainID();

    /* Lock the sema42 gate. */
    SEMA42_Lock(APP_SEMA42, SEMA42_GATE, domainId);

    MU_SetFlags(APP_MU, SEMA42_LOCK_FLAG);

    /* Wait until user press any key */
#if APP_BOARD_HAS_LED
    PRINTF("Press any key to unlock semaphore and DSP core will turn off the LED\r\n");
#else
    PRINTF("Press any key to unlock semaphore and DSP core will lock it\r\n");
#endif
    GETCHAR();

    /* Unlock the sema42 gate. */
    SEMA42_Unlock(APP_SEMA42, SEMA42_GATE);

#if APP_BOARD_HAS_LED
    PRINTF("Now the LED should be turned off\r\n");
#else
    PRINTF("Wait for DSP core lock the semaphore\r\n");
#endif
    /* Wait for DSP lock the sema */
    while (SEMA42_DSP_LOCK_FLAG != MU_GetFlags(APP_MU))
    {
    }

    PRINTF("\r\nSema42 example succeed!\r\n");

    while (1)
    {
    }
}
