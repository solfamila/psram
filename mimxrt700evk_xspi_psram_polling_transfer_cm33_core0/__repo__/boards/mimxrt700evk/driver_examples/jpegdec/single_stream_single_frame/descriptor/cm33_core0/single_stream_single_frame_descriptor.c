/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include "fsl_common.h"
#include "app.h"
#include "fsl_debug_console.h"
#include "fsl_cache.h"
#include "fsl_jpegdec.h"
#include "jpeg.h"
#include "display_support.h"
#include "board.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define APP_FB_BPP    2
#define APP_FB_FORMAT kVIDEO_PixelFormatRGB565

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
static void DEMO_BufferSwitchOffCallback(void *param, void *switchOffBuffer);

/*******************************************************************************
 * Variables
 ******************************************************************************/
static dc_fb_info_t fbInfo;
static volatile bool s_newFrameShown = false;
static uint32_t s_decodedAddr[2]     = {DEMO_BUFFER0_ADDR, DEMO_BUFFER1_ADDR};
static uint32_t s_frameBufferAddr    = DEMO_FB_ADDR;
AT_NONCACHEABLE_SECTION(static jpegdec_descpt_t s_decoderDespt);

/*******************************************************************************
 * Code
 ******************************************************************************/
static uint8_t BYTECLIP(int val)
{
    if (val < 0)
    {
        return 0U;
    }
    else if (val > 255)
    {
        return 255U;
    }
    else
    {
        return (uint8_t)val;
    }
}

void Convert_yuv420_to_rgb565(uint16_t width, uint16_t height, uint32_t yAddr, uint32_t uvAddr, uint32_t rgbAddr)
{
    uint16_t *rgb = (void *)rgbAddr;
    int16_t r, g, b;
    uint8_t R, G, B, y, u, v;

    uint16_t i, j;

    for (i = 0U; i < height; i++)
    {
        for (j = 0U; j < width; j++)
        {
            y = *(((uint8_t *)yAddr) + width * i + j);
            u = *(((uint8_t *)uvAddr) + width * (i / 2) + j - (j % 2));
            v = *(((uint8_t *)uvAddr) + width * (i / 2) + j + 1 - (j % 2));
            r = y + 1402 * (v - 128) / 1000;
            g = y - (344 * (u - 128) + 714 * (v - 128)) / 1000;
            b = y + 1772 * (u - 128) / 1000;
            R = BYTECLIP(r);
            G = BYTECLIP(g);
            B = BYTECLIP(b);
            *rgb++ =
                (uint16_t)((((uint16_t)R & 0xF8) << 8U) | (((uint16_t)G & 0xFC) << 3U) | (((uint16_t)B & 0xF8) >> 3U));
        }
    }
}

