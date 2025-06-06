/*
 * Copyright (c) 2013-2023 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * -----------------------------------------------------------------------------
 *
 * Project:     CMSIS-RTOS RTX
 * Title:       Semaphore functions
 *
 * -----------------------------------------------------------------------------
 */

#include "rtx_lib.h"


//  OS Runtime Object Memory Usage
#ifdef RTX_OBJ_MEM_USAGE
osRtxObjectMemUsage_t osRtxSemaphoreMemUsage \
__attribute__((section(".data.os.semaphore.obj"))) =
{ 0U, 0U, 0U };
#endif


//  ==== Helper functions ====

/// Decrement Semaphore tokens.
/// \param[in]  semaphore       semaphore object.
/// \return 1 - success, 0 - failure.
static uint32_t SemaphoreTokenDecrement (os_semaphore_t *semaphore) {
#if (EXCLUSIVE_ACCESS == 0)
  uint32_t primask = __get_PRIMASK();
#endif
  uint32_t ret;

#if (EXCLUSIVE_ACCESS == 0)
  __disable_irq();

  if (semaphore->tokens != 0U) {
    semaphore->tokens--;
    ret = 1U;
  } else {
    ret = 0U;
  }

  if (primask == 0U) {
    __enable_irq();
  }
#else
  if (atomic_dec16_nz(&semaphore->tokens) != 0U) {
    ret = 1U;
  } else {
    ret = 0U;
  }
#endif

  return ret;
}

/// Increment Semaphore tokens.
/// \param[in]  semaphore       semaphore object.
/// \return 1 - success, 0 - failure.
static uint32_t SemaphoreTokenIncrement (os_semaphore_t *semaphore) {
#if (EXCLUSIVE_ACCESS == 0)
  uint32_t primask = __get_PRIMASK();
#endif
  uint32_t ret;

#if (EXCLUSIVE_ACCESS == 0)
  __disable_irq();

  if (semaphore->tokens < semaphore->max_tokens) {
    semaphore->tokens++;
    ret = 1U;
  } else {
    ret = 0U;
  }

  if (primask == 0U) {
    __enable_irq();
  }
#else
  if (atomic_inc16_lt(&semaphore->tokens, semaphore->max_tokens) < semaphore->max_tokens) {
    ret = 1U;
  } else {
    ret = 0U;
  }
#endif

  return ret;
}

/// Verify that Semaphore object pointer is valid.
/// \param[in]  semaphore       semaphore object.
/// \return true - valid, false - invalid.
static bool_t IsSemaphorePtrValid (const os_semaphore_t *semaphore) {
#ifdef RTX_OBJ_PTR_CHECK
  //lint --e{923} --e{9078} "cast from pointer to unsigned int" [MISRA Note 7]
  uint32_t cb_start  = (uint32_t)&__os_semaphore_cb_start__;
  uint32_t cb_length = (uint32_t)&__os_semaphore_cb_length__;

  // Check the section boundaries
  if (((uint32_t)semaphore - cb_start) >= cb_length) {
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return FALSE;
  }
  // Check the object alignment
  if ((((uint32_t)semaphore - cb_start) % sizeof(os_semaphore_t)) != 0U) {
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return FALSE;
  }
#else
  // Check NULL pointer
  if (semaphore == NULL) {
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return FALSE;
  }
#endif
  return TRUE;
}


//  ==== Library functions ====

/// Destroy a Semaphore object.
/// \param[in]  semaphore       semaphore object.
static void osRtxSemaphoreDestroy (os_semaphore_t *semaphore) {

  // Mark object as invalid
  semaphore->id = osRtxIdInvalid;

  // Free object memory
  if ((semaphore->flags & osRtxFlagSystemObject) != 0U) {
#ifdef RTX_OBJ_PTR_CHECK
    (void)osRtxMemoryPoolFree(osRtxInfo.mpi.semaphore, semaphore);
#else
    if (osRtxInfo.mpi.semaphore != NULL) {
      (void)osRtxMemoryPoolFree(osRtxInfo.mpi.semaphore, semaphore);
    } else {
      (void)osRtxMemoryFree(osRtxInfo.mem.common, semaphore);
    }
#endif
#ifdef RTX_OBJ_MEM_USAGE
    osRtxSemaphoreMemUsage.cnt_free++;
#endif
  }
  EvrRtxSemaphoreDestroyed(semaphore);
}

