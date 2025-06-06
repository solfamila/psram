/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_biquad_cascade_df2T_f64.c
 * Description:  Processing function for floating-point transposed direct form II Biquad cascade filter
 *
 * $Date:        03 June 2022
 * $Revision:    V1.9.1
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
  @defgroup BiquadCascadeDF2T Biquad Cascade IIR Filters Using a Direct Form II Transposed Structure

  This set of functions implements arbitrary order recursive (IIR) filters using a transposed direct form II structure.
  The filters are implemented as a cascade of second order Biquad sections.
  These functions provide a slight memory savings as compared to the direct form I Biquad filter functions.
  Only floating-point data is supported.

  This function operate on blocks of input and output data and each call to the function
  processes <code>blockSize</code> samples through the filter.
  <code>pSrc</code> points to the array of input data and
  <code>pDst</code> points to the array of output data.
  Both arrays contain <code>blockSize</code> values.

  @par           Algorithm
                   Each Biquad stage implements a second order filter using the difference equation:
  <pre>
     y[n] = b0 * x[n] + d1
     d1 = b1 * x[n] + a1 * y[n] + d2
     d2 = b2 * x[n] + a2 * y[n]
  </pre>
                   where d1 and d2 represent the two state values.
  @par
                   A Biquad filter using a transposed Direct Form II structure is shown below.
                   \image html BiquadDF2Transposed.gif "Single transposed Direct Form II Biquad"
                   Coefficients <code>b0, b1, and b2 </code> multiply the input signal <code>x[n]</code> and are referred to as the feedforward coefficients.
                   Coefficients <code>a1</code> and <code>a2</code> multiply the output signal <code>y[n]</code> and are referred to as the feedback coefficients.
                   Pay careful attention to the sign of the feedback coefficients.
                   Some design tools flip the sign of the feedback coefficients:
  <pre>
     y[n] = b0 * x[n] + d1;
     d1 = b1 * x[n] - a1 * y[n] + d2;
     d2 = b2 * x[n] - a2 * y[n];
  </pre>
                   In this case the feedback coefficients <code>a1</code> and <code>a2</code> must be negated when used with the CMSIS DSP Library.
  @par
                   Higher order filters are realized as a cascade of second order sections.
                   <code>numStages</code> refers to the number of second order stages used.
                   For example, an 8th order filter would be realized with <code>numStages=4</code> second order stages.
                   A 9th order filter would be realized with <code>numStages=5</code> second order stages with the
                   coefficients for one of the stages configured as a first order filter (<code>b2=0</code> and <code>a2=0</code>).
  @par
                   <code>pState</code> points to the state variable array.
                   Each Biquad stage has 2 state variables <code>d1</code> and <code>d2</code>.
                   The state variables are arranged in the <code>pState</code> array as:
  <pre>
      {d11, d12, d21, d22, ...}
  </pre>
                   where <code>d1x</code> refers to the state variables for the first Biquad and
                   <code>d2x</code> refers to the state variables for the second Biquad.
                   The state array has a total length of <code>2*numStages</code> values.
                   The state variables are updated after each block of data is processed; the coefficients are untouched.
  @par
                   The CMSIS library contains Biquad filters in both Direct Form I and transposed Direct Form II.
                   The advantage of the Direct Form I structure is that it is numerically more robust for fixed-point data types.
                   That is why the Direct Form I structure supports Q15 and Q31 data types.
                   The transposed Direct Form II structure, on the other hand, requires a wide dynamic range for the state variables <code>d1</code> and <code>d2</code>.
                   Because of this, the CMSIS library only has a floating-point version of the Direct Form II Biquad.
                   The advantage of the Direct Form II Biquad is that it requires half the number of state variables, 2 rather than 4, per Biquad stage.

  @par           Instance Structure
                   The coefficients and state variables for a filter are stored together in an instance data structure.
                   A separate instance structure must be defined for each filter.
                   Coefficient arrays may be shared among several instances while state variable arrays cannot be shared.

  @par           Init Functions
                   There is also an associated initialization function.
                   The initialization function performs following operations:
                   - Sets the values of the internal structure fields.
                   - Zeros out the values in the state buffer.
                   To do this manually without calling the init function, assign the follow subfields of the instance structure:
                   numStages, pCoeffs, pState. Also set all of the values in pState to zero.
  @par
                   Use of the initialization function is optional except for the vectorized versions (Helium and Neon).
                   However, if the initialization function is used, then the instance structure cannot be placed into a const data section.
                   To place an instance structure into a const data section, the instance structure must be manually initialized.
                   Set the values in the state buffer to zeros before static initialization.
                   For example, to statically initialize the instance structure use
  <pre>
      arm_biquad_cascade_df2T_instance_f64 S1 = {numStages, pState, pCoeffs};
      arm_biquad_cascade_df2T_instance_f32 S1 = {numStages, pState, pCoeffs};
  </pre>
                   where <code>numStages</code> is the number of Biquad stages in the filter;
                   <code>pState</code> is the address of the state buffer.
                   <code>pCoeffs</code> is the address of the coefficient buffer;

  @par           Neon version
                  For Neon version, the function arm_biquad_cascade_df2T_compute_coefs_x must be
                  used in addition to arm_biquad_cascade_df2T_init_x.

                  See the documentation of arm_biquad_cascade_df2T_init_x for more details.

