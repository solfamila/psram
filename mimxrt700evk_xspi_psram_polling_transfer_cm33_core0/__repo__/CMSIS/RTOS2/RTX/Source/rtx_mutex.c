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
 * Title:       Mutex functions
 *
 * -----------------------------------------------------------------------------
 */

#include "rtx_lib.h"


//  OS Runtime Object Memory Usage
#ifdef RTX_OBJ_MEM_USAGE
osRtxObjectMemUsage_t osRtxMutexMemUsage \
__attribute__((section(".data.os.mutex.obj"))) =
{ 0U, 0U, 0U };
#endif


//  ==== Helper functions ====

/// Verify that Mutex object pointer is valid.
/// \param[in]  mutex           mutex object.
/// \return true - valid, false - invalid.
static bool_t IsMutexPtrValid (const os_mutex_t *mutex) {
#ifdef RTX_OBJ_PTR_CHECK
  //lint --e{923} --e{9078} "cast from pointer to unsigned int" [MISRA Note 7]
  uint32_t cb_start  = (uint32_t)&__os_mutex_cb_start__;
  uint32_t cb_length = (uint32_t)&__os_mutex_cb_length__;

  // Check the section boundaries
  if (((uint32_t)mutex - cb_start) >= cb_length) {
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return FALSE;
  }
  // Check the object alignment
  if ((((uint32_t)mutex - cb_start) % sizeof(os_mutex_t)) != 0U) {
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return FALSE;
  }
#else
  // Check NULL pointer
  if (mutex == NULL) {
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return FALSE;
  }
#endif
  return TRUE;
}


//  ==== Library functions ====

/// Release Mutex list when owner Thread terminates.
/// \param[in]  mutex_list      mutex list.
void osRtxMutexOwnerRelease (os_mutex_t *mutex_list) {
  os_mutex_t  *mutex;
  os_mutex_t  *mutex_next;
  os_thread_t *thread;

  mutex = mutex_list;
  while (mutex != NULL) {
    mutex_next = mutex->owner_next;
    // Check if Mutex is Robust
    if ((mutex->attr & osMutexRobust) != 0U) {
      // Clear Lock counter
      mutex->lock = 0U;
      EvrRtxMutexReleased(mutex, 0U);
      // Check if Thread is waiting for a Mutex
      if (mutex->thread_list != NULL) {
        // Wakeup waiting Thread with highest Priority
        thread = osRtxThreadListGet(osRtxObject(mutex));
        osRtxThreadWaitExit(thread, (uint32_t)osOK, FALSE);
        // Thread is the new Mutex owner
        mutex->owner_thread = thread;
        mutex->owner_prev   = NULL;
        mutex->owner_next   = thread->mutex_list;
        if (thread->mutex_list != NULL) {
          thread->mutex_list->owner_prev = mutex;
        }
        thread->mutex_list = mutex;
        mutex->lock = 1U;
        EvrRtxMutexAcquired(mutex, 1U);
      }
    }
    mutex = mutex_next;
  }
}

/// Restore Mutex owner Thread priority.
/// \param[in]  mutex           mutex object.
/// \param[in]  thread_wakeup   thread wakeup object.
void osRtxMutexOwnerRestore (const os_mutex_t *mutex, const os_thread_t *thread_wakeup) {
  const os_mutex_t  *mutex0;
        os_thread_t *thread;
  const os_thread_t *thread0;
        int8_t       priority;

  // Restore owner Thread priority
  if ((mutex->attr & osMutexPrioInherit) != 0U) {
    thread   = mutex->owner_thread;
    priority = thread->priority_base;
    mutex0   = thread->mutex_list;
    // Check Mutexes owned by Thread
    do {
      if ((mutex0->attr & osMutexPrioInherit) != 0U) {
        // Check Threads waiting for Mutex
        thread0 = mutex0->thread_list;
        if (thread0 == thread_wakeup) {
          // Skip thread that is waken-up
          thread0 = thread0->thread_next;
        }
        if ((thread0 != NULL) && (thread0->priority > priority)) {
          // Higher priority Thread is waiting for Mutex
          priority = thread0->priority;
        }
      }
      mutex0 = mutex0->owner_next;
    } while (mutex0 != NULL);
    if (thread->priority != priority) {
      thread->priority = priority;
      osRtxThreadListSort(thread);
    }
  }
}

