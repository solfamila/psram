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
 * @file event_callback_stub.h
 * @brief Stub definition for the application defined MQTT library incoming
 * event callback.
 */
#ifndef EVENT_CALLBACK_STUB_H_
#define EVENT_CALLBACK_STUB_H_

/* mqtt.h must precede including this header. */

/**
 * @brief User defined callback for receiving incoming publishes and incoming
 * acks.
 *
 * @param[in] pContext Initialized MQTT context.
 * @param[in] pPacketInfo Information on the type of incoming MQTT packet.
 * @param[in] pDeserializedInfo Deserialized information from incoming packet.
 */
void EventCallbackStub( MQTTContext_t * pContext,
                        MQTTPacketInfo_t * pPacketInfo,
                        MQTTDeserializedInfo_t * pDeserializedInfo );

#endif /* ifndef EVENT_CALLBACK_STUB_H_ */