#ifdef RTX_SAFETY_CLASS
/// Delete a Semaphore safety class.
/// \param[in]  safety_class    safety class.
/// \param[in]  mode            safety mode.
void osRtxSemaphoreDeleteClass (uint32_t safety_class, uint32_t mode) {
  os_semaphore_t *semaphore;
  os_thread_t    *thread;
  uint32_t        length;

  //lint --e{923} --e{9078} "cast from pointer to unsigned int" [MISRA Note 7]
  semaphore = (os_semaphore_t *)(uint32_t)&__os_semaphore_cb_start__;
  length    =                   (uint32_t)&__os_semaphore_cb_length__;
  while (length >= sizeof(os_semaphore_t)) {
    if (   (semaphore->id == osRtxIdSemaphore) &&
        ((((mode & osSafetyWithSameClass)  != 0U) &&
          ((semaphore->attr >> osRtxAttrClass_Pos) == (uint8_t)safety_class)) ||
         (((mode & osSafetyWithLowerClass) != 0U) &&
          ((semaphore->attr >> osRtxAttrClass_Pos) <  (uint8_t)safety_class)))) {
      while (semaphore->thread_list != NULL) {
        thread = osRtxThreadListGet(osRtxObject(semaphore));
        osRtxThreadWaitExit(thread, (uint32_t)osErrorResource, FALSE);
      }
      osRtxSemaphoreDestroy(semaphore);
    }
    length -= sizeof(os_semaphore_t);
    semaphore++;
  }
}
#endif


//  ==== Post ISR processing ====

/// Semaphore post ISR processing.
/// \param[in]  semaphore       semaphore object.
static void osRtxSemaphorePostProcess (os_semaphore_t *semaphore) {
  os_thread_t *thread;

  // Check if Thread is waiting for a token
  if (semaphore->thread_list != NULL) {
    // Try to acquire token
    if (SemaphoreTokenDecrement(semaphore) != 0U) {
      // Wakeup waiting Thread with highest Priority
      thread = osRtxThreadListGet(osRtxObject(semaphore));
      osRtxThreadWaitExit(thread, (uint32_t)osOK, FALSE);
      EvrRtxSemaphoreAcquired(semaphore, semaphore->tokens);
    }
  }
}


//  ==== Service Calls ====

