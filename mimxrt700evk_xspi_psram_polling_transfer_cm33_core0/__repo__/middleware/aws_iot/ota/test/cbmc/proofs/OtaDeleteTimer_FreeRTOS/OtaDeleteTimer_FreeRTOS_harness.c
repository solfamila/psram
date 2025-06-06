/*
 * AWS IoT Over-the-air Update v3.4.0
 * Copyright (C) 2021 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * @file OtaDeleteTimer_FreeRTOS_harness.c
 * @brief Implements the proof harness for OtaDeleteTimer_FreeRTOS function.
 */
/*  FreeRTOS includes for OTA library. */
#include "ota_os_freertos.h"

void OtaDeleteTimer_FreeRTOS_harness()
{
    OtaTimerId_t otaTimerId;
    OtaOsStatus_t status;

    /* otaTimerId can only have values of OtaTimerId_t enumeration. */
    __CPROVER_assume( otaTimerId == OtaRequestTimer || otaTimerId == OtaSelfTestTimer );

    status = OtaDeleteTimer_FreeRTOS( otaTimerId );

    __CPROVER_assert( ( status >= OtaOsSuccess ) && ( status <= OtaOsTimerDeleteFailed ), "Invalid value for OtaOsStats_t type." );
}
