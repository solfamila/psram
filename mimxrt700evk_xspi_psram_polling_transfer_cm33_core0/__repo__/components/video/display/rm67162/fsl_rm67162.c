/*
 * Copyright 2019-2020 NXP
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_display.h"
#include "fsl_rm67162.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define RM67162_DelayMs VIDEO_DelayMs

/*******************************************************************************
 * Variables
 ******************************************************************************/
static const uint8_t rm67162InitSetting_400x400[][2] = {
    /* Page 3:GOA */
    {0xFE, 0x04},
    /* GOA SETTING */
    {0x00, 0xDC},
    {0x01, 0x00},
    {0x02, 0x02},
    {0x03, 0x00},
    {0x04, 0x00},
    {0x05, 0x03},
    {0x06, 0x16},
    {0x07, 0x13},
    {0x08, 0x08},
    {0x09, 0xDC},
    {0x0A, 0x00},
    {0x0B, 0x02},
    {0x0C, 0x00},
    {0x0D, 0x00},
    {0x0E, 0x02},
    {0x0F, 0x16},
    {0x10, 0x18},
    {0x11, 0x08},
    {0x12, 0x92},
    {0x13, 0x00},
    {0x14, 0x02},
    {0x15, 0x05},
    {0x16, 0x40},
    {0x17, 0x03},
    {0x18, 0x16},
    {0x19, 0xD7},
    {0x1A, 0x01},
    {0x1B, 0xDC},
    {0x1C, 0x00},
    {0x1D, 0x04},
    {0x1E, 0x00},
    {0x1F, 0x00},
    {0x20, 0x03},
    {0x21, 0x16},
    {0x22, 0x18},
    {0x23, 0x08},
    {0x24, 0xDC},
    {0x25, 0x00},
    {0x26, 0x04},
    {0x27, 0x00},
    {0x28, 0x00},
    {0x29, 0x01},
    {0x2A, 0x16},
    {0x2B, 0x18},
    {0x2D, 0x08},
    {0x4C, 0x99},
    {0x4D, 0x00},
    {0x4E, 0x00},
    {0x4F, 0x00},
    {0x50, 0x01},
    {0x51, 0x0A},
    {0x52, 0x00},
    {0x5A, 0xE4},
    {0x5E, 0x77},
    {0x5F, 0x77},
    {0x60, 0x34},
    {0x61, 0x02},
    {0x62, 0x81},

    /* Page 6 */
    {0xFE, 0x07},
    {0x07, 0x4F},

    /* Page 0 */
    {0xFE, 0x01},
    /* Display Resolution Panel Option */
    {0x05, 0x15},
    /* DDVDH Charge Pump Control Normal Mode */
    {0x0E, 0x8B},
    /* DDVDH Charge Pump Control ldle Mode */
    {0x0F, 0x8B},
    /* DDVDH/VCL Regulator Enable */
    {0x10, 0x11},
    /* VCL Charge Pump Control Normal Mode */
    {0x11, 0xA2},
    /* VCL Charge Pump Control Idle Mode */
    {0x12, 0xA0},
    /* VGH Charge Pump Control ldle Mode */
    {0x14, 0xA1},
    /* VGL Charge Pump Control Normal Mode */
    {0x15, 0x82},
    /* VGHR Control */
    {0x18, 0x47},
    /* VGLR Control */
    {0x19, 0x36},
    /* VREFPN5 REGULATOR ENABLE */
    {0x1A, 0x10},
    /* VREFPN5 */
    {0x1C, 0x57},
    /* SWITCH EQ Control */
    {0x1D, 0x02},
    /* VGMP Control */
    {0x21, 0xF8},
    /* VGSP Control */
    {0x22, 0x90},
    /* VGMP / VGSP control */
    {0x23, 0x00},
    /* Low Frame Rate Control Normal Mode */
    {0x25, 0x03},
    {0x26, 0x4a},
    /* Low Frame Rate Control Idle Mode */
    {0x2A, 0x03},
    {0x2B, 0x4A},
    {0x2D, 0x12},
    {0x2F, 0x12},

    {0x30, 0x45},

    /* Source Control */
    {0x37, 0x0C},
    /* Switch Timing Control */
    {0x3A, 0x00},
    {0x3B, 0x20},
    {0x3D, 0x0B},
    {0x3F, 0x38},
    {0x40, 0x0B},
    {0x41, 0x0B},

    /* Switch Output Selection */
    {0x42, 0x33},
    {0x43, 0x66},
    {0x44, 0x11},
    {0x45, 0x44},
    {0x46, 0x22},
    {0x47, 0x55},
    {0x4C, 0x33},
    {0x4D, 0x66},
    {0x4E, 0x11},
    {0x4f, 0x44},
    {0x50, 0x22},
    {0x51, 0x55},

    /* Source Data Output Selection */
    {0x56, 0x11},
    {0x58, 0x44},
    {0x59, 0x22},
    {0x5A, 0x55},
    {0x5B, 0x33},
    {0x5C, 0x66},
    {0x61, 0x11},
    {0x62, 0x44},
    {0x63, 0x22},
    {0x64, 0x55},
    {0x65, 0x33},
    {0x66, 0x66},

    {0x6D, 0x90},
    {0x6E, 0x40},

    /* Source Sequence 2 */
    {0x70, 0xA5},

    /* OVDD control */
    {0x72, 0x04},

    /* OVSS control */
    {0x73, 0x15},

    /* Page 9 */
    {0xFE, 0x0A},
    {0x29, 0x10},

    /* Page 4 */
    {0xFE, 0x05},
    /* ELVSS -2.4V(RT4723). 0x15: RT4723. 0x01: RT4723B. 0x17: STAM1332. */
    {0x05, 0x15},

    {0xFE, 0x00},
    /* enable TE. */
    {0x35, 0x00},
};

