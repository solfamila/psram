/*
 * Copyright 2023,2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _DISPLAY_SUPPORT_H_
#define _DISPLAY_SUPPORT_H_

#include "fsl_dc_fb.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define DEMO_PANEL_TFT_PROTO_5 4 /* MikroE TFT Proto 5" CAPACITIVE FlexIO/LCD DBI Display */
#define DEMO_PANEL_RK055AHD091 0 /* NXP "RK055HDMIPI4M" MIPI Rectangular Display */
#define DEMO_PANEL_RK055IQH091 1 /* NXP RESERVED */
#define DEMO_PANEL_RM67162     3 /* NXP "G1120B0MIPI" MIPI Circular Display */
#define DEMO_PANEL_RK055MHD091 2 /* NXP "RK055MHD091A0-CTG MIPI Rectangular Display */
#define DEMO_PANEL_RASPI_7INCH 5 /* Raspberry Pi panel 7 inch */
#define DEMO_PANEL_CO5300      6 /* NXP ZC143AC72MIPI MIPI Circular Display */

/* @TEST_ANCHOR */
/* Configure this macro in Kconfig or directly in the generated mcux_config.h. */
#ifndef DEMO_PANEL
#define DEMO_PANEL DEMO_PANEL_RK055MHD091
#endif

#define DEMO_ALIGN_ADDR(addr, align) ((((addr) / (align) * (align)) == (addr)) ? (addr) : ((addr) / (align) * (align) + (align)))

#if (DEMO_PANEL_TFT_PROTO_5 == DEMO_PANEL)

#define SSD1963_DRIVEN_BY_FLEXIO 0
#define SSD1963_DRIVEN_BY_LCDIF 1

/* Configure this macro in Kconfig or directly in the generated mcux_config.h. */
#ifndef SSD1963_DRIVEN_BY
#define SSD1963_DRIVEN_BY SSD1963_DRIVEN_BY_LCDIF
#endif

/* Pixel format macro mapping. */
#define DEMO_SSD1963_BUFFER_RGB565   0
#define DEMO_SSD1963_BUFFER_RGB888   1

#ifndef DEMO_SSD1963_BUFFER_FORMAT
#define DEMO_SSD1963_BUFFER_FORMAT DEMO_SSD1963_BUFFER_RGB565
#endif

#if ((SSD1963_DRIVEN_BY == SSD1963_DRIVEN_BY_FLEXIO) && (DEMO_SSD1963_BUFFER_FORMAT == DEMO_SSD1963_BUFFER_RGB565))
#error For 8-bit 8080 data bus, the pixels sent to LCD controller should be RGB888 or BGR888. For FLEXIO driven type, the pixel format of the source can only be the same as data sent on bus.
#endif

#if (DEMO_SSD1963_BUFFER_FORMAT == DEMO_SSD1963_BUFFER_RGB565)
#define DEMO_BUFFER_PIXEL_FORMAT   kVIDEO_PixelFormatRGB565
#define DEMO_BUFFER_BYTE_PER_PIXEL 2
#define LVGL_FB_ALIGN              64U
#else
#define DEMO_BUFFER_PIXEL_FORMAT   kVIDEO_PixelFormatRGB888
#define DEMO_BUFFER_BYTE_PER_PIXEL 3
#define LVGL_FB_ALIGN              192U
#endif

/*
 * Use the 8080 panel
 */

#define DEMO_PANEL_WIDTH  800U
#define DEMO_PANEL_HEIGHT 480U

#define DEMO_BUFFER_FIXED_ADDRESS 1

/* Put frame buffer in PSRAM */
#define DEMO_BUFFER0_ADDR 0x60000000U
#define DEMO_BUFFER1_ADDR 0x60200000U

/* Definitions for the frame buffer. */
#define DEMO_BUFFER_COUNT  1 /* 1 is enough for DBI interface display. */

#define DEMO_FB_WIDTH  (DEMO_PANEL_WIDTH)
#define DEMO_FB_HEIGHT (DEMO_PANEL_HEIGHT)
#define DEMO_BUFFER_WIDTH   DEMO_FB_WIDTH
#define DEMO_BUFFER_HEIGHT  DEMO_FB_HEIGHT

/* Where the frame buffer is shown in the screen. */
#define DEMO_BUFFER_START_X 0U
#define DEMO_BUFFER_START_Y 0U

