/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_fir_interpolate_q31.c
 * Description:  Q31 FIR interpolation
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

#include "dsp/filtering_functions.h"

/**
  @ingroup groupFilters
 */

/**
  @addtogroup FIR_Interpolate
  @{
 */

/**
  @brief         Processing function for the Q31 FIR interpolator.
  @param[in]     S          points to an instance of the Q31 FIR interpolator structure
  @param[in]     pSrc       points to the block of input data
  @param[out]    pDst       points to the block of output data
  @param[in]     blockSize  number of input samples to process

  @par           Scaling and Overflow Behavior
                   The function is implemented using an internal 64-bit accumulator.
                   The accumulator has a 2.62 format and maintains full precision of the intermediate multiplication results but provides only a single guard bit.
                   Thus, if the accumulator result overflows it wraps around rather than clip.
                   In order to avoid overflows completely the input signal must be scaled down by <code>1/(numTaps/L)</code>.
                   since <code>numTaps/L</code> additions occur per output sample.
                   After all multiply-accumulates are performed, the 2.62 accumulator is truncated to 1.32 format and then saturated to 1.31 format.
 */

#if defined(ARM_MATH_MVEI) && !defined(ARM_MATH_AUTOVECTORIZE)

#include "arm_helium_utils.h"
ARM_DSP_ATTRIBUTE void arm_fir_interpolate_q31(
  const arm_fir_interpolate_instance_q31 * S,
  const q31_t * pSrc,
        q31_t * pDst,
        uint32_t blockSize)
{
    q31_t    *pState = S->pState;   /* State pointer */
    const q31_t    *pCoeffs = S->pCoeffs; /* Coefficient pointer */
    q31_t    *pStateCurnt;      /* Points to the current sample of the state */
    const q31_t    *ptr1, *ptr2;      /* Temporary pointers for state and coefficient buffers */

    uint32_t  i, blkCnt;        /* Loop counters */
    uint16_t  phaseLen = S->phaseLength;    /* Length of each polyphase filter component */
    uint32_t  strides[4] = { 0, 1 * S->L, 2 * S->L, 3 * S->L };
    uint32x4_t vec_strides0 =  vld1q_u32(strides);
    uint32x4_t vec_strides1 = vec_strides0 + 1;
    uint32x4_t vec_strides2 = vec_strides0 + 2;
    uint32x4_t vec_strides3 = vec_strides0 + 3;
    q31x4_t vecState, vecCoef;

    /*
     * S->pState buffer contains previous frame (phaseLen - 1) samples
     * pStateCurnt points to the location where the new input data should be written
     */
    pStateCurnt = S->pState + ((q31_t) phaseLen - 1);
    /*
     * Total number of intput samples
     */
    blkCnt = blockSize;
    /*
     * Loop over the blockSize.
     */
    while (blkCnt > 0U)
    {
        /*
         * Copy new input sample into the state buffer
         */
        *pStateCurnt++ = *pSrc++;
        /*
         * Loop over the Interpolation factor.
         */
        i = S->L;
        while (i > 0U)
        {
            /*
             * Initialize state pointer
             */
            ptr1 = pState;
            if (i >= 4)
            {
                /*
                 * Initialize coefficient pointer
                 */
                ptr2 = pCoeffs + (i - 1 - 3U);

                q63_t     acc0 = 0LL;
                q63_t     acc1 = 0LL;
                q63_t     acc2 = 0LL;
                q63_t     acc3 = 0LL;

                uint32_t  tapCnt = phaseLen >> 2;
                while (tapCnt > 0U)
                {
                    vecState = vldrwq_s32(ptr1);

                    vecCoef = vldrwq_gather_shifted_offset_s32(ptr2, vec_strides3);
                    acc0 = vrmlaldavhaq(acc0, vecState, vecCoef);

                    vecCoef = vldrwq_gather_shifted_offset_s32(ptr2, vec_strides2);
                    acc1 = vrmlaldavhaq(acc1, vecState, vecCoef);

                    vecCoef = vldrwq_gather_shifted_offset_s32(ptr2, vec_strides1);
                    acc2 = vrmlaldavhaq(acc2, vecState, vecCoef);

                    vecCoef = vldrwq_gather_shifted_offset_s32(ptr2, vec_strides0);
                    acc3 = vrmlaldavhaq(acc3, vecState, vecCoef);

                    ptr1 += 4;
                    ptr2 = ptr2 + S->L * 4;
                    tapCnt--;
                }
                tapCnt = phaseLen & 3;
                if (tapCnt > 0U)
                {
                    mve_pred16_t p0 = vctp32q(tapCnt);

                    vecState = vldrwq_z_s32(ptr1, p0);

                    vecCoef = vldrwq_gather_shifted_offset_z_s32(ptr2, vec_strides3, p0);
                    acc0 = vrmlaldavhaq(acc0, vecState, vecCoef);

                    vecCoef = vldrwq_gather_shifted_offset_z_s32(ptr2, vec_strides2, p0);
                    acc1 = vrmlaldavhaq(acc1, vecState, vecCoef);

                    vecCoef = vldrwq_gather_shifted_offset_z_s32(ptr2, vec_strides1, p0);
                    acc2 = vrmlaldavhaq(acc2, vecState, vecCoef);

                    vecCoef = vldrwq_gather_shifted_offset_z_s32(ptr2, vec_strides0, p0);
                    acc3 = vrmlaldavhaq(acc3, vecState, vecCoef);
                }

                acc0 = asrl(acc0, 31 - 8);
                acc1 = asrl(acc1, 31 - 8);
                acc2 = asrl(acc2, 31 - 8);
                acc3 = asrl(acc3, 31 - 8);

                *pDst++ = (q31_t) acc0;
                *pDst++ = (q31_t) acc1;
                *pDst++ = (q31_t) acc2;
                *pDst++ = (q31_t) acc3;
                i -= 4;
            }
            else if (i >= 3)
            {
                /*
                 * Initialize coefficient pointer
                 */
                ptr2 = pCoeffs + (i - 1U - 2);

                q63_t     acc0 = 0LL;
                q63_t     acc1 = 0LL;
                q63_t     acc2 = 0LL;

                uint32_t  tapCnt = phaseLen >> 2;
                while (tapCnt > 0U)
                {
                    vecState = vldrwq_s32(ptr1);

                    vecCoef = vldrwq_gather_shifted_offset_s32(ptr2, vec_strides2);
                    acc0 = vrmlaldavhaq(acc0, vecState, vecCoef);

                    vecCoef = vldrwq_gather_shifted_offset_s32(ptr2, vec_strides1);
                    acc1 = vrmlaldavhaq(acc1, vecState, vecCoef);

                    vecCoef = vldrwq_gather_shifted_offset_s32(ptr2, vec_strides0);
                    acc2 = vrmlaldavhaq(acc2, vecState, vecCoef);

                    ptr1 += 4;
                    ptr2 = ptr2 + S->L * 4;
                    tapCnt--;
                }
                tapCnt = phaseLen & 3;
                if (tapCnt > 0U)
                {
                    mve_pred16_t p0 = vctp32q(tapCnt);

                    vecState = vldrwq_z_s32(ptr1, p0);

                    vecCoef = vldrwq_gather_shifted_offset_z_s32(ptr2, vec_strides2, p0);
                    acc0 = vrmlaldavhaq(acc0, vecState, vecCoef);

                    vecCoef = vldrwq_gather_shifted_offset_z_s32(ptr2, vec_strides1, p0);
                    acc1 = vrmlaldavhaq(acc1, vecState, vecCoef);

                    vecCoef = vldrwq_gather_shifted_offset_z_s32(ptr2, vec_strides0, p0);
                    acc2 = vrmlaldavhaq(acc2, vecState, vecCoef);
                }

                acc0 = asrl(acc0, 31 - 8);
                acc1 = asrl(acc1, 31 - 8);
                acc2 = asrl(acc2, 31 - 8);

                *pDst++ = (q31_t) acc0;
                *pDst++ = (q31_t) acc1;
                *pDst++ = (q31_t) acc2;
                i -= 3;
            }
            else if (i >= 2)
            {
                /*
                 * Initialize coefficient pointer
                 */
                ptr2 = pCoeffs + (i - 1U - 1);

                q63_t     acc0 = 0LL;
                q63_t     acc1 = 0LL;

                uint32_t  tapCnt = phaseLen >> 2;
                while (tapCnt > 0U)
                {
                    vecState = vldrwq_s32(ptr1);

                    vecCoef = vldrwq_gather_shifted_offset_s32(ptr2, vec_strides1);
                    acc0 = vrmlaldavhaq(acc0, vecState, vecCoef);

                    vecCoef = vldrwq_gather_shifted_offset_s32(ptr2, vec_strides0);
                    acc1 = vrmlaldavhaq(acc1, vecState, vecCoef);

                    ptr1 += 4;
                    ptr2 = ptr2 + S->L * 4;
                    tapCnt--;
                }
                tapCnt = phaseLen & 3;
                if (tapCnt > 0U)
                {
                    mve_pred16_t p0 = vctp32q(tapCnt);

                    vecState = vldrwq_z_s32(ptr1, p0);

                    vecCoef = vldrwq_gather_shifted_offset_z_s32(ptr2, vec_strides1, p0);
                    acc0 = vrmlaldavhaq(acc0, vecState, vecCoef);

                    vecCoef = vldrwq_gather_shifted_offset_z_s32(ptr2, vec_strides0, p0);
                    acc1 = vrmlaldavhaq(acc1, vecState, vecCoef);
                }

                acc0 = asrl(acc0, 31 - 8);
                acc1 = asrl(acc1, 31 - 8);

                *pDst++ = (q31_t) acc0;
                *pDst++ = (q31_t) acc1;
                i -= 2;
            }
            else
            {
                /*
                 * Initialize coefficient pointer
                 */
                ptr2 = pCoeffs + (i - 1U);

                q63_t     acc0 = 0LL;

                uint32_t  tapCnt = phaseLen >> 2;
                while (tapCnt > 0U)
                {
                    vecState = vldrwq_s32(ptr1);
                    vecCoef = vldrwq_gather_shifted_offset_s32(ptr2, vec_strides0);

                    acc0 = vrmlaldavhaq(acc0, vecState, vecCoef);

                    ptr1 += 4;
                    ptr2 = ptr2 + S->L * 4;
                    tapCnt--;
                }
                tapCnt = phaseLen & 3;
                if (tapCnt > 0U)
                {
                    mve_pred16_t p0 = vctp32q(tapCnt);

                    vecState = vldrwq_z_s32(ptr1, p0);
                    vecCoef = vldrwq_gather_shifted_offset_z_s32(ptr2, vec_strides0, p0);
                    acc0 = vrmlaldavhaq(acc0, vecState, vecCoef);
                }

                acc0 = asrl(acc0, 31 - 8);
                *pDst++ = (q31_t) acc0;
                /*
                 * Decrement the loop counter
                 */
                i--;
            }
        }
        /*
         * Advance the state pointer by 1
         * * to process the next group of interpolation factor number samples
         */
        pState = pState + 1;
        /*
         * Decrement the loop counter
         */
        blkCnt--;
    }

    /*
     * Processing is complete.
     * ** Now copy the last phaseLen - 1 samples to the satrt of the state buffer.
     * ** This prepares the state buffer for the next function call.
     */

    /*
     * Points to the start of the state buffer
     */
    pStateCurnt = S->pState;
    blkCnt = (phaseLen - 1U) >> 2;
    while (blkCnt > 0U)
    {
        vst1q(pStateCurnt, vldrwq_s32(pState));
        pState += 4;
        pStateCurnt += 4;
        blkCnt--;
    }
    blkCnt = (phaseLen - 1U) & 3;
    if (blkCnt > 0U)
    {
        mve_pred16_t p0 = vctp32q(blkCnt);
        vstrwq_p_s32(pStateCurnt, vldrwq_s32(pState), p0);
    }
}
#else
ARM_DSP_ATTRIBUTE void arm_fir_interpolate_q31(
  const arm_fir_interpolate_instance_q31 * S,
  const q31_t * pSrc,
        q31_t * pDst,
        uint32_t blockSize)
{
#if (1)
//#if !defined(ARM_MATH_CM0_FAMILY)

        q31_t *pState = S->pState;                     /* State pointer */
  const q31_t *pCoeffs = S->pCoeffs;                   /* Coefficient pointer */
        q31_t *pStateCur;                              /* Points to the current sample of the state */
        q31_t *ptr1;                                   /* Temporary pointer for state buffer */
  const q31_t *ptr2;                                   /* Temporary pointer for coefficient buffer */
        q63_t sum0;                                    /* Accumulators */
        uint32_t i, blkCnt, tapCnt;                    /* Loop counters */
        uint32_t phaseLen = S->phaseLength;            /* Length of each polyphase filter component */
        uint32_t j;

#if defined (ARM_MATH_LOOPUNROLL)
        q63_t acc0, acc1, acc2, acc3;
        q31_t x0, x1, x2, x3;
        q31_t c0, c1, c2, c3;
#endif

  /* S->pState buffer contains previous frame (phaseLen - 1) samples */
  /* pStateCur points to the location where the new input data should be written */
  pStateCur = S->pState + (phaseLen - 1U);

#if defined (ARM_MATH_LOOPUNROLL)

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = blockSize >> 2U;

  while (blkCnt > 0U)
  {
    /* Copy new input sample into the state buffer */
    *pStateCur++ = *pSrc++;
    *pStateCur++ = *pSrc++;
    *pStateCur++ = *pSrc++;
    *pStateCur++ = *pSrc++;

    /* Address modifier index of coefficient buffer */
    j = 1U;

    /* Loop over the Interpolation factor. */
    i = (S->L);

    while (i > 0U)
    {
      /* Set accumulator to zero */
      acc0 = 0;
      acc1 = 0;
      acc2 = 0;
      acc3 = 0;

      /* Initialize state pointer */
      ptr1 = pState;

      /* Initialize coefficient pointer */
      ptr2 = pCoeffs + (S->L - j);

      /* Loop over the polyPhase length. Unroll by a factor of 4.
         Repeat until we've computed numTaps-(4*S->L) coefficients. */
      tapCnt = phaseLen >> 2U;

      x0 = *(ptr1++);
      x1 = *(ptr1++);
      x2 = *(ptr1++);

      while (tapCnt > 0U)
      {
        /* Read the input sample */
        x3 = *(ptr1++);

        /* Read the coefficient */
        c0 = *(ptr2);

        /* Perform the multiply-accumulate */
        acc0 += (q63_t) x0 * c0;
        acc1 += (q63_t) x1 * c0;
        acc2 += (q63_t) x2 * c0;
        acc3 += (q63_t) x3 * c0;

        /* Read the coefficient */
        c1 = *(ptr2 + S->L);

        /* Read the input sample */
        x0 = *(ptr1++);

        /* Perform the multiply-accumulate */
        acc0 += (q63_t) x1 * c1;
        acc1 += (q63_t) x2 * c1;
        acc2 += (q63_t) x3 * c1;
        acc3 += (q63_t) x0 * c1;

        /* Read the coefficient */
        c2 = *(ptr2 + S->L * 2);

        /* Read the input sample */
        x1 = *(ptr1++);

        /* Perform the multiply-accumulate */
        acc0 += (q63_t) x2 * c2;
        acc1 += (q63_t) x3 * c2;
        acc2 += (q63_t) x0 * c2;
        acc3 += (q63_t) x1 * c2;

        /* Read the coefficient */
        c3 = *(ptr2 + S->L * 3);

        /* Read the input sample */
        x2 = *(ptr1++);

        /* Perform the multiply-accumulate */
        acc0 += (q63_t) x3 * c3;
        acc1 += (q63_t) x0 * c3;
        acc2 += (q63_t) x1 * c3;
        acc3 += (q63_t) x2 * c3;


        /* Upsampling is done by stuffing L-1 zeros between each sample.
         * So instead of multiplying zeros with coefficients,
         * Increment the coefficient pointer by interpolation factor times. */
        ptr2 += 4 * S->L;

        /* Decrement loop counter */
        tapCnt--;
      }

      /* If the polyPhase length is not a multiple of 4, compute the remaining filter taps */
      tapCnt = phaseLen % 0x4U;

      while (tapCnt > 0U)
      {
        /* Read the input sample */
        x3 = *(ptr1++);

        /* Read the coefficient */
        c0 = *(ptr2);

        /* Perform the multiply-accumulate */
        acc0 += (q63_t) x0 * c0;
        acc1 += (q63_t) x1 * c0;
        acc2 += (q63_t) x2 * c0;
        acc3 += (q63_t) x3 * c0;

        /* Increment the coefficient pointer by interpolation factor times. */
        ptr2 += S->L;

        /* update states for next sample processing */
        x0 = x1;
        x1 = x2;
        x2 = x3;

        /* Decrement loop counter */
        tapCnt--;
      }

      /* The result is in the accumulator, store in the destination buffer. */
      *(pDst           ) = (q31_t) (acc0 >> 31);
      *(pDst +     S->L) = (q31_t) (acc1 >> 31);
      *(pDst + 2 * S->L) = (q31_t) (acc2 >> 31);
      *(pDst + 3 * S->L) = (q31_t) (acc3 >> 31);

      pDst++;

      /* Increment the address modifier index of coefficient buffer */
      j++;

      /* Decrement loop counter */
      i--;
    }

    /* Advance the state pointer by 1
     * to process the next group of interpolation factor number samples */
    pState = pState + 4;

    pDst += S->L * 3;

    /* Decrement loop counter */
    blkCnt--;
  }

  /* Loop unrolling: Compute remaining outputs */
  blkCnt = blockSize % 0x4U;

#else

  /* Initialize blkCnt with number of samples */
  blkCnt = blockSize;

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */

  while (blkCnt > 0U)
  {
    /* Copy new input sample into the state buffer */
    *pStateCur++ = *pSrc++;

    /* Address modifier index of coefficient buffer */
    j = 1U;

    /* Loop over the Interpolation factor. */
    i = S->L;
    while (i > 0U)
    {
      /* Set accumulator to zero */
      sum0 = 0;

      /* Initialize state pointer */
      ptr1 = pState;

      /* Initialize coefficient pointer */
      ptr2 = pCoeffs + (S->L - j);

      /* Loop over the polyPhase length.
         Repeat until we've computed numTaps-(4*S->L) coefficients. */

#if defined (ARM_MATH_LOOPUNROLL)

     /* Loop unrolling: Compute 4 outputs at a time */
      tapCnt = phaseLen >> 2U;

      while (tapCnt > 0U)
      {
        /* Perform the multiply-accumulate */
        sum0 += (q63_t) *ptr1++ * *ptr2;

        /* Upsampling is done by stuffing L-1 zeros between each sample.
         * So instead of multiplying zeros with coefficients,
         * Increment the coefficient pointer by interpolation factor times. */
        ptr2 += S->L;

        sum0 += (q63_t) *ptr1++ * *ptr2;
        ptr2 += S->L;

        sum0 += (q63_t) *ptr1++ * *ptr2;
        ptr2 += S->L;

        sum0 += (q63_t) *ptr1++ * *ptr2;
        ptr2 += S->L;

        /* Decrement loop counter */
        tapCnt--;
      }

      /* Loop unrolling: Compute remaining outputs */
      tapCnt = phaseLen % 0x4U;

#else

      /* Initialize tapCnt with number of samples */
      tapCnt = phaseLen;

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */

      while (tapCnt > 0U)
      {
        /* Perform the multiply-accumulate */
        sum0 += (q63_t) *ptr1++ * *ptr2;

        /* Upsampling is done by stuffing L-1 zeros between each sample.
         * So instead of multiplying zeros with coefficients,
         * Increment the coefficient pointer by interpolation factor times. */
        ptr2 += S->L;

        /* Decrement loop counter */
        tapCnt--;
      }

      /* The result is in the accumulator, store in the destination buffer. */
      *pDst++ = (q31_t) (sum0 >> 31);

      /* Increment the address modifier index of coefficient buffer */
      j++;

      /* Decrement the loop counter */
      i--;
    }

    /* Advance the state pointer by 1
     * to process the next group of interpolation factor number samples */
    pState = pState + 1;

    /* Decrement the loop counter */
    blkCnt--;
  }

  /* Processing is complete.
     Now copy the last phaseLen - 1 samples to the satrt of the state buffer.
     This prepares the state buffer for the next function call. */

  /* Points to the start of the state buffer */
  pStateCur = S->pState;

#if defined (ARM_MATH_LOOPUNROLL)

  /* Loop unrolling: Compute 4 outputs at a time */
  tapCnt = (phaseLen - 1U) >> 2U;

  /* copy data */
  while (tapCnt > 0U)
  {
    *pStateCur++ = *pState++;
    *pStateCur++ = *pState++;
    *pStateCur++ = *pState++;
    *pStateCur++ = *pState++;

    /* Decrement loop counter */
    tapCnt--;
  }

  /* Loop unrolling: Compute remaining outputs */
  tapCnt = (phaseLen - 1U) % 0x04U;

#else

    /* Initialize tapCnt with number of samples */
    tapCnt = (phaseLen - 1U);

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */

  /* Copy data */
  while (tapCnt > 0U)
  {
    *pStateCur++ = *pState++;

    /* Decrement loop counter */
    tapCnt--;
  }

#else
/* alternate version for CM0_FAMILY */

        q31_t *pState = S->pState;                     /* State pointer */
  const q31_t *pCoeffs = S->pCoeffs;                   /* Coefficient pointer */
        q31_t *pStateCur;                              /* Points to the current sample of the state */
        q31_t *ptr1;                                   /* Temporary pointer for state buffer */
  const q31_t *ptr2;                                   /* Temporary pointer for coefficient buffer */
        q63_t sum0;                                    /* Accumulators */
        uint32_t i, blkCnt, tapCnt;                    /* Loop counters */
        uint32_t phaseLen = S->phaseLength;            /* Length of each polyphase filter component */

  /* S->pState buffer contains previous frame (phaseLen - 1) samples */
  /* pStateCur points to the location where the new input data should be written */
  pStateCur = S->pState + (phaseLen - 1U);

  /* Total number of intput samples */
  blkCnt = blockSize;

  /* Loop over the blockSize. */
  while (blkCnt > 0U)
  {
    /* Copy new input sample into the state buffer */
    *pStateCur++ = *pSrc++;

    /* Loop over the Interpolation factor. */
    i = S->L;

    while (i > 0U)
    {
      /* Set accumulator to zero */
      sum0 = 0;

      /* Initialize state pointer */
      ptr1 = pState;

      /* Initialize coefficient pointer */
      ptr2 = pCoeffs + (i - 1U);

      /* Loop over the polyPhase length */
      tapCnt = phaseLen;

      while (tapCnt > 0U)
      {
        /* Perform the multiply-accumulate */
        sum0 += ((q63_t) *ptr1++ * *ptr2);

        /* Increment the coefficient pointer by interpolation factor times. */
        ptr2 += S->L;

        /* Decrement the loop counter */
        tapCnt--;
      }

      /* The result is in the accumulator, store in the destination buffer. */
      *pDst++ = (q31_t) (sum0 >> 31);

      /* Decrement loop counter */
      i--;
    }

    /* Advance the state pointer by 1
     * to process the next group of interpolation factor number samples */
    pState = pState + 1;

    /* Decrement loop counter */
    blkCnt--;
  }

  /* Processing is complete.
   ** Now copy the last phaseLen - 1 samples to the start of the state buffer.
   ** This prepares the state buffer for the next function call. */

  /* Points to the start of the state buffer */
  pStateCur = S->pState;

  tapCnt = phaseLen - 1U;

  /* Copy data */
  while (tapCnt > 0U)
  {
    *pStateCur++ = *pState++;

    /* Decrement loop counter */
    tapCnt--;
  }

#endif /* #if !defined(ARM_MATH_CM0_FAMILY) */

}
#endif /* defined(ARM_MATH_MVEI) */
/**
  @} end of FIR_Interpolate group
 */