/// Unlock Mutex owner when mutex is deleted.
/// \param[in]  mutex           mutex object.
/// \return true - successful, false - not locked.
static bool_t osRtxMutexOwnerUnlock (os_mutex_t *mutex) {
  const os_mutex_t  *mutex0;
        os_thread_t *thread;
        int8_t       priority;

  // Check if Mutex is locked
  if (mutex->lock == 0U) {
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return FALSE;
  }

  thread = mutex->owner_thread;

  // Remove Mutex from Thread owner list
  if (mutex->owner_next != NULL) {
    mutex->owner_next->owner_prev = mutex->owner_prev;
  }
  if (mutex->owner_prev != NULL) {
    mutex->owner_prev->owner_next = mutex->owner_next;
  } else {
    thread->mutex_list = mutex->owner_next;
  }

  // Restore owner Thread priority
  priority = thread->priority_base;
  mutex0   = thread->mutex_list;
  // Check Mutexes owned by Thread
  while (mutex0 != NULL) {
    if ((mutex0->attr & osMutexPrioInherit) != 0U) {
      if ((mutex0->thread_list != NULL) && (mutex0->thread_list->priority > priority)) {
        // Higher priority Thread is waiting for Mutex
        priority = mutex0->thread_list->priority;
      }
    }
    mutex0 = mutex0->owner_next;
  }
  if (thread->priority != priority) {
    thread->priority = priority;
    osRtxThreadListSort(thread);
  }

  // Unblock waiting threads
  while (mutex->thread_list != NULL) {
    thread = osRtxThreadListGet(osRtxObject(mutex));
    osRtxThreadWaitExit(thread, (uint32_t)osErrorResource, FALSE);
  }

  mutex->lock = 0U;

  return TRUE;
}

/// Destroy a Mutex object.
/// \param[in]  mutex           mutex object.
static void osRtxMutexDestroy (os_mutex_t *mutex) {

  // Mark object as invalid
  mutex->id = osRtxIdInvalid;

  // Free object memory
  if ((mutex->flags & osRtxFlagSystemObject) != 0U) {
#ifdef RTX_OBJ_PTR_CHECK
    (void)osRtxMemoryPoolFree(osRtxInfo.mpi.mutex, mutex);
#else
    if (osRtxInfo.mpi.mutex != NULL) {
      (void)osRtxMemoryPoolFree(osRtxInfo.mpi.mutex, mutex);
    } else {
      (void)osRtxMemoryFree(osRtxInfo.mem.common, mutex);
    }
#endif
#ifdef RTX_OBJ_MEM_USAGE
    osRtxMutexMemUsage.cnt_free++;
#endif
  }
  EvrRtxMutexDestroyed(mutex);
}

#ifdef RTX_SAFETY_CLASS
/// Delete a Mutex safety class.
/// \param[in]  safety_class    safety class.
/// \param[in]  mode            safety mode.
void osRtxMutexDeleteClass (uint32_t safety_class, uint32_t mode) {
  os_mutex_t *mutex;
  uint32_t    length;

  //lint --e{923} --e{9078} "cast from pointer to unsigned int" [MISRA Note 7]
  mutex  = (os_mutex_t *)(uint32_t)&__os_mutex_cb_start__;
  length =               (uint32_t)&__os_mutex_cb_length__;
  while (length >= sizeof(os_mutex_t)) {
    if (   (mutex->id == osRtxIdMutex) &&
        ((((mode & osSafetyWithSameClass)  != 0U) &&
          ((mutex->attr >> osRtxAttrClass_Pos) == (uint8_t)safety_class)) ||
         (((mode & osSafetyWithLowerClass) != 0U) &&
          ((mutex->attr >> osRtxAttrClass_Pos) <  (uint8_t)safety_class)))) {
      (void)osRtxMutexOwnerUnlock(mutex);
      osRtxMutexDestroy(mutex);
    }
    length -= sizeof(os_mutex_t);
    mutex++;
  }
}
#endif