/// Create and Initialize a Semaphore object.
/// \note API identical to osSemaphoreNew
static osSemaphoreId_t svcRtxSemaphoreNew (uint32_t max_count, uint32_t initial_count, const osSemaphoreAttr_t *attr) {
  os_semaphore_t    *semaphore;
#ifdef RTX_SAFETY_CLASS
  const os_thread_t *thread = osRtxThreadGetRunning();
#endif
  uint32_t           attr_bits;
  uint8_t            flags;
  const char        *name;

  // Check parameters
  if ((max_count == 0U) || (max_count > osRtxSemaphoreTokenLimit) || (initial_count > max_count)) {
    EvrRtxSemaphoreError(NULL, (int32_t)osErrorParameter);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return NULL;
  }

  // Process attributes
  if (attr != NULL) {
    name      = attr->name;
    attr_bits = attr->attr_bits;
    //lint -e{9079} "conversion from pointer to void to pointer to other type" [MISRA Note 6]
    semaphore = attr->cb_mem;
    if ((attr_bits & osSafetyClass_Valid) != 0U) {
#ifdef RTX_SAFETY_CLASS
      if ((thread != NULL) &&
          ((thread->attr >> osRtxAttrClass_Pos) <
          (uint8_t)((attr_bits & osSafetyClass_Msk) >> osSafetyClass_Pos))) {
        EvrRtxSemaphoreError(NULL, (int32_t)osErrorSafetyClass);
        //lint -e{904} "Return statement before end of function" [MISRA Note 1]
        return NULL;
      }
#else
      EvrRtxSemaphoreError(NULL, (int32_t)osErrorSafetyClass);
      //lint -e{904} "Return statement before end of function" [MISRA Note 1]
      return NULL;
#endif
    }
    if (semaphore != NULL) {
      if (!IsSemaphorePtrValid(semaphore) || (attr->cb_size != sizeof(os_semaphore_t))) {
        EvrRtxSemaphoreError(NULL, osRtxErrorInvalidControlBlock);
        //lint -e{904} "Return statement before end of function" [MISRA Note 1]
        return NULL;
      }
    } else {
      if (attr->cb_size != 0U) {
        EvrRtxSemaphoreError(NULL, osRtxErrorInvalidControlBlock);
        //lint -e{904} "Return statement before end of function" [MISRA Note 1]
        return NULL;
      }
    }
  } else {
    name      = NULL;
#ifdef RTX_SAFETY_CLASS
    attr_bits = 0U;
#endif
    semaphore = NULL;
  }

  // Allocate object memory if not provided
  if (semaphore == NULL) {
    if (osRtxInfo.mpi.semaphore != NULL) {
      //lint -e{9079} "conversion from pointer to void to pointer to other type" [MISRA Note 5]
      semaphore = osRtxMemoryPoolAlloc(osRtxInfo.mpi.semaphore);
#ifndef RTX_OBJ_PTR_CHECK
    } else {
      //lint -e{9079} "conversion from pointer to void to pointer to other type" [MISRA Note 5]
      semaphore = osRtxMemoryAlloc(osRtxInfo.mem.common, sizeof(os_semaphore_t), 1U);
#endif
    }
#ifdef RTX_OBJ_MEM_USAGE
    if (semaphore != NULL) {
      uint32_t used;
      osRtxSemaphoreMemUsage.cnt_alloc++;
      used = osRtxSemaphoreMemUsage.cnt_alloc - osRtxSemaphoreMemUsage.cnt_free;
      if (osRtxSemaphoreMemUsage.max_used < used) {
        osRtxSemaphoreMemUsage.max_used = used;
      }
    }
#endif
    flags = osRtxFlagSystemObject;
  } else {
    flags = 0U;
  }

  if (semaphore != NULL) {
    // Initialize control block
    semaphore->id          = osRtxIdSemaphore;
    semaphore->flags       = flags;
    semaphore->attr        = 0U;
    semaphore->name        = name;
    semaphore->thread_list = NULL;
    semaphore->tokens      = (uint16_t)initial_count;
    semaphore->max_tokens  = (uint16_t)max_count;
#ifdef RTX_SAFETY_CLASS
    if ((attr_bits & osSafetyClass_Valid) != 0U) {
      semaphore->attr     |= (uint8_t)((attr_bits & osSafetyClass_Msk) >>
                                       (osSafetyClass_Pos - osRtxAttrClass_Pos));
    } else {
      // Inherit safety class from the running thread
      if (thread != NULL) {
        semaphore->attr   |= (uint8_t)(thread->attr & osRtxAttrClass_Msk);
      }
    }
#endif

    // Register post ISR processing function
    osRtxInfo.post_process.semaphore = osRtxSemaphorePostProcess;

    EvrRtxSemaphoreCreated(semaphore, semaphore->name);
  } else {
    EvrRtxSemaphoreError(NULL,(int32_t)osErrorNoMemory);
  }

  return semaphore;
}

/// Get name of a Semaphore object.
/// \note API identical to osSemaphoreGetName
static const char *svcRtxSemaphoreGetName (osSemaphoreId_t semaphore_id) {
  os_semaphore_t *semaphore = osRtxSemaphoreId(semaphore_id);

  // Check parameters
  if (!IsSemaphorePtrValid(semaphore) || (semaphore->id != osRtxIdSemaphore)) {
    EvrRtxSemaphoreGetName(semaphore, NULL);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return NULL;
  }

  EvrRtxSemaphoreGetName(semaphore, semaphore->name);

  return semaphore->name;
}

