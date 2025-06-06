/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _APP_H_
#define _APP_H_

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*${macro:start}*/
#define DEMO_GPIO_INSTANCE      GPIO0
#define DEMO_INPUT_PIN_NUMBER   (6U)
#define DEMO_OUTPUT_PIN_NUMBER  (7U)

#define DEMO_INPUTMUX_INSTANCE  INPUTMUX0

#define DEMO_SEMA42_INSTANCE     SEMA42_4
#define DEMO_SEMA42_PRINTF_GATE  (0)
#define DEMO_SEMA42_CORE_ID_CM33 (0)
/*${macro:end}*/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*${prototype:start}*/
void BOARD_InitHardware(void);
/*${prototype:end}*/

#endif /* _APP_H_ */