//  ==== Service Calls ====

/// Create and Initialize a Mutex object.
/// \note API identical to osMutexNew
static osMutexId_t svcRtxMutexNew (const osMutexAttr_t *attr) {
  os_mutex_t        *mutex;
#ifdef RTX_SAFETY_CLASS
  const os_thread_t *thread = osRtxThreadGetRunning();
#endif
  uint32_t           attr_bits;
  uint8_t            flags;
  const char        *name;

  // Process attributes
  if (attr != NULL) {
    name      = attr->name;
    attr_bits = attr->attr_bits;
    //lint -e{9079} "conversion from pointer to void to pointer to other type" [MISRA Note 6]
    mutex     = attr->cb_mem;
    if ((attr_bits & osSafetyClass_Valid) != 0U) {
#ifdef RTX_SAFETY_CLASS
      if ((thread != NULL) &&
          ((thread->attr >> osRtxAttrClass_Pos) <
          (uint8_t)((attr_bits & osSafetyClass_Msk) >> osSafetyClass_Pos))) {
        EvrRtxMutexError(NULL, (int32_t)osErrorSafetyClass);
        //lint -e{904} "Return statement before end of function" [MISRA Note 1]
        return NULL;
      }
#else
      EvrRtxMutexError(NULL, (int32_t)osErrorSafetyClass);
      //lint -e{904} "Return statement before end of function" [MISRA Note 1]
      return NULL;
#endif
    }
    if (mutex != NULL) {
      if (!IsMutexPtrValid(mutex) || (attr->cb_size != sizeof(os_mutex_t))) {
        EvrRtxMutexError(NULL, osRtxErrorInvalidControlBlock);
        //lint -e{904} "Return statement before end of function" [MISRA Note 1]
        return NULL;
      }
    } else {
      if (attr->cb_size != 0U) {
        EvrRtxMutexError(NULL, osRtxErrorInvalidControlBlock);
        //lint -e{904} "Return statement before end of function" [MISRA Note 1]
        return NULL;
      }
    }
  } else {
    name      = NULL;
    attr_bits = 0U;
    mutex     = NULL;
  }

  // Allocate object memory if not provided
  if (mutex == NULL) {
    if (osRtxInfo.mpi.mutex != NULL) {
      //lint -e{9079} "conversion from pointer to void to pointer to other type" [MISRA Note 5]
      mutex = osRtxMemoryPoolAlloc(osRtxInfo.mpi.mutex);
#ifndef RTX_OBJ_PTR_CHECK
    } else {
      //lint -e{9079} "conversion from pointer to void to pointer to other type" [MISRA Note 5]
      mutex = osRtxMemoryAlloc(osRtxInfo.mem.common, sizeof(os_mutex_t), 1U);
#endif
    }
#ifdef RTX_OBJ_MEM_USAGE
    if (mutex != NULL) {
      uint32_t used;
      osRtxMutexMemUsage.cnt_alloc++;
      used = osRtxMutexMemUsage.cnt_alloc - osRtxMutexMemUsage.cnt_free;
      if (osRtxMutexMemUsage.max_used < used) {
        osRtxMutexMemUsage.max_used = used;
      }
    }
#endif
    flags = osRtxFlagSystemObject;
  } else {
    flags = 0U;
  }

  if (mutex != NULL) {
    // Initialize control block
    mutex->id           = osRtxIdMutex;
    mutex->flags        = flags;
    mutex->attr         = (uint8_t)(attr_bits & ~osRtxAttrClass_Msk);
    mutex->name         = name;
    mutex->thread_list  = NULL;
    mutex->owner_thread = NULL;
    mutex->owner_prev   = NULL;
    mutex->owner_next   = NULL;
    mutex->lock         = 0U;
#ifdef RTX_SAFETY_CLASS
    if ((attr_bits & osSafetyClass_Valid) != 0U) {
      mutex->attr      |= (uint8_t)((attr_bits & osSafetyClass_Msk) >>
                                    (osSafetyClass_Pos - osRtxAttrClass_Pos));
    } else {
      // Inherit safety class from the running thread
      if (thread != NULL) {
        mutex->attr    |= (uint8_t)(thread->attr & osRtxAttrClass_Msk);
      }
    }
#endif
    EvrRtxMutexCreated(mutex, mutex->name);
  } else {
    EvrRtxMutexError(NULL, (int32_t)osErrorNoMemory);
  }

  return mutex;
}

