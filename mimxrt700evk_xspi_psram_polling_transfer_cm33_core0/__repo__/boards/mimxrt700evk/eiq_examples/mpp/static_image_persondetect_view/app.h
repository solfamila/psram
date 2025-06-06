/*
 * Copyright 2023 NXP
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _APP_H_
#define _APP_H_

/*${header:start}*/
#include "fsl_power.h"
/*${header:end}*/

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*${macro:start}*/
#define DEMO_USE_XSPI2 1

#if defined(DEMO_USE_XSPI2) && DEMO_USE_XSPI2
#define EXAMPLE_XSPI                    XSPI2
#define EXAMPLE_XSPI_AMBA_BASE          0x60000000U
#define EXAMPLE_XSPI_CLOCK              kCLOCK_Xspi2
#elif defined(DEMO_USE_XSPI1) && DEMO_USE_XSPI1
#define EXAMPLE_XSPI                    XSPI1
#define EXAMPLE_XSPI_AMBA_BASE          0x08000000U
#define EXAMPLE_XSPI_CLOCK              kCLOCK_Xspi1
#endif
#define DRAM_SIZE                       0x4000 /* 16Mb/KByte */
#define EXAMPLE_XSPI_RX_SAMPLE_CLOCK    kXSPI_ReadSampleClkExternalInputFromDqsPad

#define FSL_FEATURE_XSPI_AHB_BUFFER_COUNT 4

#define HYPERRAM_CMD_LUT_SEQ_IDX_SYNC_READ   0
#define HYPERRAM_CMD_LUT_SEQ_IDX_SYNC_WRITE  1
#define HYPERRAM_CMD_LUT_SEQ_IDX_BURST_READ  2
#define HYPERRAM_CMD_LUT_SEQ_IDX_BURST_WRITE 3
#define HYPERRAM_CMD_LUT_SEQ_IDX_REG_READ    4
#define HYPERRAM_CMD_LUT_SEQ_IDX_REG_WRITE   5
#define HYPERRAM_CMD_LUT_SEQ_IDX_RESET       6

#define CUSTOM_LUT_LENGTH        80

/*${macro:end}*/

/*******************************************************************************
 * Variables
 ******************************************************************************/
/*${variable:start}*/
typedef struct _xspi_cache_status
{
    volatile bool codeCacheEnableFlag;
    volatile bool systemCacheEnableFlag;
} xspi_cache_status_t;
/*${variable:end}*/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*${prototype:start}*/
void BOARD_Init(void);
static inline void xspi_clock_init(void)
{
#if defined(DEMO_USE_XSPI2) && DEMO_USE_XSPI2
    POWER_DisablePD(kPDRUNCFG_APD_XSPI2);
    POWER_DisablePD(kPDRUNCFG_PPD_XSPI2);
    POWER_ApplyPD();
#elif defined(DEMO_USE_XSPI1) && DEMO_USE_XSPI1
    POWER_DisablePD(kPDRUNCFG_APD_XSPI1);
    POWER_DisablePD(kPDRUNCFG_PPD_XSPI1);
    POWER_ApplyPD();
#endif
}
/*${prototype:end}*/

#endif /* _APP_H_ */
