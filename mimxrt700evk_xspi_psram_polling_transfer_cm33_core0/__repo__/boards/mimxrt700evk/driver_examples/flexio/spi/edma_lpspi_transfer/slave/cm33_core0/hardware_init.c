/*
 * Copyright 2023-2024 NXP
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*${header:start}*/
#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"
#include "fsl_clock.h"
#include "app.h"
#include "fsl_power.h"
#include "fsl_edma_soc.h"
/*${header:end}*/

/*${function:start}*/
void BOARD_InitHardware(void)
{
    BOARD_ConfigMPU();
    BOARD_InitPins();
    BOARD_BootClockRUN();

    BOARD_InitDebugConsole();

    BOARD_InitAHBSC();

    /* The domain own LPSPI16 and FLEXIO, disable mask in SHARED_MASK0. */
    POWER_DisableLPRequestMask(kPower_MaskLpspi16 | kPower_MaskFlexio);

    CLOCK_AttachClk(kFRO1_DIV1_to_LPSPI16);
    CLOCK_SetClkDiv(kCLOCK_DivLpspi16Clk, 16U);

    CLOCK_AttachClk(kFRO1_DIV1_to_FLEXIO);
    CLOCK_SetClkDiv(kCLOCK_DivFlexioClk, 16U);
    RESET_ClearPeripheralReset(kFLEXIO0_RST_SHIFT_RSTn);
    RESET_ClearPeripheralReset(kLPSPI16_RST_SHIFT_RSTn);
    EDMA_EnableRequest(EXAMPLE_FLEXIO_SPI_DMA_LPSPI_BASEADDR, EXAMPLE_TX_DMA_SOURCE);
    EDMA_EnableRequest(EXAMPLE_FLEXIO_SPI_DMA_LPSPI_BASEADDR, EXAMPLE_RX_DMA_SOURCE);
}
/*${function:end}*/