/// Acquire a Semaphore token or timeout if no tokens are available.
/// \note API identical to osSemaphoreAcquire
static osStatus_t svcRtxSemaphoreAcquire (osSemaphoreId_t semaphore_id, uint32_t timeout) {
  os_semaphore_t    *semaphore = osRtxSemaphoreId(semaphore_id);
#ifdef RTX_SAFETY_CLASS
  const os_thread_t *thread;
#endif
  osStatus_t         status;

  // Check parameters
  if (!IsSemaphorePtrValid(semaphore) || (semaphore->id != osRtxIdSemaphore)) {
    EvrRtxSemaphoreError(semaphore, (int32_t)osErrorParameter);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osErrorParameter;
  }

#ifdef RTX_SAFETY_CLASS
  // Check running thread safety class
  thread = osRtxThreadGetRunning();
  if ((thread != NULL) &&
      ((thread->attr >> osRtxAttrClass_Pos) < (semaphore->attr >> osRtxAttrClass_Pos))) {
    EvrRtxSemaphoreError(semaphore, (int32_t)osErrorSafetyClass);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osErrorSafetyClass;
  }
#endif

  // Try to acquire token
  if (SemaphoreTokenDecrement(semaphore) != 0U) {
    EvrRtxSemaphoreAcquired(semaphore, semaphore->tokens);
    status = osOK;
  } else {
    // No token available
    if (timeout != 0U) {
      EvrRtxSemaphoreAcquirePending(semaphore, timeout);
      // Suspend current Thread
      if (osRtxThreadWaitEnter(osRtxThreadWaitingSemaphore, timeout)) {
        osRtxThreadListPut(osRtxObject(semaphore), osRtxThreadGetRunning());
      } else {
        EvrRtxSemaphoreAcquireTimeout(semaphore);
      }
      status = osErrorTimeout;
    } else {
      EvrRtxSemaphoreNotAcquired(semaphore);
      status = osErrorResource;
    }
  }

  return status;
}

/// Release a Semaphore token that was acquired by osSemaphoreAcquire.
/// \note API identical to osSemaphoreRelease
static osStatus_t svcRtxSemaphoreRelease (osSemaphoreId_t semaphore_id) {
  os_semaphore_t *semaphore = osRtxSemaphoreId(semaphore_id);
  os_thread_t    *thread;
  osStatus_t      status;

  // Check parameters
  if (!IsSemaphorePtrValid(semaphore) || (semaphore->id != osRtxIdSemaphore)) {
    EvrRtxSemaphoreError(semaphore, (int32_t)osErrorParameter);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osErrorParameter;
  }

#ifdef RTX_SAFETY_CLASS
  // Check running thread safety class
  thread = osRtxThreadGetRunning();
  if ((thread != NULL) &&
      ((thread->attr >> osRtxAttrClass_Pos) < (semaphore->attr >> osRtxAttrClass_Pos))) {
    EvrRtxSemaphoreError(semaphore, (int32_t)osErrorSafetyClass);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osErrorSafetyClass;
  }
#endif

  // Check if Thread is waiting for a token
  if (semaphore->thread_list != NULL) {
    EvrRtxSemaphoreReleased(semaphore, semaphore->tokens);
    // Wakeup waiting Thread with highest Priority
    thread = osRtxThreadListGet(osRtxObject(semaphore));
    osRtxThreadWaitExit(thread, (uint32_t)osOK, TRUE);
    EvrRtxSemaphoreAcquired(semaphore, semaphore->tokens);
    status = osOK;
  } else {
    // Try to release token
    if (SemaphoreTokenIncrement(semaphore) != 0U) {
      EvrRtxSemaphoreReleased(semaphore, semaphore->tokens);
      status = osOK;
    } else {
      EvrRtxSemaphoreError(semaphore, osRtxErrorSemaphoreCountLimit);
      status = osErrorResource;
    }
  }

  return status;
}

