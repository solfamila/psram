/*
 * Copyright 2024 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/*${header:start}*/
#include <xtensa/config/core.h>
#include <xtensa/xos.h>
#include "app.h"
#include "fsl_common.h"
#include "fsl_gpio.h"
#include "fsl_inputmux.h"
#include "fsl_edma.h"
#include "fsl_sai.h"
#include "pin_mux.h"
#include "srtm_utils.h"
#include "fsl_sema42.h"
#include "board.h"
/*${header:end}*/

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*${macro:start}*/
/*${macro:end}*/

/*******************************************************************************
 * Variables
 ******************************************************************************/
/*${variable:start}*/
extern int NonCacheable_start, NonCacheable_end;
extern int NonCacheable_init_start, NonCacheable_init_end;
AT_NONCACHEABLE_SECTION_ALIGN(edma_handle_t g_saiDmaHandle, 32);
AT_NONCACHEABLE_SECTION_ALIGN(edma_handle_t g_pdmDmaHandle, 32);
/*${variable:end}*/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*${function:start}*/
static void BOARD_InitClock(void)
{
    INPUTMUX_Init(INPUTMUX0);
    INPUTMUX_AttachSignal(INPUTMUX0, 17, kINPUTMUX_Dma0Irq0ToDspInterrupt);
    INPUTMUX_AttachSignal(INPUTMUX0, 18, kINPUTMUX_Dma0Irq1ToDspInterrupt);
    INPUTMUX_AttachSignal(INPUTMUX0, 1U, kINPUTMUX_Mu4BToDspInterrupt);
}

static void XOS_Init(void)
{
    xos_set_clock_freq(XOS_CLOCK_FREQ);
    xos_start_system_timer(-1, 0);
    /* Map DMA IRQ handler to INPUTMUX selection EXTINT18 = DSP INT 23 */
    xos_register_interrupt_handler(XCHAL_EXTINT18_NUM, (XosIntFunc *)EDMA_HandleIRQ, &g_pdmDmaHandle);
    xos_interrupt_enable(XCHAL_EXTINT18_NUM);
    /* Map DMA IRQ handler to INPUTMUX selection EXTINT17 = DSP INT 22 */
    xos_register_interrupt_handler(XCHAL_EXTINT17_NUM, (XosIntFunc *)EDMA_HandleIRQ, &g_saiDmaHandle);
    xos_interrupt_enable(XCHAL_EXTINT17_NUM);
}

void BOARD_InitHardware(void)
{
    edma_config_t dmaConfig = {0};
    /* Note: need tell clock driver the frequency of OSC. */
    CLOCK_SetXtalFreq(BOARD_XTAL_SYS_CLK_HZ);

    BOARD_InitBootPins();
    BOARD_InitDebugConsole();
    BOARD_InitClock();
    XOS_Init();

    /* Iniitalize DMA0 which will be shared by capturer and renderer. */
    EDMA_GetDefaultConfig(&dmaConfig);
    EDMA_Init(DEMO_DMA, &dmaConfig);

    /* SEMA42 init */
    SEMA42_Init(APP_SEMA42);
}

/*${function:end}*/