#if (SSD1963_DRIVEN_BY == SSD1963_DRIVEN_BY_FLEXIO)
/* No align requirement for FLEXIO. */
#define DEMO_BUFFER_STRIDE_BYTE DEMO_FB_WIDTH * DEMO_BUFFER_BYTE_PER_PIXEL
#else
#if (DEMO_SSD1963_BUFFER_FORMAT == DEMO_SSD1963_BUFFER_RGB565)
#define DEMO_BUFFER_STRIDE_BYTE DEMO_ALIGN_ADDR((DEMO_FB_WIDTH * DEMO_BUFFER_BYTE_PER_PIXEL), 64U)
#else
/* For RGB888 format, the stride shall also be divisible by 3. */
#define DEMO_BUFFER_STRIDE_BYTE DEMO_ALIGN_ADDR((DEMO_FB_WIDTH * DEMO_BUFFER_BYTE_PER_PIXEL), (64U * 3U))
#endif
#endif

#elif ((DEMO_PANEL_RK055AHD091 == DEMO_PANEL) || (DEMO_PANEL_RK055IQH091 == DEMO_PANEL) || \
       (DEMO_PANEL_RK055MHD091 == DEMO_PANEL) || (DEMO_PANEL_RASPI_7INCH == DEMO_PANEL))

#define DEMO_BUFFER_FIXED_ADDRESS 1
/*
 * To get best performance, frame buffer should be in dedicate SRAM partition.
 * But due to the high resolution of MIPI panel and limitted SRAM size, here we put
 * frame buffer in on-board PSRAM.
 */

/*
 * Use the MIPI dumb panel
 */

/* Definitions for the frame buffer. */
#define DEMO_BUFFER_COUNT         2   /* 2 is enough for DPI interface display. */
#define FRAME_BUFFER_ALIGN        128 /* LCDIF buffer should be 128 byte aligned. */

#if ((DEMO_PANEL_RK055AHD091 == DEMO_PANEL) || (DEMO_PANEL_RK055MHD091 == DEMO_PANEL))

#ifndef DEMO_RK055AHD091_USE_XRGB8888
#define DEMO_RK055AHD091_USE_XRGB8888 0
#endif

#ifndef DEMO_RK055MHD091_USE_XRGB8888
#define DEMO_RK055MHD091_USE_XRGB8888 0
#endif

#if (DEMO_RK055AHD091_USE_XRGB8888 || DEMO_RK055MHD091_USE_XRGB8888)

/* Frame buffer #0 is 720 x 1280 x 4 = 0x384000 bytes long */
#define DEMO_BUFFER0_ADDR 0x60000000U
#define DEMO_BUFFER1_ADDR 0x60400000U

#define DEMO_BUFFER_PIXEL_FORMAT   kVIDEO_PixelFormatXRGB8888
#define DEMO_BUFFER_BYTE_PER_PIXEL 4
#define LVGL_FB_ALIGN              64U

#else

/* Frame buffer #0 is 720 x 1280 x 2 = 0x1C2000 bytes long */
#define DEMO_BUFFER0_ADDR 0x60000000U
#define DEMO_BUFFER1_ADDR 0x60200000U

#define DEMO_BUFFER_PIXEL_FORMAT   kVIDEO_PixelFormatRGB565
#define DEMO_BUFFER_BYTE_PER_PIXEL 2
#define LVGL_FB_ALIGN              64U

#endif

#define DEMO_PANEL_WIDTH  (720)
#define DEMO_PANEL_HEIGHT (1280)

#elif (DEMO_PANEL_RK055IQH091 == DEMO_PANEL)

#define DEMO_BUFFER0_ADDR 0x60000000U
#define DEMO_BUFFER1_ADDR 0x60200000U

#define DEMO_BUFFER_PIXEL_FORMAT   kVIDEO_PixelFormatRGB565
#define DEMO_BUFFER_BYTE_PER_PIXEL 2
#define LVGL_FB_ALIGN              64U

#define DEMO_PANEL_WIDTH  (540)
#define DEMO_PANEL_HEIGHT (960)

#elif (DEMO_PANEL_RASPI_7INCH == DEMO_PANEL)

#define DEMO_BUFFER0_ADDR 0x60000000U
#define DEMO_BUFFER1_ADDR 0x60200000U

