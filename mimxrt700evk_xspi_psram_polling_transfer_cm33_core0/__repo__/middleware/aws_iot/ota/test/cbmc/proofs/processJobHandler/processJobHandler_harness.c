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
 * @file processJobHandler_harness.c
 * @brief Implements the proof harness for processJobHandler function.
 */
/*  Ota Agent includes. */
#include "ota.h"
#include "stubs.h"

extern OtaAgentContext_t otaAgent;
extern OtaErr_t processJobHandler( const OtaEventData_t * pEventData );

void processJobHandler_harness()
{
    OtaErr_t err;
    OtaEventData_t eventData;
    OtaInterfaces_t otaInterface;

    /* Havoc otaAgent and eventData to non-deterministically set all the bytes
     * in the object. */
    __CPROVER_havoc_object( &otaAgent );

    otaInterface.pal.getPlatformImageState = getPlatformImageStateStub;
    otaInterface.os.event.send = sendEventStub;
    otaInterface.os.timer.stop = stopTimerStub;
    otaInterface.pal.reset = resetPalStub;

    /* OtaInterface and the interfaces included in it cannot be NULL and they are
     * during the initialization of OTA specifically in the OTA_Init function. */
    otaAgent.pOtaInterface = &otaInterface;

    /* Initialize OtaAppCallback to an empty callback function. */
    otaAgent.OtaAppCallback = otaAppCallbackStub;

    err = processJobHandler( &eventData );

    /* processJobHandler returns the values which follow OtaErr_t enum. If it does not, then
     * there is a problem. */
    __CPROVER_assert( ( err >= OtaErrNone ) && ( err <= OtaErrActivateFailed ),
                      "Invalid return value from processJobHandler: Expected a value from OtaErr_t enum." );
}
