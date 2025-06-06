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
 * @file network_interface_stubs.c
 * @brief Stubs to mock sending and receiving over a network interface.
 */

#include "core_mqtt.h"
#include "network_interface_stubs.h"

/* An exclusive bound on the times that the NetworkInterfaceSendStub will be
 * invoked before returning a loop terminating value. This is usually defined
 * in the Makefile of the harnessed function. */
#ifndef MAX_NETWORK_SEND_TRIES
    #define MAX_NETWORK_SEND_TRIES    3
#endif

/* An exclusive bound on the times that the NetworkInterfaceReceiveStub will
 * return an unbound value. At this value and beyond, the
 * NetworkInterfaceReceiveStub will return zero on every call. */
#ifndef MAX_NETWORK_RECV_TRIES
    #define MAX_NETWORK_RECV_TRIES    4
#endif

int32_t NetworkInterfaceReceiveStub( NetworkContext_t * pNetworkContext,
                                     void * pBuffer,
                                     size_t bytesToRecv )
{
    __CPROVER_assert( pBuffer != NULL,
                      "NetworkInterfaceReceiveStub pBuffer is not NULL." );

    __CPROVER_assert( __CPROVER_w_ok( pBuffer, bytesToRecv ),
                      "NetworkInterfaceReceiveStub pBuffer is writable up to bytesToRecv." );

    __CPROVER_havoc_object( pBuffer );

    int32_t bytesOrError;
    static size_t tries = 0;

    /* It is a bug for the application defined transport send function to return
     * more than bytesToRecv. */
    __CPROVER_assume( bytesOrError <= ( int32_t ) bytesToRecv );

    if( tries < ( MAX_NETWORK_RECV_TRIES - 1 ) )
    {
        tries++;
    }
    else
    {
        bytesOrError = 0;
    }

    return bytesOrError;
}

int32_t NetworkInterfaceSendStub( NetworkContext_t * pNetworkContext,
                                  const void * pBuffer,
                                  size_t bytesToSend )
{
    __CPROVER_assert( pBuffer != NULL,
                      "NetworkInterfaceSendStub pBuffer is not NULL." );

    /* The number of tries to send the message before this invocation. */
    static size_t tries = 1;

    int32_t bytesOrError;

    /* It is a bug for the application defined transport send function to return
     * more than bytesToSend. */
    __CPROVER_assume( bytesOrError <= ( int32_t ) bytesToSend );

    /* If the maximum tries are reached, then return a timeout. In the MQTT library
     * this stub is wrapped in a loop that will does not end until the bytesOrError
     * returned is negative. This means we could loop possibly INT32_MAX
     * iterations. Looping for INT32_MAX times adds no value to the proof.
     * What matters is that the MQTT library can handle all the possible values
     * that could be returned. */
    if( tries < ( MAX_NETWORK_SEND_TRIES - 1 ) )
    {
        tries++;
    }
    else
    {
        tries = 1;
        bytesOrError = bytesToSend;
    }

    return bytesOrError;
}
