/*
 * Copyright 2016 - 2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __PD_POWER_INTERFACE_T__
#define __PD_POWER_INTERFACE_T__

#define PD_POWER_REQUEST_MIN_VOLTAGE (5000U / 50U) /* 5V */

#define VSAFE5V_IN_50MV (5000U / 50U)
#define VBUS_REQ_20V    (20000U / 50U)
#define VBUS_REQ_5V     (5000U / 50U)

typedef enum _pd_request_value_type
{
    kRequestPower_Current,
    kRequestPower_Power,
} pd_request_value_type_t;

typedef struct _pd_vbus_power
{
    uint32_t valueType : 2; /* pd_request_value_type_t */
    uint32_t minVoltage : 10;
    uint32_t maxVoltage : 10;
    uint32_t requestValue : 10; /* current or power according to valueType */
} pd_vbus_power_t;

void PD_PowerGetVbusVoltage(uint32_t *partnerSourceCaps, pd_rdo_t rdo, pd_vbus_power_t *vbusPower);
pd_status_t PD_DpmAppCommandCallback(void *callbackParam, uint32_t event, void *param);
pd_status_t PD_PowerSrcTurnOnDefaultVbus(void *callbackParam, uint8_t powerProgress);
pd_status_t PD_PowerSrcTurnOnRequestVbus(void *callbackParam, pd_rdo_t rdo);
pd_status_t PD_PowerSrcTurnOffVbus(void *callbackParam, uint8_t powerProgress);
pd_status_t PD_PowerSrcGotoMinReducePower(void *callbackParam);
pd_status_t PD_PowerSnkDrawTypeCVbus(void *callbackParam, uint8_t typecCurrentLevel, uint8_t powerProgress);
pd_status_t PD_PowerSnkDrawRequestVbus(void *callbackParam, pd_rdo_t rdo);
pd_status_t PD_PowerSnkStopDrawVbus(void *callbackParam, uint8_t powerProgress);
pd_status_t PD_PowerSnkGotoMinReducePower(void *callbackParam);
pd_status_t PD_PowerControlVconn(void *callbackParam, uint8_t on);

pd_status_t PD_PowerBoardReset(uint8_t port);
pd_status_t PD_PowerBoardSourceEnableVbusPower(uint8_t port, pd_vbus_power_t vbusPower);
pd_status_t PD_PowerBoardSinkEnableVbusPower(uint8_t port, pd_vbus_power_t vbusPower);
pd_status_t PD_PowerBoardControlVconn(uint8_t port, uint8_t on);
void PD_PowerBoardControlInit(uint8_t port, pd_handle pdHandle, hal_gpio_handle_t powerGpioHandle);
void PD_PowerBoardControlDeinit(uint8_t port);

#endif
