/*
 * Copyright (c) 2014, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * Copyright 2021 ACRIOS Systems s.r.o.
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _EMBEDDED_RPC__SIMPLE_SERVER_H_
#define _EMBEDDED_RPC__SIMPLE_SERVER_H_

#include "erpc_server.hpp"

/*!
 * @addtogroup infra_server
 * @{
 * @file
 */

////////////////////////////////////////////////////////////////////////////////
// Classes
////////////////////////////////////////////////////////////////////////////////

namespace erpc {
/*!
 * @brief Based server implementation.
 *
 * @ingroup infra_server
 */
class SimpleServer : public Server
{
public:
    /*!
     * @brief Constructor.
     *
     * This function initializes object attributes.
     */
    SimpleServer(void);

    virtual ~SimpleServer(void);

    /*!
     * @brief Run server in infinite loop.
     *
     * Will never jump out from this function.
     */
    virtual erpc_status_t run(void) override;

    /*!
     * @brief Run server implementation only if exist message to process.
     *
     * If is message to process, server process it and jumps out from this function,
     * useful for bare-metal because doesn't block main loop, when are not messages
     * to process.
     *
     * @return Return true when server is ON, else false.
     */
    virtual erpc_status_t poll(void);

    /*!
     * @brief This function sets server from ON to OFF
     */
    virtual void stop(void) override;

protected:
    bool m_isServerOn; /*!< Information if server is ON or OFF. */

    /*!
     * @brief Run server implementation.
     *
     * This function call functions for receiving data, process this data and
     * if reply exist, send it back.
     */
    erpc_status_t runInternal(void);

    /*!
     * @brief This function handle receiving request message and reading base info about message.
     *
     * @param[in] codec Inout codec to use.
     * @param[in] buff Inout codec to use.
     * @param[out] msgType Type of received message. Based on message type will be (will be not) sent respond.
     * @param[out] serviceId To identify interface.
     * @param[out] methodId To identify function in interface.
     * @param[out] sequence To connect correct answer with correct request.
     *
     * @returns #kErpcStatus_Success or based on service handleInvocation.
     */
    erpc_status_t runInternalBegin(Codec **codec, MessageBuffer &buff, message_type_t &msgType, uint32_t &serviceId,
                                   uint32_t &methodId, uint32_t &sequence);

    /*!
     * @brief This function process message and handle sending respond.
     *
     * @param[in] codec Inout codec to use.
     * @param[in] msgType Type of received message. Based on message type will be (will be not) sent respond.
     * @param[in] serviceId To identify interface.
     * @param[in] methodId To identify function in interface.
     * @param[in] sequence To connect correct answer with correct request.
     *
     * @returns #kErpcStatus_Success or based on service handleInvocation.
     */
    erpc_status_t runInternalEnd(Codec *codec, message_type_t msgType, uint32_t serviceId, uint32_t methodId,
                                 uint32_t sequence);

#if ERPC_NESTED_CALLS
    /*!
     * @brief This function runs the server.
     *
     * @param[in] Request context to check that answer was for nested call.
     */
    virtual erpc_status_t run(RequestContext &request) override;
#endif

    /*!
     * @brief Disposing message buffers and codecs.
     *
     * @param[in] codec Pointer to codec to dispose. It contains also message buffer to dispose.
     */
    void disposeBufferAndCodec(Codec *codec);
};

} // namespace erpc

/*! @} */

#endif // _EMBEDDED_RPC__SIMPLE_SERVER_H_
