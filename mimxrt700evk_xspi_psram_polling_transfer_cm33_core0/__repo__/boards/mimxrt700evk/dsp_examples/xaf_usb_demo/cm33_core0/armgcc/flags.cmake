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
    -DDSP_IMAGE_COPY_TO_RAM=1 \
    -DMCUXPRESSO_SDK \
    -DCPU_MIMXRT798SGFOA_cm33_core0 \
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
    -DDSP_IMAGE_COPY_TO_RAM=1 \
    -DMCUXPRESSO_SDK \
    -DCPU_MIMXRT798SGFOA_cm33_core0 \
    -DOSA_USED \
    -mthumb \
    -mcpu=cortex-m33 \
    ${FPU} \
")
SET(CMAKE_C_FLAGS_FLASH_DEBUG " \
    ${CMAKE_C_FLAGS_FLASH_DEBUG} \
    -include ${ProjDirPath}/../mcux_config.h \
    -DDEBUG \
    -DXA_CAPTURER=1 \
    -DXA_PCM_GAIN=1 \
    -DXA_RENDERER \
    -DUSB_STACK_FREERTOS \
    -DDSP_IMAGE_COPY_TO_RAM=1 \
    -D__USE_SHMEM \
    -DUSB_STACK_FREERTOS_HEAP_SIZE=16384 \
    -DUSB_DEVICE_CONFIG_LPCIP3511HS=0 \
    -DUSB_DEVICE_CONFIG_EHCI=1 \
    -DMCUX_META_BUILD \
    -DMCUXPRESSO_SDK \
    -DCPU_MIMXRT798SGFOA_cm33_core0 \
    -DOSA_USED \
    -DBOOT_HEADER_ENABLE=1 \
    -DSERIAL_PORT_TYPE_UART=1 \
    -DSDK_I2C_BASED_COMPONENT_USED=1 \
    -DCODEC_MULTI_ADAPTERS=1 \
    -DCODEC_WM8962_ENABLE \
    -DUSE_RTOS=1 \
    -DSDK_OS_FREE_RTOS \
    -g \
    -O0 \
    -mno-unaligned-access \
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
    -DXA_CAPTURER=1 \
    -DXA_PCM_GAIN=1 \
    -DXA_RENDERER \
    -DUSB_STACK_FREERTOS \
    -DDSP_IMAGE_COPY_TO_RAM=1 \
    -D__USE_SHMEM \
    -DUSB_STACK_FREERTOS_HEAP_SIZE=16384 \
    -DUSB_DEVICE_CONFIG_LPCIP3511HS=0 \
    -DUSB_DEVICE_CONFIG_EHCI=1 \
    -DMCUX_META_BUILD \
    -DMCUXPRESSO_SDK \
    -DCPU_MIMXRT798SGFOA_cm33_core0 \
    -DOSA_USED \
    -DBOOT_HEADER_ENABLE=1 \
    -DSERIAL_PORT_TYPE_UART=1 \
    -DSDK_I2C_BASED_COMPONENT_USED=1 \
    -DCODEC_MULTI_ADAPTERS=1 \
    -DCODEC_WM8962_ENABLE \
    -DUSE_RTOS=1 \
    -DSDK_OS_FREE_RTOS \
    -Os \
    -mno-unaligned-access \
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
    -DOSA_USED \
    -DBOOT_HEADER_ENABLE=1 \
    -DSERIAL_PORT_TYPE_UART=1 \
    -DUSE_RTOS=1 \
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
    -DOSA_USED \
    -DBOOT_HEADER_ENABLE=1 \
    -DSERIAL_PORT_TYPE_UART=1 \
    -DUSE_RTOS=1 \
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
    --defsym=__use_shmem__=1 \
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
    --defsym=__use_shmem__=1 \
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