#define DEMO_BUFFER_PIXEL_FORMAT   kVIDEO_PixelFormatRGB565
#define DEMO_BUFFER_BYTE_PER_PIXEL 2
#define LVGL_FB_ALIGN              64U

#define DEMO_PANEL_WIDTH  (800)
#define DEMO_PANEL_HEIGHT (480)

#endif

#define DEMO_FB_WIDTH   DEMO_PANEL_WIDTH
#define DEMO_FB_HEIGHT  DEMO_PANEL_HEIGHT
#define DEMO_BUFFER_WIDTH   DEMO_FB_WIDTH
#define DEMO_BUFFER_HEIGHT  DEMO_FB_HEIGHT

/* Where the frame buffer is shown in the screen. */
#define DEMO_BUFFER_START_X 0U
#define DEMO_BUFFER_START_Y 0U

#define DEMO_BUFFER_STRIDE_BYTE DEMO_ALIGN_ADDR((DEMO_FB_WIDTH * DEMO_BUFFER_BYTE_PER_PIXEL), 64U)

#elif (DEMO_PANEL_RM67162 == DEMO_PANEL)

/* Default use LCDIF DBI interface to transfer pixel to MIPI. */
#define RM67162_USE_LCDIF 1

/* Pixel format macro mapping. */
#define DEMO_RM67162_BUFFER_RGB565   0
#define DEMO_RM67162_BUFFER_RGB888   1

#ifndef DEMO_RM67162_BUFFER_FORMAT
#define DEMO_RM67162_BUFFER_FORMAT DEMO_RM67162_BUFFER_RGB565
#endif

#if (!RM67162_USE_LCDIF && (DEMO_RM67162_BUFFER_FORMAT == DEMO_RM67162_BUFFER_RGB888))
#error When using MIPI interrupt way, the frame buffer format must be the same as panel interface pixel format which is RGB565.
#endif

/* Use fixed address to place buffer on PSRAM. */
#define DEMO_BUFFER_FIXED_ADDRESS 1

/*
 * Place frame buffer in on-board PSRAM.
 */
#define DEMO_BUFFER0_ADDR 0x60000000U
#define DEMO_BUFFER1_ADDR 0x60200000U

/* Definitions for the frame buffer. */
/* 1 is enough, use 2 could render background buffer while display the foreground buffer. */
#define DEMO_BUFFER_COUNT  2
#define FRAME_BUFFER_ALIGN 128 /* LCDIF buffer should be 128 byte aligned. */

#if (DEMO_RM67162_BUFFER_FORMAT == DEMO_RM67162_BUFFER_RGB565)

#define DEMO_BUFFER_PIXEL_FORMAT   kVIDEO_PixelFormatRGB565
#define DEMO_BUFFER_BYTE_PER_PIXEL 2
#define LVGL_FB_ALIGN              64U

#elif (DEMO_RM67162_BUFFER_FORMAT == DEMO_RM67162_BUFFER_RGB888)

#define DEMO_BUFFER_PIXEL_FORMAT   kVIDEO_PixelFormatRGB888
#define DEMO_BUFFER_BYTE_PER_PIXEL 3
#define LVGL_FB_ALIGN              192U

#endif /* DEMO_RM67162_BUFFER_FORMAT */

#define DEMO_PANEL_WIDTH  (400U)
#define DEMO_PANEL_HEIGHT (392U)

#define DEMO_FB_WIDTH   (400U)
#define DEMO_FB_HEIGHT  (392U)
#define DEMO_BUFFER_WIDTH   DEMO_FB_WIDTH
#define DEMO_BUFFER_HEIGHT  DEMO_FB_HEIGHT

/* Where the frame buffer is shown in the screen. */
#define DEMO_BUFFER_START_X 4U
#define DEMO_BUFFER_START_Y 0U

#if RM67162_USE_LCDIF
#if (DEMO_RM67162_BUFFER_FORMAT == DEMO_RM67162_BUFFER_RGB565)
#define DEMO_BUFFER_STRIDE_BYTE DEMO_ALIGN_ADDR((DEMO_FB_WIDTH * DEMO_BUFFER_BYTE_PER_PIXEL), 64U)
#else
/* For RGB888 format, the stride shall also be divisible by 3. */
#define DEMO_BUFFER_STRIDE_BYTE DEMO_ALIGN_ADDR((DEMO_FB_WIDTH * DEMO_BUFFER_BYTE_PER_PIXEL), (64U * 3U))
#endif
#else
/* No align requirement for MIPI APB. */
#define DEMO_BUFFER_STRIDE_BYTE DEMO_FB_WIDTH * DEMO_BUFFER_BYTE_PER_PIXEL
#endif

