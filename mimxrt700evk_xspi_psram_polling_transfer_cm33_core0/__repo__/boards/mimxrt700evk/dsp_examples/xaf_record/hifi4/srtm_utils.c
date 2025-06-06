/*
 * Copyright 2019-2023 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "dsp_config.h"
#include "rpmsg_lite.h"
#include "srtm_utils.h"
#include "srtm_config.h"

#include "xaf-utils-test.h"

#include "fsl_gpio.h"
#include "fsl_sema42.h"

#define AUDIO_BUFFER_FILL_THRESHOLD (16 * 1024)

int g_execution_abort_flag = 0;
xf_thread_t g_disconnect_thread;
int g_num_comps_in_graph = 0;
xf_thread_t *g_comp_thread;

_XA_API_ERR_MAP error_map_table_api[XA_NUM_API_ERRS]=
{
    {(int)XAF_RTOS_ERR,       "rtos error"},
    {(int)XAF_INVALIDVAL_ERR,    "invalid value"},
    {(int)XAF_ROUTING_ERR,    "routing error"},
    {(int)XAF_INVALIDPTR_ERR,        "invalid pointer"},
    {(int)XAF_API_ERR,          "API error"},
    {(int)XAF_TIMEOUT_ERR,   "message queue Timeout"},
    {(int)XAF_MEMORY_ERR,   "memory error"},
};

/*******************************************************************************
 * Utility Functions
 ******************************************************************************/
void DSP_PRINTF(const char* ptr, ...)
{
    va_list ap;
    SEMA42_Lock(APP_SEMA42, SEMA_PRINTF_NUM, SEMA_CORE_ID_DSP);
    va_start(ap, ptr);
    DbgConsole_Vprintf(ptr, ap);
    va_end(ap);
    SEMA42_Unlock(APP_SEMA42, SEMA_PRINTF_NUM);
}

/* Wrap stdlib malloc() for audio framework allocator */
void *DSP_Malloc(int32_t size, int32_t id)
{
    /* If malloc can return un-aligned data, must return 4-byte aligned pointer for XAF. */
    return malloc(size);
}

/* Wrap stdlib free() for audio framework allocator */
void DSP_Free(void *ptr, int32_t id)
{
    free(ptr);
}

/* Read audio data for DSP processing
 *
 * param buffer Input buffer from application
 * param out Output buffer
 * param size Size of data to read in bytes
 * return number of bytes read */
uint32_t DSP_AudioRead(dsp_handle_t *dsp, char *data, uint32_t size)
{
    uint32_t read_size = size;

    if (size + dsp->buffer_in.index > dsp->buffer_in.size)
    {
        read_size = dsp->buffer_in.size - dsp->buffer_in.index;
    }

#ifdef FSL_FEATURE_SOC_XCACHE_COUNT
    xthal_dcache_region_invalidate(data, read_size);
#endif
    memcpy(data, &dsp->buffer_in.data[dsp->buffer_in.index], read_size);

    dsp->buffer_in.index += read_size;

    return read_size;
}

/* Consume audio data output from DSP processing
 *
 * param buffer Output buffer
 * param in Input buffer from DSP
 * param size Size of data to write in bytes
 * return number of bytes written */
uint32_t DSP_AudioWrite(dsp_handle_t *dsp, char *data, uint32_t size)
{
    uint32_t write_size = size;

    if (size + dsp->buffer_out.index > dsp->buffer_out.size)
    {
        write_size = dsp->buffer_out.size - dsp->buffer_out.index;
    }

    memcpy(&dsp->buffer_out.data[dsp->buffer_out.index], data, write_size);
#ifdef FSL_FEATURE_SOC_XCACHE_COUNT
    xthal_dcache_region_writeback(&dsp->buffer_out.data[dsp->buffer_out.index], write_size);
#endif

    dsp->buffer_out.index += write_size;

    return write_size;
}

/* Thread for processing DSP pipeline
 *
 * This thread will take care of polling the audio framework for status, feeding
 * data when needed, and consuming output when needed.  It will end when the
 * input or output data is exhausted.
 */