*/

/**
  @addtogroup BiquadCascadeDF2T
  @{
 */

/**
 @brief         Processing function for the floating-point transposed direct form II Biquad cascade filter.
 @param[in]     S         points to an instance of the filter data structure
 @param[in]     pSrc      points to the block of input data
 @param[out]    pDst      points to the block of output data
 @param[in]     blockSize number of samples to process
 */


#if defined(ARM_MATH_NEON) && defined(__aarch64__)
ARM_DSP_ATTRIBUTE void arm_biquad_cascade_df2T_f64(
    const arm_biquad_cascade_df2T_instance_f64 * S,
    const float64_t * pSrc,
    float64_t * pDst,
    uint32_t  blockSize)
{
    
    
    
    const float64_t *pIn = pSrc;                  /*  source pointer            */
    float64_t Xn0;
    float64_t acc0;
    float64_t *pOut = pDst;                 /*  destination pointer       */
    float64_t *pState = S->pState;          /*  State pointer             */
    uint32_t  sample, stage = S->numStages; /*  loop counters             */
    float64_t const *pCurCoeffs =          /*  coefficient pointer       */
    (float64_t const *) S->pCoeffs;
    float64x2_t b0Coeffs, a0Coeffs;           /*  Coefficients vector       */
    float64x2_t state;                        /*  State vector*/
    float64x2_t zeroV = vdupq_n_f64(0);
    
    float64_t b0 ;
    
    
    do
    {
        
        /* Reading the coefficients */
        b0 = *pCurCoeffs++ ;
        b0Coeffs = vld1q_f64(pCurCoeffs);
        pCurCoeffs += 2 ;
        a0Coeffs = vld1q_f64(pCurCoeffs);
        pCurCoeffs +=2 ;
        
        state = vld1q_f64(pState);
        
        sample = blockSize >> 1U;
        while (sample > 0U)
        {
            
            /* y[n] = b0 * x[n] + d1 */
            /* d1 = b1 * x[n] + a1 * y[n] + d2 */
            /* d2 = b2 * x[n] + a2 * y[n] */
            
            Xn0 = *pIn++ ;
            
            /* Calculation of acc0*/
            acc0 = b0*Xn0+vgetq_lane_f64(state, 0);
            
            
            /*
             *  final                  initial
             *  state   b0Coeffs        state  a0Coeffs
             *   |        |              |        |
             *   __       __             __       __
             *  /  \     /  \           /  \     /  \
             * | d1 | = | b1 | * Xn0 + | d2 | + | a1 | x acc0
             * | d2 |   | b2 |         | 0  |   | a2 |
             *  \__/     \__/           \__/     \__/
             */
            
            /* state -> initial state (see above) */
            
            state = vextq_f64(state, zeroV, 1);
            
            /* Calculation of final state */
            state = vfmaq_n_f64(state, b0Coeffs, Xn0);
            state = vfmaq_n_f64(state, a0Coeffs, acc0);
            
            *pOut++ = acc0 ;
            
            /* y[n] = b0 * x[n] + d1 */
            /* d1 = b1 * x[n] + a1 * y[n] + d2 */
            /* d2 = b2 * x[n] + a2 * y[n] */
            
            Xn0 = *pIn++ ;
            
            /* Calculation of acc0*/
            acc0 = b0*Xn0+vgetq_lane_f64(state, 0);
            
            
            /*
             *  final                  initial
             *  state   b0Coeffs        state  a0Coeffs
             *   |        |              |        |
             *   __       __             __       __
             *  /  \     /  \           /  \     /  \
             * | d1 | = | b1 | * Xn0 + | d2 | + | a1 | x acc0
             * | d2 |   | b2 |         | 0  |   | a2 |
             *  \__/     \__/           \__/     \__/
             */
            
            /* state -> initial state (see above) */
            
            state = vextq_f64(state, zeroV, 1);
            
            /* Calculation of final state */
            state = vfmaq_n_f64(state, b0Coeffs, Xn0);
            state = vfmaq_n_f64(state, a0Coeffs, acc0);
            
            *pOut++ = acc0;
            sample--;
        }
        sample = blockSize & 1 ;
        while (sample > 0U)
        {
            
            /* y[n] = b0 * x[n] + d1 */
            /* d1 = b1 * x[n] + a1 * y[n] + d2 */
            /* d2 = b2 * x[n] + a2 * y[n] */
            
            Xn0 = *pIn++ ;
            
            /* Calculation of acc0*/
            acc0 = b0*Xn0+vgetq_lane_f64(state, 0);
            
            
            /*
             *  final                  initial
             *  state   b0Coeffs        state  a0Coeffs
             *   |        |              |        |
             *   __       __             __       __
             *  /  \     /  \           /  \     /  \
             * | d1 | = | b1 | * Xn0 + | d2 | + | a1 | x acc0
             * | d2 |   | b2 |         | 0  |   | a2 |
             *  \__/     \__/           \__/     \__/
             */
            
            /* state -> initial state (see above) */
            
            state = vextq_f64(state, zeroV, 1);
            
            /* Calculation of final state */
            state = vfmaq_n_f64(state, b0Coeffs, Xn0);
            state = vfmaq_n_f64(state, a0Coeffs, acc0);
            
            *pOut++ = acc0 ;
            sample--;
        }
        /* Store the updated state variables back into the state array */
        pState[0] = vgetq_lane_f64(state, 0);
        pState[1] = vgetq_lane_f64(state, 1);
        
        pState += 2U;
        
        /* The current stage output is given as the input to the next stage */
        pIn = pDst;
        
        /* Reset the output working pointer */
        pOut = pDst;
        
        stage--;
        
    } while (stage > 0U);
    
}
#else

