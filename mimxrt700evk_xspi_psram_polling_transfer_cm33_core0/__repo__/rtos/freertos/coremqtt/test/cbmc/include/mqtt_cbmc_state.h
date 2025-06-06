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
 * @file mqtt_cbmc_state.h
 * @brief Allocation and assumption utilities for the MQTT library CBMC proofs.
 */
#ifndef MQTT_CBMC_STATE_H_
#define MQTT_CBMC_STATE_H_

#include <stdbool.h>

/* mqtt.h must precede including this header. */

#define IMPLIES( a, b )    ( !( a ) || ( b ) )

/**
 * @brief Allocate a #MQTTPacketInfo_t object.
 *
 * @param[in] pPacketInfo #MQTTPacketInfo_t object information.
 *
 * @return NULL or allocated #MQTTPacketInfo_t memory.
 */
MQTTPacketInfo_t * allocateMqttPacketInfo( MQTTPacketInfo_t * pPacketInfo );

/**
 * @brief Validate a #MQTTPacketInfo_t object.
 *
 * @param[in] pPacketInfo #MQTTPacketInfo_t object to validate.
 *
 * @return True if the #MQTTPacketInfo_t object is valid, false otherwise.
 *
 * @note A NULL object is a valid object. This is for coverage of the NULL
 * parameter checks in the function under proof.
 */
bool isValidMqttPacketInfo( const MQTTPacketInfo_t * pPacketInfo );

/**
 * @brief Allocate a #MQTTPublishInfo_t object.
 *
 * @param[in] pPublishInfo #MQTTPublishInfo_t object information.
 *
 * @return NULL or allocated #MQTTPublishInfo_t memory.
 */
MQTTPublishInfo_t * allocateMqttPublishInfo( MQTTPublishInfo_t * pPublishInfo );

/**
 * @brief Validate a #MQTTPublishInfo_t object.
 *
 * @param[in] pPublishInfo #MQTTPublishInfo_t object to validate.
 *
 * @return True if the #MQTTPublishInfo_t object is valid, false otherwise.
 *
 * @note A NULL object is a valid object. This is for coverage of the NULL
 * parameter checks in the function under proof.
 */
bool isValidMqttPublishInfo( const MQTTPublishInfo_t * pPublishInfo );

/**
 * @brief Allocate a #MQTTConnectInfo_t object.
 *
 * @param[in] pConnectInfo #MQTTConnectInfo_t object information.
 *
 * @return NULL or allocated #MQTTConnectInfo_t memory.
 */
MQTTConnectInfo_t * allocateMqttConnectInfo( MQTTConnectInfo_t * pConnectInfo );

/**
 * @brief Validate a #MQTTConnectInfo_t object.
 *
 * @param[in] pConnectInfo #MQTTConnectInfo_t object to validate.
 *
 * @return True if the #MQTTConnectInfo_t object is valid, false otherwise.
 *
 * @note A NULL object is a valid object. This is for coverage of the NULL
 * parameter checks in the function under proof.
 */
bool isValidMqttConnectInfo( const MQTTConnectInfo_t * pConnectInfo );

/**
 * @brief Allocate a #MQTTFixedBuffer_t object.
 *
 * @param[in] pBuffer #MQTTFixedBuffer_t object information.
 *
 * @return NULL or allocated #MQTTFixedBuffer_t memory.
 */
MQTTFixedBuffer_t * allocateMqttFixedBuffer( MQTTFixedBuffer_t * pFixedBuffer );

/**
 * @brief Validate a #MQTTFixedBuffer_t object.
 *
 * @param[in] pBuffer #MQTTFixedBuffer_t object to validate.
 *
 * @return True if the #MQTTFixedBuffer_t object is valid, false otherwise.
 *
 * @note A NULL object is a valid object. This is for coverage of the NULL
 * parameter checks in the function under proof.
 */
bool isValidMqttFixedBuffer( const MQTTFixedBuffer_t * pFixedBuffer );

/**
 * @brief Allocate an array of #MQTTSubscribeInfo_t objects.
 *
 * @param[in] pSubscriptionList #MQTTSubscribeInfo_t object information.
 * @param[in] subscriptionCount The amount of #MQTTSubscribeInfo_t objects to allocate.
 *
 * @return NULL or allocated #MQTTSubscribeInfo_t array.
 */
MQTTSubscribeInfo_t * allocateMqttSubscriptionList( MQTTSubscribeInfo_t * pSubscriptionList,
                                                    size_t subscriptionCount );

/**
 * @brief Validate an array of #MQTTSubscribeInfo_t objects.
 *
 * @param[in] pSubscriptionList #MQTTSubscribeInfo_t object information.
 * @param[in] subscriptionCount The length of #MQTTSubscribeInfo_t objects in the pSubscriptionList.
 *
 * @return True if the #MQTTSubscribeInfo_t is valid.
 *
 * @note A NULL object is a valid object. This is for coverage of the NULL
 * parameter checks in the function under proof.
 */
bool isValidMqttSubscriptionList( MQTTSubscribeInfo_t * pSubscriptionList,
                                  size_t subscriptionCount );

/**
 * @brief Allocate a #MQTTContext_t object.
 *
 * @param[in] pBuffer #MQTTContext_t object information.
 *
 * @return NULL or allocated #MQTTContext_t memory.
 */
MQTTContext_t * allocateMqttContext( MQTTContext_t * pContext );

/**
 * @brief Validate a #MQTTContext_t object.
 *
 * @param[in] pBuffer #MQTTContext_t object to validate.
 *
 * @return True if the #MQTTContext_t object is valid, false otherwise.
 *
 * @note A NULL object is a valid object. This is for coverage of the NULL
 * parameter checks in the function under proof.
 */
bool isValidMqttContext( const MQTTContext_t * pContext );

#endif /* ifndef MQTT_CBMC_STATE_H_ */
