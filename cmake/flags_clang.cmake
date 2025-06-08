# Clang/LLVM specific compiler and linker flags for ARM Cortex-M33
# This file defines all the compilation and linking flags needed for the MIMXRT700 project

# Set ARM GCC sysroot for embedded libraries
SET(ARM_GCC_SYSROOT "/usr/lib/arm-none-eabi")

# Floating point unit configuration
IF(NOT DEFINED FPU)  
    SET(FPU "-mfloat-abi=hard -mfpu=fpv5-sp-d16")  
ENDIF()  

# Specs file configuration (not used with Clang, but kept for compatibility)
IF(NOT DEFINED SPECS)  
    SET(SPECS "--specs=nosys.specs")  
ENDIF()

# Add ARM GCC sysroot and include paths for Clang
SET(ARM_SYSROOT_FLAGS "--sysroot=${ARM_GCC_SYSROOT} -I${ARM_GCC_SYSROOT}/include")
SET(ARM_LINK_FLAGS "-L${ARM_GCC_SYSROOT}/newlib/thumb/v8-m.main+fp/hard -L${ARM_GCC_SYSROOT}/newlib -L/usr/lib/gcc/arm-none-eabi/13.2.1/thumb/v8-m.main+fp/hard")

# Assembly flags for debug build
SET(CMAKE_ASM_FLAGS_DEBUG " \
    ${FPU} \
    ${ARM_SYSROOT_FLAGS} \
    -DDEBUG \
    -D__STARTUP_CLEAR_BSS \
    -mcpu=cortex-m33 \
    -mthumb \
    -Wall \
    -fno-common \
    -ffunction-sections \
    -fdata-sections \
    -fno-builtin \
    -std=gnu99 \
    -target arm-none-eabi \
")

# Assembly flags for release build
SET(CMAKE_ASM_FLAGS_RELEASE " \
    ${FPU} \
    ${ARM_SYSROOT_FLAGS} \
    -DNDEBUG \
    -D__STARTUP_CLEAR_BSS \
    -mcpu=cortex-m33 \
    -mthumb \
    -Wall \
    -fno-common \
    -ffunction-sections \
    -fdata-sections \
    -fno-builtin \
    -std=gnu99 \
    -target arm-none-eabi \
")

# C compiler flags for debug build
SET(CMAKE_C_FLAGS_DEBUG " \
    ${FPU} \
    ${ARM_SYSROOT_FLAGS} \
    -DDEBUG \
    -DCPU_MIMXRT798SGFOA_cm33_core0 \
    -DARM_MATH_CM33 \
    -D__FPU_PRESENT=1 \
    -DSERIAL_PORT_TYPE_UART=1 \
    -DSDK_DEBUGCONSOLE=1 \
    -DCR_INTEGER_PRINTF \
    -DPRINTF_FLOAT_ENABLE=0 \
    -D__MCUXPRESSO \
    -D__USE_CMSIS \
    -D__REDLIB__ \
    -DMCUXPRESSO_SDK \
    -g \
    -O0 \
    -mcpu=cortex-m33 \
    -mthumb \
    -MMD \
    -MP \
    -Wall \
    -fno-common \
    -ffunction-sections \
    -fdata-sections \
    -fno-builtin \
    -std=gnu99 \
    -target arm-none-eabi \
")

# C compiler flags for release build
SET(CMAKE_C_FLAGS_RELEASE " \
    ${FPU} \
    -DNDEBUG \
    -DCPU_MIMXRT798SGFOA_cm33_core0 \
    -DARM_MATH_CM33 \
    -D__FPU_PRESENT=1 \
    -DSERIAL_PORT_TYPE_UART=1 \
    -DSDK_DEBUGCONSOLE=1 \
    -DCR_INTEGER_PRINTF \
    -DPRINTF_FLOAT_ENABLE=0 \
    -D__MCUXPRESSO \
    -D__USE_CMSIS \
    -D__REDLIB__ \
    -DMCUXPRESSO_SDK \
    -Os \
    -mcpu=cortex-m33 \
    -mthumb \
    -MMD \
    -MP \
    -Wall \
    -fno-common \
    -ffunction-sections \
    -fdata-sections \
    -fno-builtin \
    -std=gnu99 \
    -target arm-none-eabi \
")