/// Get current Semaphore token count.
/// \note API identical to osSemaphoreGetCount
static uint32_t svcRtxSemaphoreGetCount (osSemaphoreId_t semaphore_id) {
  os_semaphore_t *semaphore = osRtxSemaphoreId(semaphore_id);

  // Check parameters
  if (!IsSemaphorePtrValid(semaphore) || (semaphore->id != osRtxIdSemaphore)) {
    EvrRtxSemaphoreGetCount(semaphore, 0U);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return 0U;
  }

  EvrRtxSemaphoreGetCount(semaphore, semaphore->tokens);

  return semaphore->tokens;
}

/// Delete a Semaphore object.
/// \note API identical to osSemaphoreDelete
static osStatus_t svcRtxSemaphoreDelete (osSemaphoreId_t semaphore_id) {
  os_semaphore_t *semaphore = osRtxSemaphoreId(semaphore_id);
  os_thread_t    *thread;

  // Check parameters
  if (!IsSemaphorePtrValid(semaphore) || (semaphore->id != osRtxIdSemaphore)) {
    EvrRtxSemaphoreError(semaphore, (int32_t)osErrorParameter);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osErrorParameter;
  }

#ifdef RTX_SAFETY_CLASS
  // Check running thread safety class
  thread = osRtxThreadGetRunning();
  if ((thread != NULL) &&
      ((thread->attr >> osRtxAttrClass_Pos) < (semaphore->attr >> osRtxAttrClass_Pos))) {
    EvrRtxSemaphoreError(semaphore, (int32_t)osErrorSafetyClass);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osErrorSafetyClass;
  }
#endif

  // Unblock waiting threads
  if (semaphore->thread_list != NULL) {
    do {
      thread = osRtxThreadListGet(osRtxObject(semaphore));
      osRtxThreadWaitExit(thread, (uint32_t)osErrorResource, FALSE);
    } while (semaphore->thread_list != NULL);
    osRtxThreadDispatch(NULL);
  }

  osRtxSemaphoreDestroy(semaphore);

  return osOK;
}

//  Service Calls definitions
//lint ++flb "Library Begin" [MISRA Note 11]
SVC0_3(SemaphoreNew,      osSemaphoreId_t, uint32_t, uint32_t, const osSemaphoreAttr_t *)
SVC0_1(SemaphoreGetName,  const char *,    osSemaphoreId_t)
SVC0_2(SemaphoreAcquire,  osStatus_t,      osSemaphoreId_t, uint32_t)
SVC0_1(SemaphoreRelease,  osStatus_t,      osSemaphoreId_t)
SVC0_1(SemaphoreGetCount, uint32_t,        osSemaphoreId_t)
SVC0_1(SemaphoreDelete,   osStatus_t,      osSemaphoreId_t)
//lint --flb "Library End"


//  ==== ISR Calls ====

/// Acquire a Semaphore token or timeout if no tokens are available.
/// \note API identical to osSemaphoreAcquire
__STATIC_INLINE
osStatus_t isrRtxSemaphoreAcquire (osSemaphoreId_t semaphore_id, uint32_t timeout) {
  os_semaphore_t *semaphore = osRtxSemaphoreId(semaphore_id);
  osStatus_t      status;

  // Check parameters
  if (!IsSemaphorePtrValid(semaphore) || (semaphore->id != osRtxIdSemaphore) || (timeout != 0U)) {
    EvrRtxSemaphoreError(semaphore, (int32_t)osErrorParameter);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osErrorParameter;
  }

  // Try to acquire token
  if (SemaphoreTokenDecrement(semaphore) != 0U) {
    EvrRtxSemaphoreAcquired(semaphore, semaphore->tokens);
    status = osOK;
  } else {
    // No token available
    EvrRtxSemaphoreNotAcquired(semaphore);
    status = osErrorResource;
  }

  return status;
}