/// Get name of a Mutex object.
/// \note API identical to osMutexGetName
static const char *svcRtxMutexGetName (osMutexId_t mutex_id) {
  os_mutex_t *mutex = osRtxMutexId(mutex_id);

  // Check parameters
  if (!IsMutexPtrValid(mutex) || (mutex->id != osRtxIdMutex)) {
    EvrRtxMutexGetName(mutex, NULL);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return NULL;
  }

  EvrRtxMutexGetName(mutex, mutex->name);

  return mutex->name;
}

/// Acquire a Mutex or timeout if it is locked.
/// \note API identical to osMutexAcquire
static osStatus_t svcRtxMutexAcquire (osMutexId_t mutex_id, uint32_t timeout) {
  os_mutex_t  *mutex = osRtxMutexId(mutex_id);
  os_thread_t *thread;
  osStatus_t   status;

  // Check running thread
  thread = osRtxThreadGetRunning();
  if (thread == NULL) {
    EvrRtxMutexError(mutex, osRtxErrorKernelNotRunning);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osError;
  }

  // Check parameters
  if (!IsMutexPtrValid(mutex) || (mutex->id != osRtxIdMutex)) {
    EvrRtxMutexError(mutex, (int32_t)osErrorParameter);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osErrorParameter;
  }

#ifdef RTX_SAFETY_CLASS
  // Check running thread safety class
  if ((thread->attr >> osRtxAttrClass_Pos) < (mutex->attr >> osRtxAttrClass_Pos)) {
    EvrRtxMutexError(mutex, (int32_t)osErrorSafetyClass);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osErrorSafetyClass;
  }
#endif

  // Check if Mutex is not locked
  if (mutex->lock == 0U) {
    // Acquire Mutex
    mutex->owner_thread = thread;
    mutex->owner_prev   = NULL;
    mutex->owner_next   = thread->mutex_list;
    if (thread->mutex_list != NULL) {
      thread->mutex_list->owner_prev = mutex;
    }
    thread->mutex_list = mutex;
    mutex->lock = 1U;
    EvrRtxMutexAcquired(mutex, mutex->lock);
    status = osOK;
  } else {
    // Check if Mutex is recursive and running Thread is the owner
    if (((mutex->attr & osMutexRecursive) != 0U) && (mutex->owner_thread == thread)) {
      // Try to increment lock counter
      if (mutex->lock == osRtxMutexLockLimit) {
        EvrRtxMutexError(mutex, osRtxErrorMutexLockLimit);
        status = osErrorResource;
      } else {
        mutex->lock++;
        EvrRtxMutexAcquired(mutex, mutex->lock);
        status = osOK;
      }
    } else {
      // Check if timeout is specified
      if (timeout != 0U) {
        // Check if Priority inheritance protocol is enabled
        if ((mutex->attr & osMutexPrioInherit) != 0U) {
          // Raise priority of owner Thread if lower than priority of running Thread
          if (mutex->owner_thread->priority < thread->priority) {
            mutex->owner_thread->priority = thread->priority;
            osRtxThreadListSort(mutex->owner_thread);
          }
        }
        EvrRtxMutexAcquirePending(mutex, timeout);
        // Suspend current Thread
        if (osRtxThreadWaitEnter(osRtxThreadWaitingMutex, timeout)) {
          osRtxThreadListPut(osRtxObject(mutex), thread);
        } else {
          EvrRtxMutexAcquireTimeout(mutex);
        }
        status = osErrorTimeout;
      } else {
        EvrRtxMutexNotAcquired(mutex);
        status = osErrorResource;
      }
    }
  }

  return status;
}

