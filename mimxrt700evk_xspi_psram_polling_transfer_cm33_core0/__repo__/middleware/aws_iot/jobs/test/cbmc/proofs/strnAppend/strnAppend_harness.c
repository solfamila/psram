/*
 * AWS IoT Jobs v1.3.0
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
 * @file strnAppend_harness.c
 * @brief Implements the proof harness for strnAppend function.
 */

#include <stdlib.h>
#include "jobs_annex.h"

void harness()
{
    char * dest, * src;
    size_t start, max, length;
    JobsStatus_t ret;

    /* max is the destination buffer length which must not exceed unwindings. */
    __CPROVER_assume( max < CBMC_MAX_BUFSIZE );

    /* destination buffer must not be NULL. */
    dest = malloc( max );
    __CPROVER_assume( dest != NULL );

    /* length is the source buffer length which must not exceed unwindings. */
    __CPROVER_assume( length < CBMC_MAX_BUFSIZE );

    /* source buffer must not be NULL. */
    src = malloc( length );
    __CPROVER_assume( src != NULL );

    ret = strnAppend( dest,
                      &start,
                      max,
                      src,
                      length );

    __CPROVER_assert( strnAppendEnum( ret ), "The return value is a subset of JobsStatus_t." );

    if( ret == JobsSuccess )
    {
        __CPROVER_assert( start < max,
                          "The buffer start index is less than the buffer length." );
    }
}