static const uint8_t rm67162InitSetting_400x392[][2] = {
    {0xFE, 0x01},
    {0x06, 0x62},
    {0x0E, 0x80},
    {0x0F, 0x80},
    {0x10, 0x71},
    {0x13, 0x81},
    {0x14, 0x81},
    {0x15, 0x82},
    {0x16, 0x82},
    {0x18, 0x88},
    {0x19, 0x55},
    {0x1A, 0x10},
    {0x1C, 0x99},
    {0x1D, 0x03},
    {0x1E, 0x03},
    {0x1F, 0x03},
    {0x20, 0x03},
    {0x25, 0x03},
    {0x26, 0x8D},
    {0x2A, 0x03},
    {0x2B, 0x8D},
    {0x36, 0x00},
    {0x37, 0x10},
    {0x3A, 0x00},
    {0x3B, 0x00},
    {0x3D, 0x20},
    {0x3F, 0x3A},
    {0x40, 0x30},
    {0x41, 0x30},
    {0x42, 0x33},
    {0x43, 0x22},
    {0x44, 0x11},
    {0x45, 0x66},
    {0x46, 0x55},
    {0x47, 0x44},
    {0x4C, 0x33},
    {0x4D, 0x22},
    {0x4E, 0x11},
    {0x4F, 0x66},
    {0x50, 0x55},
    {0x51, 0x44},
    {0x57, 0xB3},
    {0x6B, 0x19},
    {0x70, 0x55},
    {0x74, 0x0C},

    /* VGMP/VGSP Voltage Control */
    {0xFE, 0x02},
    {0x9B, 0x40},
    {0x9C, 0x67},
    {0x9D, 0x20},

    /* VGMP/VGSP Voltage Control */
    {0xFE, 0x03},
    {0x9B, 0x40},
    {0x9C, 0x67},
    {0x9D, 0x20},

    /* VSR Command */
    {0xFE, 0x04},
    {0x5D, 0x10},

    /* VSR1 Timing Set */
    {0xFE, 0x04},
    {0x00, 0x8D},
    {0x01, 0x00},
    {0x02, 0x01},
    {0x03, 0x01},
    {0x04, 0x10},
    {0x05, 0x01},
    {0x06, 0xA7},
    {0x07, 0x20},
    {0x08, 0x00},

    /* VSR2 Timing Set */
    {0xFE, 0x04},
    {0x09, 0xC2},
    {0x0A, 0x00},
    {0x0B, 0x02},
    {0x0C, 0x01},
    {0x0D, 0x40},
    {0x0E, 0x06},
    {0x0F, 0x01},
    {0x10, 0xA7},
    {0x11, 0x00},

    /* VSR3 Timing Set */
    {0xFE, 0x04},
    {0x12, 0xC2},
    {0x13, 0x00},
    {0x14, 0x02},
    {0x15, 0x01},
    {0x16, 0x40},
    {0x17, 0x07},
    {0x18, 0x01},
    {0x19, 0xA7},
    {0x1A, 0x00},

    /* VSR4 Timing Set */
    {0xFE, 0x04},
    {0x1B, 0x82},
    {0x1C, 0x00},
    {0x1D, 0xFF},
    {0x1E, 0x05},
    {0x1F, 0x60},
    {0x20, 0x02},
    {0x21, 0x01},
    {0x22, 0x7C},
    {0x23, 0x00},

    /* VSR5 Timing Set */
    {0xFE, 0x04},
    {0x24, 0xC2},
    {0x25, 0x00},
    {0x26, 0x04},
    {0x27, 0x02},
    {0x28, 0x70},
    {0x29, 0x05},
    {0x2A, 0x74},
    {0x2B, 0x8D},
    {0x2D, 0x00},

    /* VSR6 Timing Set */
    {0xFE, 0x04},
    {0x2F, 0xC2},
    {0x30, 0x00},
    {0x31, 0x04},
    {0x32, 0x02},
    {0x33, 0x70},
    {0x34, 0x07},
    {0x35, 0x74},
    {0x36, 0x8D},
    {0x37, 0x00},

    /* VSR Marping command  */
    {0xFE, 0x04},
    {0x5E, 0x20},
    {0x5F, 0x31},
    {0x60, 0x54},
    {0x61, 0x76},
    {0x62, 0x98},

    /* ELVSS -2.4V(RT4723). 0x15: RT4723. 0x01: RT4723B. 0x17: STAM1332. */
    {0xFE, 0x05},
    {0x05, 0x15},
    {0x2A, 0x04},
    {0x91, 0x00},

    {0xFE, 0x00},
    {0x35, 0x00}, /* TE enable. */
};