void DEMO_Decode_JPEG(void)
{
    jpegdec_config_t config;
    uint32_t status;

    memset(&s_decoderDespt, 0U, sizeof(jpegdec_descpt_t));

    /* Step 1: Init JPEG decoder module. */
    JPEGDEC_GetDefaultConfig(&config);

    config.slots = kJPEGDEC_Slot0; /* Enable only one slot. */

    JPEGDEC_Init(APP_JPEGDEC, &config);

    /* Step 2: Set source buffer, buffer size. */
    JPEGDEC_SetJpegBuffer(&s_decoderDespt.config, (uint8_t *)jpegImg, jpegImgLen);

    /* Step 3: Set buffer of generated image for JPEG decoder. */
    JPEGDEC_SetOutputBuffer(&s_decoderDespt.config, (uint8_t *)s_decodedAddr[0], (uint8_t *)s_decodedAddr[1]);

    /* Step 4: Parse header. */
    JPEGDEC_ParseHeader(&s_decoderDespt.config);

    /* Step 5: Set output pitch, auto start decode when descriptor is loaded. */
    JPEGDEC_SetDecodeOption(&s_decoderDespt.config, s_decoderDespt.config.width, false, true);

    /* Step 6: Set slot descriptor. */
    JPEGDEC_SetSlotNextDescpt(APP_JPEGDEC, 0U, &s_decoderDespt);

    /* Step 7: Enable the descriptor to start the decoding. */
    JPEGDEC_EnableSlotNextDescpt(APP_JPEGDEC, 0U);

    /* Step 8: Wait for decoding complete. */
    status = JPEGDEC_GetStatusFlags(APP_JPEGDEC, 0U);

    while ((status & (kJPEGDEC_DecodeCompleteFlag | kJPEGDEC_ErrorFlags)) == 0U)
    {
        status = JPEGDEC_GetStatusFlags(APP_JPEGDEC, 0U);
    }

    if ((status & kJPEGDEC_DecodeCompleteFlag) == 0U)
    {
        JPEGDEC_ClearStatusFlags(APP_JPEGDEC, 0U, status);
        PRINTF("Error occured during JPEG decoding\r\n");
        assert(false);
    }

    /* Step 9: Convert the YUV420 format pixel to RGB565 to display. */
    Convert_yuv420_to_rgb565(s_decoderDespt.config.width, s_decoderDespt.config.height, s_decodedAddr[0],
                             s_decodedAddr[1], s_frameBufferAddr);

    /* Step 10: Configure display layer configuration. */
    fbInfo.pixelFormat = APP_FB_FORMAT;
    fbInfo.width       = s_decoderDespt.config.width;
    fbInfo.height      = s_decoderDespt.config.height;
    fbInfo.startX      = (DEMO_PANEL_WIDTH - s_decoderDespt.config.width) / 2U;
    fbInfo.startY      = (DEMO_PANEL_HEIGHT - s_decoderDespt.config.height) / 2U;
    fbInfo.strideBytes = s_decoderDespt.config.width * APP_FB_BPP;
    if (kStatus_Success != g_dc.ops->setLayerConfig(&g_dc, 0, &fbInfo))
    {
        PRINTF("Error: Could not configure the display controller\r\n");
        assert(false);
    }

    g_dc.ops->setFrameBuffer(&g_dc, 0, (void *)s_frameBufferAddr);

    if ((g_dc.ops->getProperty(&g_dc) & kDC_FB_ReserveFrameBuffer) == 0)
    {
        while (s_newFrameShown == false)
        {
        }
    }

    s_newFrameShown = true;
    
    /* Step 11: Enable layer and display the decoded image. */
    g_dc.ops->enableLayer(&g_dc, 0);
}

void DEMO_InitDisplay(void)
{
    status_t status;

    BOARD_PrepareDisplayController();

    memset((void *)s_frameBufferAddr, 0, DEMO_PANEL_HEIGHT * DEMO_PANEL_WIDTH * APP_FB_BPP);

    status = g_dc.ops->init(&g_dc);
    if (kStatus_Success != status)
    {
        PRINTF("Display initialization failed\r\n");
        assert(false);
    }

    g_dc.ops->getLayerDefaultConfig(&g_dc, 0, &fbInfo);
    g_dc.ops->setCallback(&g_dc, 0, DEMO_BufferSwitchOffCallback, NULL);
}

/*!
 * @brief Main function
 */
int main(void)
{
    BOARD_InitHardware();

    PRINTF("JPEG decoder demo start:\r\n");
    PRINTF("One frame of JPEG picture will be decoded by slot 0\r\n");
    PRINTF(
        "First we configure the descriptor then enable the descriptor to let it update the decoder configuration\r\n");

    DEMO_InitDisplay();

    PRINTF("Decoding the image...\r\n");
    DEMO_Decode_JPEG();
    PRINTF("done!\r\n");

    while (1)
    {
    }
}

static void DEMO_BufferSwitchOffCallback(void *param, void *switchOffBuffer)
{
    s_newFrameShown = true;
}