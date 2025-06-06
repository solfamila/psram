/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_cmplx_mag_f16.c
 * Description:  Floating-point complex magnitude
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

#include "dsp/complex_math_functions_f16.h"

#if defined(ARM_FLOAT16_SUPPORTED)
/**
  @ingroup groupCmplxMath
 */



/**
  @addtogroup cmplx_mag
  @{
 */

/**
  @brief         Floating-point complex magnitude.
  @param[in]     pSrc        points to input vector
  @param[out]    pDst        points to output vector
  @param[in]     numSamples  number of samples in each vector
 */

#if defined(ARM_MATH_MVE_FLOAT16) && !defined(ARM_MATH_AUTOVECTORIZE)

#include "arm_helium_utils.h"


ARM_DSP_ATTRIBUTE void arm_cmplx_mag_f16(
  const float16_t * pSrc,
        float16_t * pDst,
        uint32_t numSamples)
{
    int32_t blockSize = numSamples;  /* loop counters */
    uint32_t  blkCnt;           /* loop counters */
    f16x8x2_t vecSrc;
    f16x8_t sum;

    /* Compute 4 complex samples at a time */
    blkCnt = blockSize >> 3;
    while (blkCnt > 0U)
    {
        q15x8_t newtonStartVec;
        f16x8_t sumHalf, invSqrt;

        vecSrc = vld2q(pSrc);  
        pSrc += 16;
        sum = vmulq(vecSrc.val[0], vecSrc.val[0]);
        sum = vfmaq(sum, vecSrc.val[1], vecSrc.val[1]);

        /*
         * inlined Fast SQRT using inverse SQRT newton-raphson method
         */

        /* compute initial value */
        newtonStartVec = vdupq_n_s16(INVSQRT_MAGIC_F16) - vshrq((q15x8_t) sum, 1);
        sumHalf = sum * 0.5f;
        /*
         * compute 3 x iterations
         *
         * The more iterations, the more accuracy.
         * If you need to trade a bit of accuracy for more performance,
         * you can comment out the 3rd use of the macro.
         */
        INVSQRT_NEWTON_MVE_F16(invSqrt, sumHalf, (f16x8_t) newtonStartVec);
        INVSQRT_NEWTON_MVE_F16(invSqrt, sumHalf, invSqrt);
        INVSQRT_NEWTON_MVE_F16(invSqrt, sumHalf, invSqrt);
        /*
         * set negative values to 0
         */
        invSqrt = vdupq_m(invSqrt, (float16_t)0.0f, vcmpltq(invSqrt, (float16_t)0.0f));
        /*
         * sqrt(x) = x * invSqrt(x)
         */
        sum = vmulq(sum, invSqrt);
        vstrhq_f16(pDst, sum); 
        pDst += 8;
        /*
         * Decrement the blockSize loop counter
         */
        blkCnt--;
    }
    /*
     * tail
     */
    blkCnt = blockSize & 7;
    if (blkCnt > 0U)
    {
        mve_pred16_t p0 = vctp16q(blkCnt);
        q15x8_t newtonStartVec;
        f16x8_t sumHalf, invSqrt;

        vecSrc = vld2q((float16_t const *)pSrc);
        sum = vmulq(vecSrc.val[0], vecSrc.val[0]);
        sum = vfmaq(sum, vecSrc.val[1], vecSrc.val[1]);

        /*
         * inlined Fast SQRT using inverse SQRT newton-raphson method
         */

        /* compute initial value */
        newtonStartVec = vdupq_n_s16(INVSQRT_MAGIC_F16) - vshrq((q15x8_t) sum, 1);
        sumHalf = vmulq(sum, (float16_t)0.5);
        /*
         * compute 2 x iterations
         */
        INVSQRT_NEWTON_MVE_F16(invSqrt, sumHalf, (f16x8_t) newtonStartVec);
        INVSQRT_NEWTON_MVE_F16(invSqrt, sumHalf, invSqrt);
        /*
         * set negative values to 0
         */
        invSqrt = vdupq_m(invSqrt, (float16_t)0.0, vcmpltq(invSqrt, (float16_t)0.0));
        /*
         * sqrt(x) = x * invSqrt(x)
         */
        sum = vmulq(sum, invSqrt);
        vstrhq_p_f16(pDst, sum, p0);
    }
}

#else
ARM_DSP_ATTRIBUTE void arm_cmplx_mag_f16(
  const float16_t * pSrc,
        float16_t * pDst,
        uint32_t numSamples)
{
  uint32_t blkCnt;                               /* loop counter */
  _Float16 real, imag;                      /* Temporary variables to hold input values */

#if defined (ARM_MATH_LOOPUNROLL) && !defined(ARM_MATH_AUTOVECTORIZE)

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = numSamples >> 2U;

  while (blkCnt > 0U)
  {
    /* C[0] = sqrt(A[0] * A[0] + A[1] * A[1]) */

    real = *pSrc++;
    imag = *pSrc++;

    /* store result in destination buffer. */
    arm_sqrt_f16((real * real) + (imag * imag), pDst++);

    real = *pSrc++;
    imag = *pSrc++;
    arm_sqrt_f16((real * real) + (imag * imag), pDst++);

    real = *pSrc++;
    imag = *pSrc++;
    arm_sqrt_f16((real * real) + (imag * imag), pDst++);

    real = *pSrc++;
    imag = *pSrc++;
    arm_sqrt_f16((real * real) + (imag * imag), pDst++);

    /* Decrement loop counter */
    blkCnt--;
  }

  /* Loop unrolling: Compute remaining outputs */
  blkCnt = numSamples % 0x4U;

#else

  /* Initialize blkCnt with number of samples */
  blkCnt = numSamples;

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */

  while (blkCnt > 0U)
  {
    /* C[0] = sqrt(A[0] * A[0] + A[1] * A[1]) */

    real = *pSrc++;
    imag = *pSrc++;

    /* store result in destination buffer. */
    arm_sqrt_f16((real * real) + (imag * imag), pDst++);

    /* Decrement loop counter */
    blkCnt--;
  }

}
#endif /* defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE) */

/**
  @} end of cmplx_mag group
 */

#endif /* #if defined(ARM_FLOAT16_SUPPORTED) */
