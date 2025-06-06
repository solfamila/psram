/*
 * SPDX-FileCopyrightText: Copyright 2010-2020, 2022, 2024 Arm Limited and/or its affiliates
 * <open-source-office@arm.com>
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

/* ----------------------------------------------------------------------
 * Project:      CMSIS NN Library
 * Title:        arm_nn_depthwise_conv_nt_t_s4.c
 * Description:  Depthwise convolution on matrices with no padding and packed int4 weights.
 *
 * $Date:        05 April 2024
 * $Revision:    V.1.0.0
 *
 * Target Processor:  Cortex-M processors with MVE extension.
 * -------------------------------------------------------------------- */

#include "arm_nnsupportfunctions.h"

/**
 * @ingroup groupSupport
 */

/**
 * @addtogroup supportConvolution
 * @{
 */

/*
 * Depthwise convolution of rhs matrix with 4 lhs matrices with no padding and packed int4 weights.
 * Dimensions are the same for lhs and rhs.
 *
 * Refer header file for details.
 *
 */
arm_cmsis_nn_status arm_nn_depthwise_conv_nt_t_s4(const int8_t *lhs,
                                                  const int8_t *rhs,
                                                  const int32_t input_offset,
                                                  const int32_t active_ch,
                                                  const int32_t total_ch,
                                                  const int32_t *out_shift,
                                                  const int32_t *out_mult,
                                                  const int32_t out_offset,
                                                  const int32_t activation_min,
                                                  const int32_t activation_max,
                                                  const uint16_t row_x_col,
                                                  const int32_t *const output_bias,
                                                  int8_t *out)
{
#if defined(ARM_MATH_MVEI)
    const int32_t *bias = output_bias;
    int32_t loop_count = (active_ch + 3) / 4;
    uint32_t num_ch_to_process = active_ch;
    const uint32x4_t gather_offset = {0, 0, 1, 1};
    const mve_pred16_t lower_nibble_mask = 3855; // 0000111100001111

    for (int i_loop_cnt = 0, offset = 0; i_loop_cnt < loop_count;
         num_ch_to_process -= 4, offset += 4, out += 4, i_loop_cnt++)
    {
        int32x4_t out_0 = vdupq_n_s32(0);
        if (bias)
        {
            out_0 = vldrwq_s32(bias);
            bias += 4;
        }
        int32x4_t out_1 = out_0;
        int32x4_t out_2 = out_0;
        int32x4_t out_3 = out_0;

        const int8_t *rhs_0 = rhs + (offset >> 1);
        const int8_t *lhs_0 = lhs + offset;
        const int8_t *lhs_1 = lhs + row_x_col * S4_CH_IN_BLOCK_MVE + offset;
        const int8_t *lhs_2 = lhs + (row_x_col * S4_CH_IN_BLOCK_MVE * 2) + offset;
        const int8_t *lhs_3 = lhs + (row_x_col * S4_CH_IN_BLOCK_MVE * 3) + offset;
        int32x4_t ker_sum = vdupq_n_s32(0);

        if (total_ch % 2)
        {
            int get_low_nibble = 1;
            for (int i_row_x_col = 0; i_row_x_col < row_x_col; i_row_x_col++)
            {
                int32x4_t ker_0;
                if (get_low_nibble)
                {
                    ker_0 = vldrbq_gather_offset_s32(rhs_0, gather_offset);

                    ker_0 = vrshlq_m_n_s32(ker_0, 28, lower_nibble_mask);
                    ker_0 = vshrq_m_n_s32(ker_0, ker_0, 24, lower_nibble_mask);

                    ker_0 = vshrq_n_s32(ker_0, 4);
                }
                else
                {
                    int8_t temp[] = {
                        rhs_0[0] >> 4, (int8_t)(rhs_0[1] << 4) >> 4, rhs_0[1] >> 4, (int8_t)(rhs_0[2] << 4) >> 4};
                    ker_0 = vldrbq_s32(temp);
                }

                ker_sum = vaddq_s32(ker_sum, ker_0);

                int32x4_t ip_0 = vldrbq_s32(lhs_0);
                out_0 += vmulq_s32(ip_0, ker_0);

                int32x4_t ip_1 = vldrbq_s32(lhs_1);
                out_1 += vmulq_s32(ip_1, ker_0);

                int32x4_t ip_2 = vldrbq_s32(lhs_2);
                out_2 += vmulq_s32(ip_2, ker_0);

                int32x4_t ip_3 = vldrbq_s32(lhs_3);
                out_3 += vmulq_s32(ip_3, ker_0);

                lhs_0 += S4_CH_IN_BLOCK_MVE;
                lhs_1 += S4_CH_IN_BLOCK_MVE;
                lhs_2 += S4_CH_IN_BLOCK_MVE;
                lhs_3 += S4_CH_IN_BLOCK_MVE;

                get_low_nibble = !get_low_nibble;
                rhs_0 += (total_ch >> 1) + get_low_nibble;
            }
        }
        else
        {
            for (int i_row_x_col = 0; i_row_x_col < row_x_col; i_row_x_col++)
            {
                int32x4_t ker_0 = vldrbq_gather_offset_s32(rhs_0, gather_offset);

                ker_0 = vrshlq_m_n_s32(ker_0, 28, lower_nibble_mask);
                ker_0 = vshrq_m_n_s32(ker_0, ker_0, 24, lower_nibble_mask);

                ker_0 = vshrq_n_s32(ker_0, 4);

                ker_sum = vaddq_s32(ker_sum, ker_0);

                int32x4_t ip_0 = vldrbq_s32(lhs_0);
                out_0 += vmulq_s32(ip_0, ker_0);

                int32x4_t ip_1 = vldrbq_s32(lhs_1);
                out_1 += vmulq_s32(ip_1, ker_0);

                int32x4_t ip_2 = vldrbq_s32(lhs_2);
                out_2 += vmulq_s32(ip_2, ker_0);

                int32x4_t ip_3 = vldrbq_s32(lhs_3);
                out_3 += vmulq_s32(ip_3, ker_0);

                lhs_0 += S4_CH_IN_BLOCK_MVE;
                lhs_1 += S4_CH_IN_BLOCK_MVE;
                lhs_2 += S4_CH_IN_BLOCK_MVE;
                lhs_3 += S4_CH_IN_BLOCK_MVE;

                rhs_0 += total_ch >> 1;
            }
        }

        ker_sum = vmulq_n_s32(ker_sum, input_offset);
        out_0 = ker_sum + out_0;
        out_1 = ker_sum + out_1;
        out_2 = ker_sum + out_2;
        out_3 = ker_sum + out_3;

        const int32x4_t mult = vldrwq_s32(out_mult);
        const int32x4_t shift = vldrwq_s32(out_shift);
        out_mult += 4;
        out_shift += 4;
        mve_pred16_t p = vctp32q(num_ch_to_process);

        out_0 = arm_requantize_mve_32x4(out_0, mult, shift);
        out_0 = vaddq_n_s32(out_0, out_offset);
        out_0 = vmaxq_s32(out_0, vdupq_n_s32(activation_min));
        out_0 = vminq_s32(out_0, vdupq_n_s32(activation_max));
        vstrbq_p_s32(out, out_0, p);

        out_1 = arm_requantize_mve_32x4(out_1, mult, shift);
        out_1 = vaddq_n_s32(out_1, out_offset);
        out_1 = vmaxq_s32(out_1, vdupq_n_s32(activation_min));
        out_1 = vminq_s32(out_1, vdupq_n_s32(activation_max));
        vstrbq_p_s32(out + total_ch, out_1, p);

        out_2 = arm_requantize_mve_32x4(out_2, mult, shift);
        out_2 = vaddq_n_s32(out_2, out_offset);
        out_2 = vmaxq_s32(out_2, vdupq_n_s32(activation_min));
        out_2 = vminq_s32(out_2, vdupq_n_s32(activation_max));
        vstrbq_p_s32(out + 2 * total_ch, out_2, p);

        out_3 = arm_requantize_mve_32x4(out_3, mult, shift);
        out_3 = vaddq_n_s32(out_3, out_offset);
        out_3 = vmaxq_s32(out_3, vdupq_n_s32(activation_min));
        out_3 = vminq_s32(out_3, vdupq_n_s32(activation_max));
        vstrbq_p_s32(out + 3 * total_ch, out_3, p);
    }

    return ARM_CMSIS_NN_SUCCESS;
#else
    (void)lhs;
    (void)rhs;
    (void)input_offset;
    (void)active_ch;
    (void)total_ch;
    (void)out_shift;
    (void)out_mult;
    (void)out_offset;
    (void)activation_min;
    (void)activation_max;
    (void)row_x_col;
    (void)output_bias;
    (void)out;
    return ARM_CMSIS_NN_NO_IMPL_ERROR;
#endif
}

/**
 * @} end of Doxygen group
 */