/// Release a Mutex that was acquired by osMutexAcquire.
/// \note API identical to osMutexRelease
static osStatus_t svcRtxMutexRelease (osMutexId_t mutex_id) {
        os_mutex_t  *mutex = osRtxMutexId(mutex_id);
  const os_mutex_t  *mutex0;
        os_thread_t *thread;
        int8_t       priority;

  // Check running thread
  thread = osRtxThreadGetRunning();
  if (thread == NULL) {
    EvrRtxMutexError(mutex, osRtxErrorKernelNotRunning);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osError;
  }

  // Check parameters
  if (!IsMutexPtrValid(mutex) || (mutex->id != osRtxIdMutex)) {
    EvrRtxMutexError(mutex, (int32_t)osErrorParameter);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osErrorParameter;
  }

  // Check if Mutex is not locked
  if (mutex->lock == 0U) {
    EvrRtxMutexError(mutex, osRtxErrorMutexNotLocked);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osErrorResource;
  }

  // Check if running Thread is not the owner
  if (mutex->owner_thread != thread) {
    EvrRtxMutexError(mutex, osRtxErrorMutexNotOwned);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osErrorResource;
  }

  // Decrement Lock counter
  mutex->lock--;
  EvrRtxMutexReleased(mutex, mutex->lock);

  // Check Lock counter
  if (mutex->lock == 0U) {

    // Remove Mutex from Thread owner list
    if (mutex->owner_next != NULL) {
      mutex->owner_next->owner_prev = mutex->owner_prev;
    }
    if (mutex->owner_prev != NULL) {
      mutex->owner_prev->owner_next = mutex->owner_next;
    } else {
      thread->mutex_list = mutex->owner_next;
    }

    // Restore running Thread priority
    priority = thread->priority_base;
    mutex0   = thread->mutex_list;
    // Check mutexes owned by running Thread
    while (mutex0 != NULL) {
      if ((mutex0->attr & osMutexPrioInherit) != 0U) {
        if ((mutex0->thread_list != NULL) && (mutex0->thread_list->priority > priority)) {
          // Higher priority Thread is waiting for Mutex
          priority = mutex0->thread_list->priority;
        }
      }
      mutex0 = mutex0->owner_next;
    }
    thread->priority = priority;

    // Check if Thread is waiting for a Mutex
    if (mutex->thread_list != NULL) {
      // Wakeup waiting Thread with highest Priority
      thread = osRtxThreadListGet(osRtxObject(mutex));
      osRtxThreadWaitExit(thread, (uint32_t)osOK, FALSE);
      // Thread is the new Mutex owner
      mutex->owner_thread = thread;
      mutex->owner_prev   = NULL;
      mutex->owner_next   = thread->mutex_list;
      if (thread->mutex_list != NULL) {
        thread->mutex_list->owner_prev = mutex;
      }
      thread->mutex_list = mutex;
      mutex->lock = 1U;
      EvrRtxMutexAcquired(mutex, 1U);
    }

    osRtxThreadDispatch(NULL);
  }

  return osOK;
}

/// Get Thread which owns a Mutex object.
/// \note API identical to osMutexGetOwner
static osThreadId_t svcRtxMutexGetOwner (osMutexId_t mutex_id) {
  os_mutex_t *mutex = osRtxMutexId(mutex_id);

  // Check parameters
  if (!IsMutexPtrValid(mutex) || (mutex->id != osRtxIdMutex)) {
    EvrRtxMutexGetOwner(mutex, NULL);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return NULL;
  }

  // Check if Mutex is not locked
  if (mutex->lock == 0U) {
    EvrRtxMutexGetOwner(mutex, NULL);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return NULL;
  }

  EvrRtxMutexGetOwner(mutex, mutex->owner_thread);

  return mutex->owner_thread;
}

