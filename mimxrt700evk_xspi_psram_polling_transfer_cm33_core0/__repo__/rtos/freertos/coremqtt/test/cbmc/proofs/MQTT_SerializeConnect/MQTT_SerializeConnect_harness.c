/*
 * coreMQTT v2.1.1
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
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
 * @file MQTT_SerializeConnect_harness.c
 * @brief Implements the proof harness for MQTT_SerializeConnect function.
 */
#include "core_mqtt.h"
#include "mqtt_cbmc_state.h"

void harness()
{
    MQTTConnectInfo_t * pConnectInfo;
    MQTTPublishInfo_t * pWillInfo;
    size_t remainingLength;
    MQTTFixedBuffer_t * pFixedBuffer;
    size_t packetSize;
    MQTTStatus_t status = MQTTSuccess;

    pConnectInfo = allocateMqttConnectInfo( NULL );
    __CPROVER_assume( isValidMqttConnectInfo( pConnectInfo ) );

    pWillInfo = allocateMqttPublishInfo( NULL );
    __CPROVER_assume( isValidMqttPublishInfo( pWillInfo ) );

    pFixedBuffer = allocateMqttFixedBuffer( NULL );
    __CPROVER_assume( isValidMqttFixedBuffer( pFixedBuffer ) );

    /* Before calling MQTT_SerializeConnect() it is up to the application to make
     * sure that the information in MQTTConnectInfo_t and MQTTPublishInfo_t can
     * fit into the MQTTFixedBuffer_t. It is a violation of the API to call
     * MQTT_SerializeConnect() without first calling MQTT_GetConnectPacketSize(). */
    if( pConnectInfo != NULL )
    {
        /* The output parameter pPacketSize of the function MQTT_GetConnectPacketSize()
         * must not be NULL. packetSize returned is not used in this proof, but
         * is used normally by the application to verify the size of its
         * MQTTFixedBuffer_t. MQTT_SerializeConnect() will use the remainingLength
         * to recalculate the packetSize. */
        status = MQTT_GetConnectPacketSize( pConnectInfo,
                                            pWillInfo,
                                            &remainingLength,
                                            &packetSize );
    }

    if( status == MQTTSuccess )
    {
        /* For coverage, it is expected that a NULL pConnectInfo will reach this
         * function. */
        MQTT_SerializeConnect( pConnectInfo, pWillInfo, remainingLength, pFixedBuffer );
    }
}
