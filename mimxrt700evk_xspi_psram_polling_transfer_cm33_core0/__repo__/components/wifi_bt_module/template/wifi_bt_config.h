/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _WIFI_BT_CONFIG_H_
#define _WIFI_BT_CONFIG_H_

#ifndef WIFI_BT_USE_IMU_INTERFACE
#include "sdmmc_config.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/*******************************************************************************
 * API
 ******************************************************************************/
/*!
 * @brief BOARD SDIO configurations.
 * @param card card descriptor
 * @param cardInt card interrupt
 */
void BOARD_WIFI_BT_Config(void *card, sdio_int_t cardInt);

/*!
 * @brief enable wifi/bt module.
 * @param enable true enable module, false disable module.
 */
void BOARD_WIFI_BT_Enable(bool enable);

#if defined(__cplusplus)
}
#endif /* __cplusplus */
#endif /* WIFI_BT_USE_IMU_INTERFACE */

#endif /* _WIFI_BT_CONFIG_H_ */
