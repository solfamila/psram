/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef H_OS_MALLOC_
#define H_OS_MALLOC_

#if defined(USE_RTOS)
#include "FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

#undef malloc
#define malloc pvPortMalloc // os_malloc

#undef free
#define free vPortFree // os_free

#undef realloc
#define realloc os_realloc

#ifdef __cplusplus
}
#endif

#endif /* define(USE_RTOS) */

#endif