int DSP_ProcessThread(void *arg, int wake_value)
{
    dsp_handle_t *ctx = (dsp_handle_t *)arg;
    xaf_comp_status status;
    XAF_ERR_CODE ret;
    char *buffer;
    uint32_t buffer_len;
    uint32_t event_state;
    int info[4];
    int read_size, write_size;

    DSP_PRINTF("[DSP_ProcessThread] start\r\n");

    ret = xaf_comp_process(NULL, ctx->comp, NULL, 0, XAF_EXEC_FLAG);
    if (ret != XAF_NO_ERR)
    {
        DSP_PRINTF("[DSP_ProcessThread] xaf_comp_process XAF_EXEC_FLAG failure: %d\r\n", ret);
        return -1;
    }

    while (1)
    {
        /* Check for external events to the processing thread */
        xos_event_get(&ctx->pipeline_event, &event_state);
        if (event_state & DSP_EVENT_STOP)
        {
            xos_event_clear(&ctx->pipeline_event, DSP_EVENT_STOP);
            /* Send INPUT_OVER to decoder to gracefully shutdown pipeline */
            ret = xaf_comp_process(NULL, ctx->comp, NULL, 0, XAF_INPUT_OVER_FLAG);
            if (ret != XAF_NO_ERR)
            {
                DSP_PRINTF("[DSP_ProcessThread] xaf_comp_process XAF_INPUT_OVER_FLAG failure: %d\r\n", ret);
            }

            break;
        }

        ret = xaf_comp_get_status(NULL, ctx->comp, &status, &info[0]);
        if (ret != XAF_NO_ERR)
        {
            DSP_PRINTF("[DSP_ProcessThread] xaf_comp_get_status failure: %d\r\n", ret);
            DSP_PRINTF("[DSP_ProcessThread] Error, exiting\r\n");
            return -1;
        }

        if (status == XAF_EXEC_DONE)
        {
            DSP_PRINTF("[DSP_ProcessThread] Execution complete - exiting\r\n");
            break;
        }
        else if (status == XAF_NEED_INPUT)
        {
            /* Read input and feed data to pipeline for processing */
            buffer     = (char *)info[0];
            buffer_len = (uint32_t)info[1];

            read_size = ctx->audio_read(ctx, buffer, buffer_len);

            // DSP_PRINTF("[DSP_ProcessThread] Processing %d bytes\r\n", read_size);

            if (read_size > 0)
            {
                ret = xaf_comp_process(NULL, ctx->comp, (void *)buffer, read_size, XAF_INPUT_READY_FLAG);
                if (ret != XAF_NO_ERR)
                {
                    DSP_PRINTF("[DSP_ProcessThread] xaf_comp_process XAF_INPUT_READY_FLAG failure: %d\r\n", ret);
                    return -1;
                }
            }
            else
            {
                ret = xaf_comp_process(NULL, ctx->comp, NULL, 0, XAF_INPUT_OVER_FLAG);
                if (ret != XAF_NO_ERR)
                {
                    DSP_PRINTF("[DSP_ProcessThread] xaf_comp_process XAF_INPUT_OVER_FLAG failure: %d\r\n", ret);
                    return -1;
                }

                // DSP_PRINTF("[DSP_ProcessThread] input over\r\n");
            }
        }
        else if (status == XAF_OUTPUT_READY)
        {
            /* Consume output from pipeline */
            buffer     = (char *)info[0];
            buffer_len = (uint32_t)info[1];

            if (buffer_len == 0)
            {
                /* No output available */
            }
            else
            {
                write_size = ctx->audio_write(ctx, buffer, buffer_len);

                ret = xaf_comp_process(NULL, ctx->comp, buffer, buffer_len, XAF_NEED_OUTPUT_FLAG);
                if (ret != XAF_NO_ERR)
                {
                    DSP_PRINTF("[DSP_ProcessThread] xaf_comp_process XAF_NEED_OUTPUT_FLAG failure: %d\r\n", ret);
                    return -1;
                }
            }
        }
        else
        {
            /* Error or nonstandard response. */
            DSP_PRINTF("[DSP_ProcessThread] unexpected status: %d\r\n", status);
        }
    }

    DSP_PRINTF("[DSP_ProcessThread] exiting\r\n");

    return 0;
}

int abort_blocked_threads()
{
    int i;

    /*...set global abort flag immediately */
    g_execution_abort_flag = 1;

    /* Ignore if not enabled in the testbench */
    if ( g_num_comps_in_graph == 0 )
        return -1;

    for( i=0; i<g_num_comps_in_graph; i++ )
    {
        if ( __xf_thread_get_state(&g_comp_thread[i]) == XF_THREAD_STATE_BLOCKED )
        {
            fprintf(stderr, "Aborting thread: %d\n", i);
            __xf_thread_cancel( (xf_thread_t *) &g_comp_thread[i] );
        }
    }

    if ( __xf_thread_get_state(&g_disconnect_thread) == XF_THREAD_STATE_BLOCKED )
    {
        fprintf(stderr, "Aborting disconnect thread\n");
        __xf_thread_cancel( &g_disconnect_thread );
    }

    return 0;
}