const display_operations_t rm67162_ops = {
    .init   = RM67162_Init,
    .deinit = RM67162_Deinit,
    .start  = RM67162_Start,
    .stop   = RM67162_Stop,
};

/*******************************************************************************
 * Code
 ******************************************************************************/

status_t RM67162_Init(display_handle_t *handle, const display_config_t *config)
{
    uint32_t i;
    status_t status = kStatus_Success;
    mipi_dsc_pixel_format_t dscPixelFormat;
    const rm67162_resource_t *resource = (const rm67162_resource_t *)(handle->resource);
    mipi_dsi_device_t *dsiDevice       = resource->dsiDevice;
    uint32_t initSettingSize;
    const uint8_t(*initSetting)[2];

    if (config->resolution == FSL_VIDEO_RESOLUTION(400, 400))
    {
        initSetting     = rm67162InitSetting_400x400;
        initSettingSize = ARRAY_SIZE(rm67162InitSetting_400x400);
    }
    else if (config->resolution == FSL_VIDEO_RESOLUTION(400, 392))
    {
        initSetting     = rm67162InitSetting_400x392;
        initSettingSize = ARRAY_SIZE(rm67162InitSetting_400x392);
    }
    else
    {
        return kStatus_InvalidArgument;
    }

    handle->height = FSL_VIDEO_EXTRACT_HEIGHT(config->resolution);
    handle->width  = FSL_VIDEO_EXTRACT_WIDTH(config->resolution);

    /* Only support RGB888 and RGB565. */
    if (kVIDEO_PixelFormatRGB565 == config->pixelFormat)
    {
        dscPixelFormat = kMIPI_DCS_Pixel16Bits;
    }
    else if ((kVIDEO_PixelFormatXRGB8888 == config->pixelFormat) || (kVIDEO_PixelFormatRGB888 == config->pixelFormat))
    {
        dscPixelFormat = kMIPI_DCS_Pixel24Bits;
    }
    else
    {
        return kStatus_InvalidArgument;
    }

    handle->pixelFormat = config->pixelFormat;

    /* Power on. */
    resource->pullPowerPin(true);
    RM67162_DelayMs(1);

    /* Perform reset. */
    resource->pullResetPin(false);
    RM67162_DelayMs(1);
    resource->pullResetPin(true);
    RM67162_DelayMs(150);

    /* Set the init settings. */
    for (i = 0; i < initSettingSize; i++)
    {
        status = MIPI_DSI_GenericWrite(dsiDevice, initSetting[i], 2);

        if (kStatus_Success != status)
        {
            return status;
        }
    }
    status = MIPI_DSI_DCS_SetPixelFormat(dsiDevice, dscPixelFormat, kMIPI_DCS_Pixel24Bits);
    if (kStatus_Success != status)
    {
        return status;
    }

    RM67162_DelayMs(50);

    /* Sleep out. */
    status = MIPI_DSI_DCS_EnterSleepMode(dsiDevice, false);
    if (kStatus_Success != status)
    {
        return status;
    }

    RM67162_DelayMs(150);

    return MIPI_DSI_DCS_SetDisplayOn(dsiDevice, true);
}

status_t RM67162_Deinit(display_handle_t *handle)
{
    const rm67162_resource_t *resource = (const rm67162_resource_t *)(handle->resource);
    mipi_dsi_device_t *dsiDevice       = resource->dsiDevice;

    (void)MIPI_DSI_DCS_EnterSleepMode(dsiDevice, true);

    resource->pullResetPin(false);
    resource->pullPowerPin(false);

    return kStatus_Success;
}

status_t RM67162_Start(display_handle_t *handle)
{
    const rm67162_resource_t *resource = (const rm67162_resource_t *)(handle->resource);
    mipi_dsi_device_t *dsiDevice       = resource->dsiDevice;

    return MIPI_DSI_DCS_SetDisplayOn(dsiDevice, true);
}

status_t RM67162_Stop(display_handle_t *handle)
{
    const rm67162_resource_t *resource = (const rm67162_resource_t *)(handle->resource);
    mipi_dsi_device_t *dsiDevice       = resource->dsiDevice;

    return MIPI_DSI_DCS_SetDisplayOn(dsiDevice, false);
}
