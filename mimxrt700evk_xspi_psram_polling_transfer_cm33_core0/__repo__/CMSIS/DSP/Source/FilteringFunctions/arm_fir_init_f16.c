/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_fir_init_f16.c
 * Description:  Floating-point FIR filter initialization function
 *
 * $Date:        23 April 2021
 * $Revision:    V1.9.0
 *
 * Target Processor: Cortex-M and Cortex-A cores
 * -------------------------------------------------------------------- */
/*
 * Copyright (C) 2010-2021 ARM Limited or its affiliates. All rights reserved.
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
 */

#include "dsp/filtering_functions_f16.h"

#if defined(ARM_FLOAT16_SUPPORTED)

/**
  @ingroup groupFilters
 */

/**
  @addtogroup FIR
  @{
 */

/**
  @brief         Initialization function for the floating-point FIR filter.
  @param[in,out] S          points to an instance of the floating-point FIR filter structure
  @param[in]     numTaps    number of filter coefficients in the filter
  @param[in]     pCoeffs    points to the filter coefficients buffer
  @param[in]     pState     points to the state buffer
  @param[in]     blockSize  number of samples processed per call

  @par           Details
                   <code>pCoeffs</code> points to the array of filter coefficients stored in time reversed order:
  <pre>
      {b[numTaps-1], b[numTaps-2], b[N-2], ..., b[1], b[0]}
  </pre>
  @par
                   <code>pState</code> points to the array of state variables.
                   <code>pState</code> is of length <code>numTaps+blockSize-1</code> samples (except for Helium - see below), where <code>blockSize</code> is the number of input samples processed by each call to <code>arm_fir_f16()</code>.
  @par          Initialization of Helium version
                 For Helium version the array of coefficients must be a multiple of 4 (4a) even if less
                 then 4a coefficients are defined in the FIR. The additional coefficients 
                 (4a - numTaps) must be set to 0.
                 numTaps is still set to its right value in the init function. It means that
                 the implementation may require to read more coefficients due to the vectorization and
                 to avoid having to manage too many different cases in the code.


  @par          Helium state buffer
                 The state buffer must contain some additional temporary data
                 used during the computation but which is not the state of the FIR.
                 The first 8*ceil(blockSize/8) samples are temporary data.
                 The remaining samples are the state of the FIR filter.
                 So the state buffer has size <code> numTaps + 8*ceil(blockSize/8) + blockSize - 1 </code>

 */

ARM_DSP_ATTRIBUTE void arm_fir_init_f16(
        arm_fir_instance_f16 * S,
        uint16_t numTaps,
  const float16_t * pCoeffs,
        float16_t * pState,
        uint32_t blockSize)
{
  /* Assign filter taps */
  S->numTaps = numTaps;

  /* Assign coefficient pointer */
  S->pCoeffs = pCoeffs;

  /* Clear state buffer. The size is always (blockSize + numTaps - 1) */
#if defined(ARM_MATH_MVE_FLOAT16) && !defined(ARM_MATH_AUTOVECTORIZE)
  memset(pState, 0, (numTaps + (blockSize - 1U) + ARM_ROUND_UP(blockSize, 8)) * sizeof(float16_t));
#else
  memset(pState, 0, (numTaps + (blockSize - 1U)) * sizeof(float16_t));
#endif 

  /* Assign state pointer */
  S->pState = pState;
}

/**
  @} end of FIR group
 */

#endif /* #if defined(ARM_FLOAT16_SUPPORTED) */