/// Release a Semaphore token that was acquired by osSemaphoreAcquire.
/// \note API identical to osSemaphoreRelease
__STATIC_INLINE
osStatus_t isrRtxSemaphoreRelease (osSemaphoreId_t semaphore_id) {
  os_semaphore_t *semaphore = osRtxSemaphoreId(semaphore_id);
  osStatus_t      status;

  // Check parameters
  if (!IsSemaphorePtrValid(semaphore) || (semaphore->id != osRtxIdSemaphore)) {
    EvrRtxSemaphoreError(semaphore, (int32_t)osErrorParameter);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osErrorParameter;
  }

  // Try to release token
  if (SemaphoreTokenIncrement(semaphore) != 0U) {
    // Register post ISR processing
    osRtxPostProcess(osRtxObject(semaphore));
    EvrRtxSemaphoreReleased(semaphore, semaphore->tokens);
    status = osOK;
  } else {
    EvrRtxSemaphoreError(semaphore, osRtxErrorSemaphoreCountLimit);
    status = osErrorResource;
  }

  return status;
}


//  ==== Public API ====

/// Create and Initialize a Semaphore object.
osSemaphoreId_t osSemaphoreNew (uint32_t max_count, uint32_t initial_count, const osSemaphoreAttr_t *attr) {
  osSemaphoreId_t semaphore_id;

  EvrRtxSemaphoreNew(max_count, initial_count, attr);
  if (IsException() || IsIrqMasked()) {
    EvrRtxSemaphoreError(NULL, (int32_t)osErrorISR);
    semaphore_id = NULL;
  } else {
    semaphore_id = __svcSemaphoreNew(max_count, initial_count, attr);
  }
  return semaphore_id;
}

/// Get name of a Semaphore object.
const char *osSemaphoreGetName (osSemaphoreId_t semaphore_id) {
  const char *name;

  if (IsException() || IsIrqMasked()) {
    name = svcRtxSemaphoreGetName(semaphore_id);
  } else {
    name =  __svcSemaphoreGetName(semaphore_id);
  }
  return name;
}

/// Acquire a Semaphore token or timeout if no tokens are available.
osStatus_t osSemaphoreAcquire (osSemaphoreId_t semaphore_id, uint32_t timeout) {
  osStatus_t status;

  EvrRtxSemaphoreAcquire(semaphore_id, timeout);
  if (IsException() || IsIrqMasked()) {
    status = isrRtxSemaphoreAcquire(semaphore_id, timeout);
  } else {
    status =  __svcSemaphoreAcquire(semaphore_id, timeout);
  }
  return status;
}

/// Release a Semaphore token that was acquired by osSemaphoreAcquire.
osStatus_t osSemaphoreRelease (osSemaphoreId_t semaphore_id) {
  osStatus_t status;

  EvrRtxSemaphoreRelease(semaphore_id);
  if (IsException() || IsIrqMasked()) {
    status = isrRtxSemaphoreRelease(semaphore_id);
  } else {
    status =  __svcSemaphoreRelease(semaphore_id);
  }
  return status;
}

/// Get current Semaphore token count.
uint32_t osSemaphoreGetCount (osSemaphoreId_t semaphore_id) {
  uint32_t count;

  if (IsException() || IsIrqMasked()) {
    count = svcRtxSemaphoreGetCount(semaphore_id);
  } else {
    count =  __svcSemaphoreGetCount(semaphore_id);
  }
  return count;
}

/// Delete a Semaphore object.
osStatus_t osSemaphoreDelete (osSemaphoreId_t semaphore_id) {
  osStatus_t status;

  EvrRtxSemaphoreDelete(semaphore_id);
  if (IsException() || IsIrqMasked()) {
    EvrRtxSemaphoreError(semaphore_id, (int32_t)osErrorISR);
    status = osErrorISR;
  } else {
    status = __svcSemaphoreDelete(semaphore_id);
  }
  return status;
}
