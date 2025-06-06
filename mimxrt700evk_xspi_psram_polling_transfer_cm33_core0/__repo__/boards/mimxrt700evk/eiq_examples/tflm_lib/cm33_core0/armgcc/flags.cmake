IF(NOT DEFINED FPU)  
    SET(FPU "-mfloat-abi=hard -mfpu=fpv5-sp-d16")  
ENDIF()  

IF(NOT DEFINED DEBUG_CONSOLE_CONFIG)  
    SET(DEBUG_CONSOLE_CONFIG "-DSDK_DEBUGCONSOLE=1")  
ENDIF()  

SET(CMAKE_ASM_FLAGS_DEBUG " \
    ${CMAKE_ASM_FLAGS_DEBUG} \
    -D__STARTUP_CLEAR_BSS \
    -DCPU_MIMXRT798SGFOA_cm33_core0 \
    -mthumb \
    -mcpu=cortex-m33 \
    ${FPU} \
")
SET(CMAKE_ASM_FLAGS_RELEASE " \
    ${CMAKE_ASM_FLAGS_RELEASE} \
    -D__STARTUP_CLEAR_BSS \
    -DCPU_MIMXRT798SGFOA_cm33_core0 \
    -mthumb \
    -mcpu=cortex-m33 \
    ${FPU} \
")
SET(CMAKE_C_FLAGS_DEBUG " \
    ${CMAKE_C_FLAGS_DEBUG} \
    -include ${ProjDirPath}/../mcux_config.h \
    -DDEBUG \
    -DNDEBUG \
    -DCPU_MIMXRT798SGFOA_cm33_core0 \
    -DTF_LITE_STATIC_MEMORY \
    -DCMSIS_NN \
    -Wall \
    -fno-common \
    -ffunction-sections \
    -fdata-sections \
    -mthumb \
    -mapcs \
    -O2 \
    --specs=nano.specs \
    -fno-builtin \
    -std=gnu99 \
    -mcpu=cortex-m33 \
    -std=c17 \
    ${FPU} \
    ${DEBUG_CONSOLE_CONFIG} \
")
SET(CMAKE_C_FLAGS_RELEASE " \
    ${CMAKE_C_FLAGS_RELEASE} \
    -include ${ProjDirPath}/../mcux_config.h \
    -DNDEBUG \
    -DCPU_MIMXRT798SGFOA_cm33_core0 \
    -DTF_LITE_STATIC_MEMORY \
    -DCMSIS_NN \
    -Wall \
    -fno-common \
    -ffunction-sections \
    -fdata-sections \
    -mthumb \
    -mapcs \
    -O2 \
    --specs=nano.specs \
    -fno-builtin \
    -std=gnu99 \
    -mcpu=cortex-m33 \
    -std=c17 \
    ${FPU} \
    ${DEBUG_CONSOLE_CONFIG} \
")
SET(CMAKE_CXX_FLAGS_DEBUG " \
    ${CMAKE_CXX_FLAGS_DEBUG} \
    -DDEBUG \
    -DNDEBUG \
    -DCPU_MIMXRT798SGFOA_cm33_core0 \
    -DTF_LITE_STATIC_MEMORY \
    -DCMSIS_NN \
    -Wall \
    -fno-common \
    -ffunction-sections \
    -fdata-sections \
    -mthumb \
    -mapcs \
    -fno-rtti \
    -fno-exceptions \
    -O2 \
    --specs=nano.specs \
    -fno-builtin \
    -mcpu=cortex-m33 \
    -std=gnu++17 \
    ${FPU} \
    ${DEBUG_CONSOLE_CONFIG} \
")
SET(CMAKE_CXX_FLAGS_RELEASE " \
    ${CMAKE_CXX_FLAGS_RELEASE} \
    -DNDEBUG \
    -DCPU_MIMXRT798SGFOA_cm33_core0 \
    -DTF_LITE_STATIC_MEMORY \
    -DCMSIS_NN \
    -Wall \
    -fno-common \
    -ffunction-sections \
    -fdata-sections \
    -mthumb \
    -mapcs \
    -fno-rtti \
    -fno-exceptions \
    -O2 \
    --specs=nano.specs \
    -fno-builtin \
    -mcpu=cortex-m33 \
    -std=gnu++17 \
    ${FPU} \
    ${DEBUG_CONSOLE_CONFIG} \
")
