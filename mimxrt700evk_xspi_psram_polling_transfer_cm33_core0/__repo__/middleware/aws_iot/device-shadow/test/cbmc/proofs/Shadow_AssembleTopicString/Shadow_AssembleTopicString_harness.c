/*
 * AWS IoT Device Shadow v1.3.0
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
 * @file Shadow_AssembleTopicString_harness.c
 * @brief Implements the proof harness for Shadow_AssembleTopicString function.
 */

#include "shadow.h"

#include <stdlib.h>

void harness()
{
    char * topicBuffer;
    uint16_t bufferSize;
    uint16_t * pOutLength;
    char * pThingName;
    uint8_t thingNameLength;
    char * pShadowName;
    uint8_t shadowNameLength;
    uint8_t topicType;

    topicBuffer = malloc( bufferSize );
    pOutLength = malloc( sizeof( *pOutLength ) );
    pThingName = malloc( thingNameLength );
    pShadowName = malloc( shadowNameLength );

    Shadow_AssembleTopicString( topicType,
                                pThingName,
                                thingNameLength,
                                pShadowName,
                                shadowNameLength,
                                topicBuffer,
                                bufferSize,
                                pOutLength );
}