#elif (DEMO_PANEL_CO5300 == DEMO_PANEL)

/* Default use LCDIF DBI interface to transfer pixel to MIPI. */
#define CO5300_USE_LCDIF 1

/* Pixel format macro mapping. */
#define DEMO_CO5300_BUFFER_RGB565   0
#define DEMO_CO5300_BUFFER_RGB888   1

#ifndef DEMO_CO5300_BUFFER_FORMAT
#define DEMO_CO5300_BUFFER_FORMAT DEMO_CO5300_BUFFER_RGB565
#endif

#if (!CO5300_USE_LCDIF && (DEMO_CO5300_BUFFER_FORMAT == DEMO_CO5300_BUFFER_RGB888))
#error When using MIPI interrupt way, the frame buffer format must be the same as panel interface pixel format which is RGB565.
#endif

/* Use fixed address to place buffer on PSRAM. */
#define DEMO_BUFFER_FIXED_ADDRESS 1

/*
 * Place frame buffer in on-board PSRAM.
 */
#define DEMO_BUFFER0_ADDR 0x60000000U
#define DEMO_BUFFER1_ADDR 0x60200000U

/* Definitions for the frame buffer. */
/* 1 is enough, use 2 could render background buffer while display the foreground buffer. */
#define DEMO_BUFFER_COUNT  2
#define FRAME_BUFFER_ALIGN 128 /* LCDIF buffer should be 128 byte aligned. */

#if (DEMO_CO5300_BUFFER_FORMAT == DEMO_CO5300_BUFFER_RGB565)

#define DEMO_BUFFER_PIXEL_FORMAT   kVIDEO_PixelFormatRGB565
#define DEMO_BUFFER_BYTE_PER_PIXEL 2

#elif (DEMO_CO5300_BUFFER_FORMAT == DEMO_CO5300_BUFFER_RGB888)

#define DEMO_BUFFER_PIXEL_FORMAT   kVIDEO_PixelFormatRGB888
#define DEMO_BUFFER_BYTE_PER_PIXEL 3

#endif /* DEMO_CO5300_BUFFER_FORMAT */

#define DEMO_PANEL_WIDTH  (480U)
#define DEMO_PANEL_HEIGHT (466U)

#define DEMO_FB_WIDTH   466U
#define DEMO_FB_HEIGHT  466U /* Circular panel, only 466x466 pixels are shown. */
#define DEMO_BUFFER_WIDTH   DEMO_FB_WIDTH
#define DEMO_BUFFER_HEIGHT  DEMO_FB_HEIGHT

/* Where the frame buffer is shown in the screen. */
#define DEMO_BUFFER_START_X 6U /* The hardware actually starts to show from pixel 7. */
#define DEMO_BUFFER_START_Y 0U

#if CO5300_USE_LCDIF
#if (DEMO_CO5300_BUFFER_FORMAT == DEMO_CO5300_BUFFER_RGB565)
#define DEMO_BUFFER_STRIDE_BYTE DEMO_ALIGN_ADDR((DEMO_FB_WIDTH * DEMO_BUFFER_BYTE_PER_PIXEL), 64U)
#else
/* For RGB888 format, the stride shall also be divisible by 3. */
#define DEMO_BUFFER_STRIDE_BYTE DEMO_ALIGN_ADDR((DEMO_FB_WIDTH * DEMO_BUFFER_BYTE_PER_PIXEL), (64U * 3U))
#endif
#else
/* No align requirement for MIPI APB. */
#define DEMO_BUFFER_STRIDE_BYTE DEMO_FB_WIDTH * DEMO_BUFFER_BYTE_PER_PIXEL
#endif

#endif

extern const dc_fb_t g_dc;

/*******************************************************************************
 * API
 ******************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

status_t BOARD_PrepareDisplayController(void);
void BOARD_DisplayTEPinHandler(void);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* _DISPLAY_SUPPORT_H_ */