ARM_DSP_ATTRIBUTE void arm_biquad_cascade_df2T_f64(
    const arm_biquad_cascade_df2T_instance_f64 * S,
    const float64_t * pSrc,
    float64_t * pDst,
    uint32_t blockSize)
{
    
    const float64_t *pIn = pSrc;                   /* Source pointer */
    float64_t *pOut = pDst;                        /* Destination pointer */
    float64_t *pState = S->pState;                 /* State pointer */
    const float64_t *pCoeffs = S->pCoeffs;               /* Coefficient pointer */
    float64_t acc1;                                /* Accumulator */
    float64_t b0, b1, b2, a1, a2;                  /* Filter coefficients */
    float64_t Xn1;                                 /* Temporary input */
    float64_t d1, d2;                              /* State variables */
    uint32_t sample, stage = S->numStages;         /* Loop counters */
    
    
    do
    {
        /* Reading the coefficients */
        b0 = pCoeffs[0];
        b1 = pCoeffs[1];
        b2 = pCoeffs[2];
        a1 = pCoeffs[3];
        a2 = pCoeffs[4];
        
        /* Reading the state values */
        d1 = pState[0];
        d2 = pState[1];
        
        pCoeffs += 5U;
        
#if defined (ARM_MATH_LOOPUNROLL)
        
        /* Loop unrolling: Compute 16 outputs at a time */
        sample = blockSize >> 4U;
        
        while (sample > 0U) {
            
            /* y[n] = b0 * x[n] + d1 */
            /* d1 = b1 * x[n] + a1 * y[n] + d2 */
            /* d2 = b2 * x[n] + a2 * y[n] */
            
            /*  1 */
            Xn1 = *pIn++;
            
            acc1 = b0 * Xn1 + d1;
            
            d1 = b1 * Xn1 + d2;
            d1 += a1 * acc1;
            
            d2 = b2 * Xn1;
            d2 += a2 * acc1;
            
            *pOut++ = acc1;
            
            
            /*  2 */
            Xn1 = *pIn++;
            
            acc1 = b0 * Xn1 + d1;
            
            d1 = b1 * Xn1 + d2;
            d1 += a1 * acc1;
            
            d2 = b2 * Xn1;
            d2 += a2 * acc1;
            
            *pOut++ = acc1;
            
            /*  3 */
            Xn1 = *pIn++;
            
            acc1 = b0 * Xn1 + d1;
            
            d1 = b1 * Xn1 + d2;
            d1 += a1 * acc1;
            
            d2 = b2 * Xn1;
            d2 += a2 * acc1;
            
            *pOut++ = acc1;
            
            /*  4 */
            Xn1 = *pIn++;
            
            acc1 = b0 * Xn1 + d1;
            
            d1 = b1 * Xn1 + d2;
            d1 += a1 * acc1;
            
            d2 = b2 * Xn1;
            d2 += a2 * acc1;
            
            *pOut++ = acc1;
            
            /*  5 */
            Xn1 = *pIn++;
            
            acc1 = b0 * Xn1 + d1;
            
            d1 = b1 * Xn1 + d2;
            d1 += a1 * acc1;
            
            d2 = b2 * Xn1;
            d2 += a2 * acc1;
            
            *pOut++ = acc1;
            
            /*  6 */
            Xn1 = *pIn++;
            
            acc1 = b0 * Xn1 + d1;
            
            d1 = b1 * Xn1 + d2;
            d1 += a1 * acc1;
            
            d2 = b2 * Xn1;
            d2 += a2 * acc1;
            
            *pOut++ = acc1;
            
            /*  7 */
            Xn1 = *pIn++;
            
            acc1 = b0 * Xn1 + d1;
            
            d1 = b1 * Xn1 + d2;
            d1 += a1 * acc1;
            
            d2 = b2 * Xn1;
            d2 += a2 * acc1;
            
            *pOut++ = acc1;
            
            /*  8 */
            Xn1 = *pIn++;
            
            acc1 = b0 * Xn1 + d1;
            
            d1 = b1 * Xn1 + d2;
            d1 += a1 * acc1;
            
            d2 = b2 * Xn1;
            d2 += a2 * acc1;
            
            *pOut++ = acc1;
            
            /*  9 */
            Xn1 = *pIn++;
            
            acc1 = b0 * Xn1 + d1;
            
            d1 = b1 * Xn1 + d2;
            d1 += a1 * acc1;
            
            d2 = b2 * Xn1;
            d2 += a2 * acc1;
            
            *pOut++ = acc1;
            
            /* 10 */
            Xn1 = *pIn++;
            
            acc1 = b0 * Xn1 + d1;
            
            d1 = b1 * Xn1 + d2;
            d1 += a1 * acc1;
            
            d2 = b2 * Xn1;
            d2 += a2 * acc1;
            
            *pOut++ = acc1;
            
            /* 11 */
            Xn1 = *pIn++;
            
            acc1 = b0 * Xn1 + d1;
            
            d1 = b1 * Xn1 + d2;
            d1 += a1 * acc1;
            
            d2 = b2 * Xn1;
            d2 += a2 * acc1;
            
            *pOut++ = acc1;
            
            /* 12 */
            Xn1 = *pIn++;
            
            acc1 = b0 * Xn1 + d1;
            
            d1 = b1 * Xn1 + d2;
            d1 += a1 * acc1;
            
            d2 = b2 * Xn1;
            d2 += a2 * acc1;
            
            *pOut++ = acc1;
            
            /* 13 */
            Xn1 = *pIn++;
            
            acc1 = b0 * Xn1 + d1;
            
            d1 = b1 * Xn1 + d2;
            d1 += a1 * acc1;
            
            d2 = b2 * Xn1;
            d2 += a2 * acc1;
            
            *pOut++ = acc1;
            
            /* 14 */
            Xn1 = *pIn++;
            
            acc1 = b0 * Xn1 + d1;
            
            d1 = b1 * Xn1 + d2;
            d1 += a1 * acc1;
            
            d2 = b2 * Xn1;
            d2 += a2 * acc1;
            
            *pOut++ = acc1;
            
            /* 15 */
            Xn1 = *pIn++;
            
            acc1 = b0 * Xn1 + d1;
            
            d1 = b1 * Xn1 + d2;
            d1 += a1 * acc1;
            
            d2 = b2 * Xn1;
            d2 += a2 * acc1;
            
            *pOut++ = acc1;
            
            /* 16 */
            Xn1 = *pIn++;
            
            acc1 = b0 * Xn1 + d1;
            
            d1 = b1 * Xn1 + d2;
            d1 += a1 * acc1;
            
            d2 = b2 * Xn1;
            d2 += a2 * acc1;
            
            *pOut++ = acc1;
            
            /* decrement loop counter */
            sample--;
        }
        
        /* Loop unrolling: Compute remaining outputs */
        sample = blockSize & 0xFU;
        
#else
        
        /* Initialize blkCnt with number of samples */
        sample = blockSize;
        
#endif /* #if defined (ARM_MATH_LOOPUNROLL) */
        
        while (sample > 0U) {
            Xn1 = *pIn++;
            
            acc1 = b0 * Xn1 + d1;
            
            d1 = b1 * Xn1 + d2;
            d1 += a1 * acc1;
            
            d2 = b2 * Xn1;
            d2 += a2 * acc1;
            
            *pOut++ = acc1;
            
            /* decrement loop counter */
            sample--;
        }
        
        /* Store the updated state variables back into the state array */
        pState[0] = d1;
        pState[1] = d2;
        
        pState += 2U;
        
        /* The current stage output is given as the input to the next stage */
        pIn = pDst;
        
        /* Reset the output working pointer */
        pOut = pDst;
        
        /* decrement loop counter */
        stage--;
        
    } while (stage > 0U);
    
}
#endif



/**
 @} end of BiquadCascadeDF2T group
 */