# C++ compiler flags for debug build
SET(CMAKE_CXX_FLAGS_DEBUG " \
    ${FPU} \
    -DDEBUG \
    -DCPU_MIMXRT798SGFOA_cm33_core0 \
    -DARM_MATH_CM33 \
    -D__FPU_PRESENT=1 \
    -DSERIAL_PORT_TYPE_UART=1 \
    -DSDK_DEBUGCONSOLE=1 \
    -DCR_INTEGER_PRINTF \
    -DPRINTF_FLOAT_ENABLE=0 \
    -D__MCUXPRESSO \
    -D__USE_CMSIS \
    -D__REDLIB__ \
    -DMCUXPRESSO_SDK \
    -g \
    -O0 \
    -mcpu=cortex-m33 \
    -mthumb \
    -MMD \
    -MP \
    -Wall \
    -fno-common \
    -ffunction-sections \
    -fdata-sections \
    -fno-builtin \
    -fno-rtti \
    -fno-exceptions \
    -target arm-none-eabi \
")

# C++ compiler flags for release build
SET(CMAKE_CXX_FLAGS_RELEASE " \
    ${FPU} \
    -DNDEBUG \
    -DCPU_MIMXRT798SGFOA_cm33_core0 \
    -DARM_MATH_CM33 \
    -D__FPU_PRESENT=1 \
    -DSERIAL_PORT_TYPE_UART=1 \
    -DSDK_DEBUGCONSOLE=1 \
    -DCR_INTEGER_PRINTF \
    -DPRINTF_FLOAT_ENABLE=0 \
    -D__MCUXPRESSO \
    -D__USE_CMSIS \
    -D__REDLIB__ \
    -DMCUXPRESSO_SDK \
    -Os \
    -mcpu=cortex-m33 \
    -mthumb \
    -MMD \
    -MP \
    -Wall \
    -fno-common \
    -ffunction-sections \
    -fdata-sections \
    -fno-builtin \
    -fno-rtti \
    -fno-exceptions \
    -target arm-none-eabi \
")

# Linker flags for debug build
SET(CMAKE_EXE_LINKER_FLAGS_DEBUG " \
    ${FPU} \
    ${ARM_SYSROOT_FLAGS} \
    ${ARM_LINK_FLAGS} \
    -Wl,-Map=output.map \
    -Wl,--gc-sections \
    -Wl,--print-memory-usage \
    -mcpu=cortex-m33 \
    -mthumb \
    -T${ProjDirPath}/MIMXRT798Sxxxx_cm33_core0_flash.ld \
    -static \
    -target arm-none-eabi \
    -fuse-ld=lld \
    -rtlib=libgcc \
    -Wl,--entry=Reset_Handler \
    -Wl,--defsym=__main=main \
    -Wl,--defsym=_start=main \
    -Wl,--defsym=__base_NCACHE_REGION=0x20000000 \
    -Wl,--defsym=__top_NCACHE_REGION=0x20200000 \
")

# Linker flags for release build
SET(CMAKE_EXE_LINKER_FLAGS_RELEASE " \
    ${FPU} \
    ${ARM_SYSROOT_FLAGS} \
    ${ARM_LINK_FLAGS} \
    -Wl,-Map=output.map \
    -Wl,--gc-sections \
    -Wl,--print-memory-usage \
    -mcpu=cortex-m33 \
    -mthumb \
    -T${ProjDirPath}/MIMXRT798Sxxxx_cm33_core0_flash.ld \
    -static \
    -target arm-none-eabi \
    -fuse-ld=lld \
    -rtlib=libgcc \
    -Wl,--entry=Reset_Handler \
    -Wl,--defsym=__main=main \
    -Wl,--defsym=_start=main \
    -Wl,--defsym=__base_NCACHE_REGION=0x20000000 \
    -Wl,--defsym=__top_NCACHE_REGION=0x20200000 \
")

# Linker flags for flash debug build
SET(CMAKE_EXE_LINKER_FLAGS_FLASH_DEBUG " \
    ${FPU} \
    ${ARM_SYSROOT_FLAGS} \
    ${ARM_LINK_FLAGS} \
    -Wl,-Map=output.map \
    -Wl,--gc-sections \
    -Wl,--print-memory-usage \
    -mcpu=cortex-m33 \
    -mthumb \
    -T${ProjDirPath}/MIMXRT798Sxxxx_cm33_core0_flash.ld \
    -static \
    -target arm-none-eabi \
    -fuse-ld=lld \
    -rtlib=libgcc \
")

# Linker flags for flash release build
SET(CMAKE_EXE_LINKER_FLAGS_FLASH_RELEASE " \
    ${FPU} \
    ${ARM_SYSROOT_FLAGS} \
    ${ARM_LINK_FLAGS} \
    -Wl,-Map=output.map \
    -Wl,--gc-sections \
    -Wl,--print-memory-usage \
    -mcpu=cortex-m33 \
    -mthumb \
    -T${ProjDirPath}/MIMXRT798Sxxxx_cm33_core0_flash.ld \
    -static \
    -target arm-none-eabi \
    -fuse-ld=lld \
    -rtlib=libgcc \
")

