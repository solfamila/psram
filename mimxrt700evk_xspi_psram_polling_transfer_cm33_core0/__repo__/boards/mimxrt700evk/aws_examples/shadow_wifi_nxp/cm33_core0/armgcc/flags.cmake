IF(NOT DEFINED FPU)  
    SET(FPU "-mfloat-abi=hard -mfpu=fpv5-sp-d16")  
ENDIF()  

IF(NOT DEFINED SPECS)  
    SET(SPECS "--specs=nosys.specs")  
ENDIF()  

IF(NOT DEFINED DEBUG_CONSOLE_CONFIG)  
    SET(DEBUG_CONSOLE_CONFIG "-DSDK_DEBUGCONSOLE=1")  
ENDIF()  

SET(CMAKE_ASM_FLAGS_FLASH_DEBUG " \
    ${CMAKE_ASM_FLAGS_FLASH_DEBUG} \
    -D__STARTUP_INITIALIZE_NONCACHEDATA \
    -D__STARTUP_CLEAR_BSS \
    -DMCUXPRESSO_SDK \
    -DCPU_MIMXRT798SGFOA_cm33_core0 \
    -DDEBUG_CONSOLE_TRANSFER_NON_BLOCKING \
    -DOSA_USED \
    -g \
    -mthumb \
    -mcpu=cortex-m33 \
    ${FPU} \
")
SET(CMAKE_ASM_FLAGS_FLASH_RELEASE " \
    ${CMAKE_ASM_FLAGS_FLASH_RELEASE} \
    -D__STARTUP_INITIALIZE_NONCACHEDATA \
    -D__STARTUP_CLEAR_BSS \
    -DMCUXPRESSO_SDK \
    -DCPU_MIMXRT798SGFOA_cm33_core0 \
    -DDEBUG_CONSOLE_TRANSFER_NON_BLOCKING \
    -DOSA_USED \
    -mthumb \
    -mcpu=cortex-m33 \
    ${FPU} \
")
SET(CMAKE_C_FLAGS_FLASH_DEBUG " \
    ${CMAKE_C_FLAGS_FLASH_DEBUG} \
    -include ${ProjDirPath}/../mcux_config.h \
    -DDEBUG \
    -DUSE_RTOS=1 \
    -DDISABLE_LOGGING \
    -DappmainMQTT_AGENT_TASK_STACK_SIZE=3072 \
    -DMCUX_META_BUILD \
    -DMCUXPRESSO_SDK \
    -DCPU_MIMXRT798SGFOA_cm33_core0 \
    -DDEBUG_CONSOLE_TRANSFER_NON_BLOCKING \
    -DOSA_USED \
    -DBOOT_HEADER_ENABLE=1 \
    -DSERIAL_PORT_TYPE_UART=1 \
    -DWIFI_IW612_BOARD_MURATA_2EL_M2 \
    -DPSA_CRYPTO_DRIVER_THREAD_EN \
    -DPSA_CRYPTO_DRIVER_ELS_PKC \
    -DMFLASH_FILE_BASEADDR=7340032 \
    -DCONFIG_NXP_WIFI_SOFTAP_SUPPORT=1 \
    -DSDIO_ENABLED=1 \
    -DSDK_OS_FREE_RTOS \
    -DMQTT_AGENT_DO_NOT_USE_CUSTOM_CONFIG \
    -DMBEDTLS_THREADING_ALT \
    -DMBEDTLS_THREADING_C \
    -g \
    -O0 \
    --specs=nano.specs \
    -Wall \
    -fno-common \
    -ffunction-sections \
    -fdata-sections \
    -fno-builtin \
    -mthumb \
    -mapcs \
    -std=gnu99 \
    -mcpu=cortex-m33 \
    ${FPU} \
    ${DEBUG_CONSOLE_CONFIG} \
")
SET(CMAKE_C_FLAGS_FLASH_RELEASE " \
    ${CMAKE_C_FLAGS_FLASH_RELEASE} \
    -include ${ProjDirPath}/../mcux_config.h \
    -DNDEBUG \
    -DUSE_RTOS=1 \
    -DDISABLE_LOGGING \
    -DappmainMQTT_AGENT_TASK_STACK_SIZE=3072 \
    -DMCUX_META_BUILD \
    -DMCUXPRESSO_SDK \
    -DCPU_MIMXRT798SGFOA_cm33_core0 \
    -DDEBUG_CONSOLE_TRANSFER_NON_BLOCKING \
    -DOSA_USED \
    -DBOOT_HEADER_ENABLE=1 \
    -DSERIAL_PORT_TYPE_UART=1 \
    -DWIFI_IW612_BOARD_MURATA_2EL_M2 \
    -DPSA_CRYPTO_DRIVER_THREAD_EN \
    -DPSA_CRYPTO_DRIVER_ELS_PKC \
    -DMFLASH_FILE_BASEADDR=7340032 \
    -DCONFIG_NXP_WIFI_SOFTAP_SUPPORT=1 \
    -DSDIO_ENABLED=1 \
    -DSDK_OS_FREE_RTOS \
    -DMQTT_AGENT_DO_NOT_USE_CUSTOM_CONFIG \
    -DMBEDTLS_THREADING_ALT \
    -DMBEDTLS_THREADING_C \
    -Os \
    --specs=nano.specs \
    -Wall \
    -fno-common \
    -ffunction-sections \
    -fdata-sections \
    -fno-builtin \
    -mthumb \
    -mapcs \
    -std=gnu99 \
    -mcpu=cortex-m33 \
    ${FPU} \
    ${DEBUG_CONSOLE_CONFIG} \
")
SET(CMAKE_CXX_FLAGS_FLASH_DEBUG " \
    ${CMAKE_CXX_FLAGS_FLASH_DEBUG} \
    -DDEBUG \
    -DMCUX_META_BUILD \
    -DMCUXPRESSO_SDK \
    -DCPU_MIMXRT798SGFOA_cm33_core0 \
    -DDEBUG_CONSOLE_TRANSFER_NON_BLOCKING \
    -DOSA_USED \
    -DBOOT_HEADER_ENABLE=1 \
    -DSERIAL_PORT_TYPE_UART=1 \
    -DUSE_RTOS=1 \
    -DWIFI_IW612_BOARD_MURATA_2EL_M2 \
    -DMFLASH_FILE_BASEADDR=7340032 \
    -DSDK_OS_FREE_RTOS \
    -g \
    -O0 \
    --specs=nano.specs \
    -Wall \
    -fno-common \
    -ffunction-sections \
    -fdata-sections \
    -fno-builtin \
    -mthumb \
    -mapcs \
    -fno-rtti \
    -fno-exceptions \
    -mcpu=cortex-m33 \
    ${FPU} \
    ${DEBUG_CONSOLE_CONFIG} \
")
SET(CMAKE_CXX_FLAGS_FLASH_RELEASE " \
    ${CMAKE_CXX_FLAGS_FLASH_RELEASE} \
    -DNDEBUG \
    -DMCUX_META_BUILD \
    -DMCUXPRESSO_SDK \
    -DCPU_MIMXRT798SGFOA_cm33_core0 \
    -DDEBUG_CONSOLE_TRANSFER_NON_BLOCKING \
    -DOSA_USED \
    -DBOOT_HEADER_ENABLE=1 \
    -DSERIAL_PORT_TYPE_UART=1 \
    -DUSE_RTOS=1 \
    -DWIFI_IW612_BOARD_MURATA_2EL_M2 \
    -DMFLASH_FILE_BASEADDR=7340032 \
    -DSDK_OS_FREE_RTOS \
    -Os \
    --specs=nano.specs \
    -Wall \
    -fno-common \
    -ffunction-sections \
    -fdata-sections \
    -fno-builtin \
    -mthumb \
    -mapcs \
    -fno-rtti \
    -fno-exceptions \
    -mcpu=cortex-m33 \
    ${FPU} \
    ${DEBUG_CONSOLE_CONFIG} \
")
SET(CMAKE_EXE_LINKER_FLAGS_FLASH_DEBUG " \
    ${CMAKE_EXE_LINKER_FLAGS_FLASH_DEBUG} \
    -g \
    -Xlinker \
    --defsym=__stack_size__=0x2000 \
    -Xlinker \
    -Map=output.map \
    -Wall \
    -fno-common \
    -ffunction-sections \
    -fdata-sections \
    -fno-builtin \
    -mthumb \
    -mapcs \
    -Wl,--gc-sections \
    -Wl,-static \
    -Wl,--print-memory-usage \
    -mcpu=cortex-m33 \
    ${FPU} \
    ${SPECS} \
    -T\"${ProjDirPath}/MIMXRT798Sxxxx_cm33_core0_flash.ld\" -static \
")
SET(CMAKE_EXE_LINKER_FLAGS_FLASH_RELEASE " \
    ${CMAKE_EXE_LINKER_FLAGS_FLASH_RELEASE} \
    -Xlinker \
    --defsym=__stack_size__=0x2000 \
    -Xlinker \
    -Map=output.map \
    -Wall \
    -fno-common \
    -ffunction-sections \
    -fdata-sections \
    -fno-builtin \
    -mthumb \
    -mapcs \
    -Wl,--gc-sections \
    -Wl,-static \
    -Wl,--print-memory-usage \
    -mcpu=cortex-m33 \
    ${FPU} \
    ${SPECS} \
    -T\"${ProjDirPath}/MIMXRT798Sxxxx_cm33_core0_flash.ld\" -static \
")
