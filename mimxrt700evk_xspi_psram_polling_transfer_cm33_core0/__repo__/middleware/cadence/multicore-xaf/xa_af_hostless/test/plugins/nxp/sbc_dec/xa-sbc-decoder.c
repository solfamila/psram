/*******************************************************************************
* Copyright (c) 2015-2019 Cadence Design Systems, Inc.
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to use this Software with Cadence processor cores only and
* not with any other processors and platforms, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************************/
/*
 * Copyright 2019-2022 NXP.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/*******************************************************************************
 * xa-sbc-decoder.c
 *
 * SBC decoder plugin - thin wrapper around SBCDEC library
 ******************************************************************************/

#define MODULE_TAG                      SBCDEC

/*******************************************************************************
 * Includes
 ******************************************************************************/

/* Enable just aac and mp3 / opus and vorbis to save memory*/
#if XA_SBC_DECODER

#include "xa_sbc_dec_api.h"
#include "audio/xa-audio-decoder-api.h"


#ifdef XAF_PROFILE
#include "xaf-clk-test.h"
extern clk_t dec_cycles;
#endif

/*******************************************************************************
 * Override GET-CONFIG-PARAM function
 ******************************************************************************/

static inline XA_ERRORCODE xa_sbc_get_config_param(xa_codec_handle_t handle, WORD32 i_idx, pVOID pv_value)
{
    /* ...translate "standard" parameter index into internal value */
    switch (i_idx)
    {
    case XA_CODEC_CONFIG_PARAM_CHANNELS:
        /* ...return number of output channels */
        i_idx = XA_SBC_DEC_CONFIG_PARAM_NUM_CHANNELS;
        break;

    case XA_CODEC_CONFIG_PARAM_SAMPLE_RATE:
        /* ...return output sampling frequency */
        i_idx = XA_SBC_DEC_CONFIG_PARAM_SAMP_FREQ;
        break;

    case XA_CODEC_CONFIG_PARAM_PCM_WIDTH:
        /* ...return sample bit-width */
        i_idx = XA_SBC_DEC_CONFIG_PARAM_PCM_WDSZ;
        break;
    }

    /* ...pass to library */
    return xa_sbc_dec(handle, XA_API_CMD_GET_CONFIG_PARAM, i_idx, pv_value);
}

/*******************************************************************************
 * API entry point
 ******************************************************************************/

XA_ERRORCODE xa_sbc_decoder(xa_codec_handle_t p_xa_module_obj, WORD32 i_cmd, WORD32 i_idx, pVOID pv_value)
{
    XA_ERRORCODE ret;
#ifdef XAF_PROFILE
    clk_t comp_start, comp_stop;
#endif

    /* ...process common audio-decoder commands */
    if (i_cmd == XA_API_CMD_GET_CONFIG_PARAM)
    {
        return xa_sbc_get_config_param(p_xa_module_obj, i_idx, pv_value);
    }
    else if (i_cmd == XA_API_CMD_SET_CONFIG_PARAM)
    {
        /* Workaround: SBC decoder has not config params to set,
         * but XAF requires set config to be called in order to get through
         * XA_API_CMD_INIT / XA_CMD_TYPE_INIT_API_POST_CONFIG_PARAMS phase */
    	return XA_NO_ERROR;
    }
    else
    {
#ifdef XAF_PROFILE
        comp_start = clk_read_start(CLK_SELN_THREAD);
#endif
        ret = xa_sbc_dec(p_xa_module_obj, i_cmd, i_idx, pv_value);

#ifdef XAF_PROFILE
        comp_stop = clk_read_stop(CLK_SELN_THREAD);
        dec_cycles += clk_diff(comp_stop, comp_start);
#endif
        return ret;
    }
}

#endif
