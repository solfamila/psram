/*
 * Copyright (c) 2014-2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2023 NXP
 * Copyright 2021 ACRIOS Systems s.r.o.
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "erpc_message_buffer.hpp"

#include "erpc_config_internal.h"

#include <cstring>

using namespace erpc;
using namespace std;

////////////////////////////////////////////////////////////////////////////////
// Code
////////////////////////////////////////////////////////////////////////////////

MessageBuffer::MessageBuffer(const MessageBuffer &buffer)
{
    m_buf = buffer.m_buf;
    m_len = buffer.m_len;
    m_used = buffer.m_used;
}

void MessageBuffer::setUsed(uint16_t used)
{
    erpc_assert(used <= m_len);

    m_used = used;
}

erpc_status_t MessageBuffer::read(uint16_t offset, void *data, uint32_t length)
{
    erpc_status_t err = kErpcStatus_Success;

    if (length > 0U)
    {
        if (data == NULL)
        {
            err = kErpcStatus_MemoryError;
        }
        else if ((offset + length) > m_len || (offset + length) < offset)
        {
            err = kErpcStatus_BufferOverrun;
        }
        else
        {
            (void)memcpy(data, &m_buf[offset], length);
        }
    }

    return err;
}

erpc_status_t MessageBuffer::write(uint16_t offset, const void *data, uint32_t length)
{
    erpc_status_t err = kErpcStatus_Success;

    if (length > 0U)
    {
        if (data == NULL)
        {
            err = kErpcStatus_MemoryError;
        }
        else if ((offset + length) > m_len || (offset + length) < offset)
        {
            err = kErpcStatus_BufferOverrun;
        }
        else
        {
            (void)memcpy(&m_buf[offset], data, length);
        }
    }

    return err;
}

erpc_status_t MessageBuffer::copy(const MessageBuffer *other)
{
    erpc_status_t err;

    erpc_assert(other != NULL);
    erpc_assert(m_len >= other->m_len);

    m_used = other->m_used;
    err = this->write(0, other->m_buf, m_used);

    return err;
}

void MessageBuffer::swap(MessageBuffer *other)
{
    erpc_assert(other != NULL);

    MessageBuffer temp(*other);

    other->m_len = m_len;
    other->m_used = m_used;
    other->m_buf = m_buf;
    m_len = temp.m_len;
    m_used = temp.m_used;
    m_buf = temp.m_buf;
}

void Cursor::setBuffer(MessageBuffer &buffer, uint8_t reserved)
{
    // RPMSG when nested calls are enabled can set NULL buffer.
    // erpc_assert(buffer->get() && "Data buffer wasn't set to MessageBuffer.");
    // receive function should return err if it couldn't set data buffer.

    // erpc_assert(buffer != NULL);

    m_buffer = buffer;
    if (buffer != NULL)
    {
        m_pos = buffer.get() + reserved;
    }
}

MessageBuffer Cursor::getBuffer(void)
{
    return m_buffer;
}

MessageBuffer &Cursor::getBufferRef(void)
{
    return m_buffer;
}

uint8_t &Cursor::operator[](int index)
{
    erpc_assert(((m_pos + index) >= m_buffer.get()) &&
                ((uint16_t)(m_pos - m_buffer.get()) + index <= m_buffer.getLength()));

    return m_pos[index];
}

const uint8_t &Cursor::operator[](int index) const
{
    erpc_assert(((m_pos + index) >= m_buffer.get()) &&
                ((uint16_t)(m_pos - m_buffer.get()) + index <= m_buffer.getLength()));

    return m_pos[index];
}

Cursor &Cursor::operator+=(uint16_t n)
{
    erpc_assert((uint32_t)(m_pos - m_buffer.get()) + n <= m_buffer.getLength());

    m_pos += n;

    return *this;
}

Cursor &Cursor::operator-=(uint16_t n)
{
    erpc_assert(((uintptr_t)m_pos >= n) && (m_pos - n) >= m_buffer.get());

    m_pos -= n;

    return *this;
}

Cursor &Cursor::operator++(void)
{
    erpc_assert((uint16_t)(m_pos - m_buffer.get()) < m_buffer.getLength());

    ++m_pos;

    return *this;
}

Cursor &Cursor::operator--(void)
{
    erpc_assert(m_pos > m_buffer.get());

    --m_pos;

    return *this;
}

erpc_status_t Cursor::read(void *data, uint32_t length)
{
    erpc_assert((m_pos != NULL) && ("Data buffer wasn't set to MessageBuffer." != NULL));

    erpc_status_t err = kErpcStatus_Success;

    if (length > 0U)
    {
        if (data == NULL)
        {
            err = kErpcStatus_MemoryError;
        }
        else if (length > getRemainingUsed())
        {
            err = kErpcStatus_Fail;
        }
        else if (length > getRemaining())
        {
            err = kErpcStatus_BufferOverrun;
        }
        else
        {
            (void)memcpy(data, m_pos, length);
            m_pos += length;
        }
    }

    return err;
}

erpc_status_t Cursor::write(const void *data, uint32_t length)
{
    erpc_assert((m_pos != NULL) && ("Data buffer wasn't set to MessageBuffer." != NULL));
    erpc_assert(m_pos == (m_buffer.get() + m_buffer.getUsed()));

    erpc_status_t err = kErpcStatus_Success;

    if (length > 0U)
    {
        if (data == NULL)
        {
            err = kErpcStatus_MemoryError;
        }
        else if (length > getRemaining())
        {
            err = kErpcStatus_BufferOverrun;
        }
        else
        {
            (void)memcpy(m_pos, data, length);
            m_pos += length;
            m_buffer.setUsed(m_buffer.getUsed() + length);
        }
    }

    return err;
}

MessageBufferFactory::MessageBufferFactory(void) {}

MessageBufferFactory::~MessageBufferFactory(void) {}

MessageBuffer MessageBufferFactory::create(uint8_t reserveHeaderSize)
{
    MessageBuffer messageBuffer = create();

    messageBuffer.setUsed(reserveHeaderSize);

    return messageBuffer;
}

bool MessageBufferFactory::createServerBuffer(void)
{
    return true;
}

erpc_status_t MessageBufferFactory::prepareServerBufferForSend(MessageBuffer &message, uint8_t reserveHeaderSize)
{
    message.setUsed(reserveHeaderSize);
    return kErpcStatus_Success;
}
