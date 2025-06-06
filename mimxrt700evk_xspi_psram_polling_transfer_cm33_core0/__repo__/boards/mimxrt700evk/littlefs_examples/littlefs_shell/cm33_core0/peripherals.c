/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/***********************************************************************************************************************
 * This file was generated by the MCUXpresso Config Tools. Any manual edits made to this file
 * will be overwritten if the respective MCUXpresso Config Tools is used to update this file.
 **********************************************************************************************************************/

/* clang-format off */
/* TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
!!GlobalInfo
product: Peripherals v15.0
processor: MIMXRT798S
package_id: MIMXRT798SGFOA
mcu_data: ksdk2_0
processor_version: 0.2412.60
functionalGroups:
- name: BOARD_InitPeripherals
  UUID: 673cf921-1ec2-4930-93f2-e0768a84092c
  called_from_default_init: true
  selectedCore: cm33_core0
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS **********/

/* TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
component:
- type: 'system'
- type_id: 'system_54b53072540eeeb8f8e9343e71f28176'
- global_system_definitions:
  - user_definitions: ''
  - user_includes: ''
  - global_init: ''
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS **********/

/* TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
component:
- type: 'uart_cmsis_common'
- type_id: 'uart_cmsis_common_9cb8e302497aa696fdbb5a4fd622c2a8'
- global_USART_CMSIS_common:
  - quick_selection: 'default'
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS **********/

/* TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
component:
- type: 'gpio_adapter_common'
- type_id: 'gpio_adapter_common'
- global_gpio_adapter_common:
  - quick_selection: 'default'
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS **********/
/* clang-format on */

/***********************************************************************************************************************
 * Included files
 **********************************************************************************************************************/
#include "peripherals.h"

/***********************************************************************************************************************
 * BOARD_InitPeripherals functional group
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * NVIC0 initialization code
 **********************************************************************************************************************/
/* clang-format off */
/* TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
instance:
- name: 'NVIC0'
- type: 'nvic'
- mode: 'general'
- custom_name_enabled: 'false'
- type_id: 'nvic'
- functional_group: 'BOARD_InitPeripherals'
- peripheral: 'NVIC0'
- config_sets:
  - nvic:
    - interrupt_table: []
    - interrupts: []
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS **********/
/* clang-format on */

/* Empty initialization function (commented out)
static void NVIC0_init(void) {
} */

/***********************************************************************************************************************
 * LittleFS initialization code
 **********************************************************************************************************************/
/* clang-format off */
/* TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
instance:
- name: 'LittleFS'
- type: 'littlefs'
- mode: 'general'
- custom_name_enabled: 'false'
- type_id: 'littlefs_2.4.0'
- functional_group: 'BOARD_InitPeripherals'
- config_sets:
  - general_config:
    - moduleInclude: 'lfs_mflash.h'
    - lfsConfig:
      - enableUserContext: 'true'
      - userContext:
        - contextVar: '(void*)&LittleFS_ctx'
        - contextDef: ''
        - contextExternalDef: 'struct lfs_mflash_ctx LittleFS_ctx'
      - userCallbacks:
        - read: 'lfs_mflash_read'
        - prog: 'lfs_mflash_prog'
        - erase: 'lfs_mflash_erase'
        - sync: 'lfs_mflash_sync'
        - lock: ''
        - unlock: ''
      - readSize: '16'
      - progSize: '256'
      - blockSize: '4096'
      - firstBlock: '0x00400000'
      - blockCount: '1024'
      - blockCycles: '100'
      - cacheSize: '256'
      - lookaheadSize: '16'
      - enableReadBuff: 'false'
      - readBuffer:
        - customBuffer: 'false'
      - enableProgBuff: 'false'
      - progBuffer:
        - customBuffer: 'false'
      - enableLookaheadBuff: 'false'
      - lookaheadBuffer:
        - customBuffer: 'false'
      - enableOptionalSizes: 'false'
      - optionalSizes:
        - name_max: '255'
        - file_max: '0x7FFFFFFF'
        - attr_max: '1022'
        - metadata_max: '4096'
    - initLFS: 'false'
    - initConfig:
      - lfsObj: 'LittleFS_system'
      - mountLFS: 'disable'
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS **********/
/* clang-format on */
const struct lfs_config LittleFS_config = {
  .context = (void*)&LittleFS_ctx,
  .read = lfs_mflash_read,
  .prog = lfs_mflash_prog,
  .erase = lfs_mflash_erase,
  .sync = lfs_mflash_sync,
#ifdef LFS_THREADSAFE
  .lock = NULL,
  .unlock = NULL,
#endif
  .read_size = LITTLEFS_READ_SIZE,
  .prog_size = LITTLEFS_PROG_SIZE,
  .block_size = LITTLEFS_BLOCK_SIZE,
  .block_count = LITTLEFS_BLOCK_COUNT,
  .block_cycles = LITTLEFS_BLOCK_CYCLES,
  .cache_size = LITTLEFS_CACHE_SIZE,
  .lookahead_size = LITTLEFS_LOOKAHEAD_SIZE
};

/* Empty initialization function (commented out)
static void LittleFS_init(void) {
} */

/***********************************************************************************************************************
 * Initialization functions
 **********************************************************************************************************************/
void BOARD_InitPeripherals(void)
{
  /* Initialize components */
}

/***********************************************************************************************************************
 * BOARD_InitBootPeripherals function
 **********************************************************************************************************************/
void BOARD_InitBootPeripherals(void)
{
  BOARD_InitPeripherals();
}
