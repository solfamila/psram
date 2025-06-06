/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_fir_decimate_f64.c
 * Description:  FIR decimation for floating-point sequences
 *
 * $Date:        17 February 2024
 * $Revision:    V1.16.0
 *
 * Target Processor: Cortex-M and Cortex-A cores
 * -------------------------------------------------------------------- */
/*
 * Copyright (C) 2010-2024 ARM Limited or its affiliates. All rights reserved.
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

#include "dsp/filtering_functions.h"

/**
  @ingroup groupFilters
 */

/**
  @addtogroup FIR_decimate
  @{
 */

/**
  @brief         Processing function for floating-point FIR decimator.
  @param[in]     S         points to an instance of the floating-point FIR decimator structure
  @param[in]     pSrc      points to the block of input data
  @param[out]    pDst      points to the block of output data
  @param[in]     blockSize number of input samples to process
 */

ARM_DSP_ATTRIBUTE void arm_fir_decimate_f64(
  const arm_fir_decimate_instance_f64 * S,
  const float64_t * pSrc,
        float64_t * pDst,
        uint32_t blockSize)
{
        float64_t *pState = S->pState;                 /* State pointer */
  const float64_t *pCoeffs = S->pCoeffs;               /* Coefficient pointer */
        float64_t *pStateCur;                          /* Points to the current sample of the state */
        float64_t *px0;                                /* Temporary pointer for state buffer */
  const float64_t *pb;                                 /* Temporary pointer for coefficient buffer */
        float64_t x0, c0;                              /* Temporary variables to hold state and coefficient values */
        float64_t acc0;                                /* Accumulator */
        uint32_t numTaps = S->numTaps;                 /* Number of filter coefficients in the filter */
        uint32_t i, tapCnt, blkCnt, outBlockSize = blockSize / S->M;  /* Loop counters */

#if defined (ARM_MATH_LOOPUNROLL)
        float64_t *px1, *px2, *px3;
        float64_t x1, x2, x3;
        float64_t acc1, acc2, acc3;
#endif

  /* S->pState buffer contains previous frame (numTaps - 1) samples */
  /* pStateCur points to the location where the new input data should be written */
  pStateCur = S->pState + (numTaps - 1U);

#if defined (ARM_MATH_LOOPUNROLL)

    /* Loop unrolling: Compute 4 samples at a time */
  blkCnt = outBlockSize >> 2U;

  /* Samples loop unrolled by 4 */
  while (blkCnt > 0U)
  {
    /* Copy 4 * decimation factor number of new input samples into the state buffer */
    i = S->M * 4;

    do
    {
      *pStateCur++ = *pSrc++;

    } while (--i);

    /* Set accumulators to zero */
    acc0 = 0.0;
    acc1 = 0.0;
    acc2 = 0.0;
    acc3 = 0.0;

    /* Initialize state pointer for all the samples */
    px0 = pState;
    px1 = pState + S->M;
    px2 = pState + 2 * S->M;
    px3 = pState + 3 * S->M;

    /* Initialize coeff pointer */
    pb = pCoeffs;

    /* Loop unrolling: Compute 4 taps at a time */
    tapCnt = numTaps >> 2U;

    while (tapCnt > 0U)
    {
      /* Read the b[numTaps-1] coefficient */
      c0 = *(pb++);

      /* Read x[n-numTaps-1] sample for acc0 */
      x0 = *(px0++);
      /* Read x[n-numTaps-1] sample for acc1 */
      x1 = *(px1++);
      /* Read x[n-numTaps-1] sample for acc2 */
      x2 = *(px2++);
      /* Read x[n-numTaps-1] sample for acc3 */
      x3 = *(px3++);

      /* Perform the multiply-accumulate */
      acc0 += x0 * c0;
      acc1 += x1 * c0;
      acc2 += x2 * c0;
      acc3 += x3 * c0;

      /* Read the b[numTaps-2] coefficient */
      c0 = *(pb++);

      /* Read x[n-numTaps-2] sample for acc0, acc1, acc2, acc3 */
      x0 = *(px0++);
      x1 = *(px1++);
      x2 = *(px2++);
      x3 = *(px3++);

      /* Perform the multiply-accumulate */
      acc0 += x0 * c0;
      acc1 += x1 * c0;
      acc2 += x2 * c0;
      acc3 += x3 * c0;

      /* Read the b[numTaps-3] coefficient */
      c0 = *(pb++);

      /* Read x[n-numTaps-3] sample acc0, acc1, acc2, acc3 */
      x0 = *(px0++);
      x1 = *(px1++);
      x2 = *(px2++);
      x3 = *(px3++);

      /* Perform the multiply-accumulate */
      acc0 += x0 * c0;
      acc1 += x1 * c0;
      acc2 += x2 * c0;
      acc3 += x3 * c0;

      /* Read the b[numTaps-4] coefficient */
      c0 = *(pb++);

      /* Read x[n-numTaps-4] sample acc0, acc1, acc2, acc3 */
      x0 = *(px0++);
      x1 = *(px1++);
      x2 = *(px2++);
      x3 = *(px3++);

      /* Perform the multiply-accumulate */
      acc0 += x0 * c0;
      acc1 += x1 * c0;
      acc2 += x2 * c0;
      acc3 += x3 * c0;

      /* Decrement loop counter */
      tapCnt--;
    }

    /* Loop unrolling: Compute remaining taps */
    tapCnt = numTaps % 0x4U;

    while (tapCnt > 0U)
    {
      /* Read coefficients */
      c0 = *(pb++);

      /* Fetch state variables for acc0, acc1, acc2, acc3 */
      x0 = *(px0++);
      x1 = *(px1++);
      x2 = *(px2++);
      x3 = *(px3++);

      /* Perform the multiply-accumulate */
      acc0 += x0 * c0;
      acc1 += x1 * c0;
      acc2 += x2 * c0;
      acc3 += x3 * c0;

      /* Decrement loop counter */
      tapCnt--;
    }

    /* Advance the state pointer by the decimation factor
     * to process the next group of decimation factor number samples */
    pState = pState + S->M * 4;

    /* The result is in the accumulator, store in the destination buffer. */
    *pDst++ = acc0;
    *pDst++ = acc1;
    *pDst++ = acc2;
    *pDst++ = acc3;

    /* Decrement loop counter */
    blkCnt--;
  }

  /* Loop unrolling: Compute remaining samples */
  blkCnt = outBlockSize % 0x4U;

#else

  /* Initialize blkCnt with number of samples */
  blkCnt = outBlockSize;

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */

  while (blkCnt > 0U)
  {
    /* Copy decimation factor number of new input samples into the state buffer */
    i = S->M;

    do
    {
      *pStateCur++ = *pSrc++;

    } while (--i);

    /* Set accumulator to zero */
    acc0 = 0.0;

    /* Initialize state pointer */
    px0 = pState;

    /* Initialize coeff pointer */
    pb = pCoeffs;

#if defined (ARM_MATH_LOOPUNROLL)

    /* Loop unrolling: Compute 4 taps at a time */
    tapCnt = numTaps >> 2U;

    while (tapCnt > 0U)
    {
      /* Read the b[numTaps-1] coefficient */
      c0 = *pb++;

      /* Read x[n-numTaps-1] sample */
      x0 = *px0++;

      /* Perform the multiply-accumulate */
      acc0 += x0 * c0;

      /* Read the b[numTaps-2] coefficient */
      c0 = *pb++;

      /* Read x[n-numTaps-2] sample */
      x0 = *px0++;

      /* Perform the multiply-accumulate */
      acc0 += x0 * c0;

      /* Read the b[numTaps-3] coefficient */
      c0 = *pb++;

      /* Read x[n-numTaps-3] sample */
      x0 = *px0++;

      /* Perform the multiply-accumulate */
      acc0 += x0 * c0;

      /* Read the b[numTaps-4] coefficient */
      c0 = *pb++;

      /* Read x[n-numTaps-4] sample */
      x0 = *px0++;

      /* Perform the multiply-accumulate */
      acc0 += x0 * c0;

      /* Decrement loop counter */
      tapCnt--;
    }

    /* Loop unrolling: Compute remaining taps */
    tapCnt = numTaps % 0x4U;

#else

    /* Initialize tapCnt with number of taps */
    tapCnt = numTaps;

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */

    while (tapCnt > 0U)
    {
      /* Read coefficients */
      c0 = *pb++;

      /* Fetch 1 state variable */
      x0 = *px0++;

      /* Perform the multiply-accumulate */
      acc0 += x0 * c0;

      /* Decrement loop counter */
      tapCnt--;
    }

    /* Advance the state pointer by the decimation factor
     * to process the next group of decimation factor number samples */
    pState = pState + S->M;

    /* The result is in the accumulator, store in the destination buffer. */
    *pDst++ = acc0;

    /* Decrement loop counter */
    blkCnt--;
  }

  /* Processing is complete.
     Now copy the last numTaps - 1 samples to the satrt of the state buffer.
     This prepares the state buffer for the next function call. */

  /* Points to the start of the state buffer */
  pStateCur = S->pState;

#if defined (ARM_MATH_LOOPUNROLL)

  /* Loop unrolling: Compute 4 taps at a time */
  tapCnt = (numTaps - 1U) >> 2U;

  /* Copy data */
  while (tapCnt > 0U)
  {
    *pStateCur++ = *pState++;
    *pStateCur++ = *pState++;
    *pStateCur++ = *pState++;
    *pStateCur++ = *pState++;

    /* Decrement loop counter */
    tapCnt--;
  }

  /* Loop unrolling: Compute remaining taps */
  tapCnt = (numTaps - 1U) % 0x04U;

#else

  /* Initialize tapCnt with number of taps */
  tapCnt = (numTaps - 1U);

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */

  /* Copy data */
  while (tapCnt > 0U)
  {
    *pStateCur++ = *pState++;

    /* Decrement loop counter */
    tapCnt--;
  }

}
/**
  @} end of FIR_decimate group
 */