/// Delete a Mutex object.
/// \note API identical to osMutexDelete
static osStatus_t svcRtxMutexDelete (osMutexId_t mutex_id) {
        os_mutex_t  *mutex = osRtxMutexId(mutex_id);
#ifdef RTX_SAFETY_CLASS
  const os_thread_t *thread;
#endif

  // Check parameters
  if (!IsMutexPtrValid(mutex) || (mutex->id != osRtxIdMutex)) {
    EvrRtxMutexError(mutex, (int32_t)osErrorParameter);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osErrorParameter;
  }

#ifdef RTX_SAFETY_CLASS
  // Check running thread safety class
  thread = osRtxThreadGetRunning();
  if ((thread != NULL) &&
      ((thread->attr >> osRtxAttrClass_Pos) < (mutex->attr >> osRtxAttrClass_Pos))) {
    EvrRtxMutexError(mutex, (int32_t)osErrorSafetyClass);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return osErrorSafetyClass;
  }
#endif

  // Unlock the mutex owner
  if (osRtxMutexOwnerUnlock(mutex)) {
    osRtxThreadDispatch(NULL);
  }

  osRtxMutexDestroy(mutex);

  return osOK;
}

//  Service Calls definitions
//lint ++flb "Library Begin" [MISRA Note 11]
SVC0_1(MutexNew,      osMutexId_t,  const osMutexAttr_t *)
SVC0_1(MutexGetName,  const char *, osMutexId_t)
SVC0_2(MutexAcquire,  osStatus_t,   osMutexId_t, uint32_t)
SVC0_1(MutexRelease,  osStatus_t,   osMutexId_t)
SVC0_1(MutexGetOwner, osThreadId_t, osMutexId_t)
SVC0_1(MutexDelete,   osStatus_t,   osMutexId_t)
//lint --flb "Library End"


//  ==== Public API ====

/// Create and Initialize a Mutex object.
osMutexId_t osMutexNew (const osMutexAttr_t *attr) {
  osMutexId_t mutex_id;

  EvrRtxMutexNew(attr);
  if (IsException() || IsIrqMasked()) {
    EvrRtxMutexError(NULL, (int32_t)osErrorISR);
    mutex_id = NULL;
  } else {
    mutex_id = __svcMutexNew(attr);
  }
  return mutex_id;
}

/// Get name of a Mutex object.
const char *osMutexGetName (osMutexId_t mutex_id) {
  const char *name;

  if (IsException() || IsIrqMasked()) {
    name = svcRtxMutexGetName(mutex_id);
  } else {
    name =  __svcMutexGetName(mutex_id);
  }
  return name;
}

/// Acquire a Mutex or timeout if it is locked.
osStatus_t osMutexAcquire (osMutexId_t mutex_id, uint32_t timeout) {
  osStatus_t status;

  EvrRtxMutexAcquire(mutex_id, timeout);
  if (IsException() || IsIrqMasked()) {
    EvrRtxMutexError(mutex_id, (int32_t)osErrorISR);
    status = osErrorISR;
  } else {
    status = __svcMutexAcquire(mutex_id, timeout);
  }
  return status;
}

/// Release a Mutex that was acquired by \ref osMutexAcquire.
osStatus_t osMutexRelease (osMutexId_t mutex_id) {
  osStatus_t status;

  EvrRtxMutexRelease(mutex_id);
  if (IsException() || IsIrqMasked()) {
    EvrRtxMutexError(mutex_id, (int32_t)osErrorISR);
    status = osErrorISR;
  } else {
    status = __svcMutexRelease(mutex_id);
  }
  return status;
}

/// Get Thread which owns a Mutex object.
osThreadId_t osMutexGetOwner (osMutexId_t mutex_id) {
  osThreadId_t thread;

  if (IsException() || IsIrqMasked()) {
    EvrRtxMutexGetOwner(mutex_id, NULL);
    thread = NULL;
  } else {
    thread = __svcMutexGetOwner(mutex_id);
  }
  return thread;
}

/// Delete a Mutex object.
osStatus_t osMutexDelete (osMutexId_t mutex_id) {
  osStatus_t status;

  EvrRtxMutexDelete(mutex_id);
  if (IsException() || IsIrqMasked()) {
    EvrRtxMutexError(mutex_id, (int32_t)osErrorISR);
    status = osErrorISR;
  } else {
    status = __svcMutexDelete(mutex_id);
  }
  return status;
}
