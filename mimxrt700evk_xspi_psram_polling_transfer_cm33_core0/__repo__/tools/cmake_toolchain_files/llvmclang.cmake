# LLVM/Clang Toolchain for ARM Cortex-M33 (MIMXRT798S)
# Based on armgcc.cmake but adapted for LLVM/Clang toolchain

# TOOLCHAIN EXTENSION
IF(WIN32)
    SET(TOOLCHAIN_EXT ".exe")
ELSE()
    SET(TOOLCHAIN_EXT "")
ENDIF()

# EXECUTABLE EXTENSION
SET (CMAKE_EXECUTABLE_SUFFIX ".elf")

# TOOLCHAIN_DIR AND LLVM INSTALLATION
SET(TOOLCHAIN_DIR $ENV{LLVM_DIR})
STRING(REGEX REPLACE "\\\\" "/" TOOLCHAIN_DIR "${TOOLCHAIN_DIR}")

IF(NOT TOOLCHAIN_DIR)
    # Default to our built LLVM installation
    SET(TOOLCHAIN_DIR "$ENV{HOME}/Downloads/nxp/llvmproject/llvm-install")
ENDIF()

MESSAGE(STATUS "LLVM TOOLCHAIN_DIR: " ${TOOLCHAIN_DIR})

# TARGET_TRIPLET for ARM Cortex-M33
SET(TARGET_TRIPLET "thumbv8m.main-none-eabihf")

SET(TOOLCHAIN_BIN_DIR ${TOOLCHAIN_DIR}/bin)
SET(TOOLCHAIN_LIB_DIR ${TOOLCHAIN_DIR}/lib)

SET(CMAKE_SYSTEM_NAME Generic)
SET(CMAKE_SYSTEM_PROCESSOR arm)

# Set Clang as the compiler
SET(CMAKE_C_COMPILER ${TOOLCHAIN_BIN_DIR}/clang${TOOLCHAIN_EXT})
SET(CMAKE_CXX_COMPILER ${TOOLCHAIN_BIN_DIR}/clang++${TOOLCHAIN_EXT})
SET(CMAKE_ASM_COMPILER ${TOOLCHAIN_BIN_DIR}/clang${TOOLCHAIN_EXT})

set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_CXX_COMPILER_FORCED TRUE)

# Use LLVM tools instead of GNU tools
SET(CMAKE_OBJCOPY ${TOOLCHAIN_BIN_DIR}/llvm-objcopy CACHE INTERNAL "objcopy tool")
SET(CMAKE_OBJDUMP ${TOOLCHAIN_BIN_DIR}/llvm-objdump CACHE INTERNAL "objdump tool")
SET(CMAKE_AR ${TOOLCHAIN_BIN_DIR}/llvm-ar CACHE INTERNAL "ar tool")
SET(CMAKE_RANLIB ${TOOLCHAIN_BIN_DIR}/llvm-ranlib CACHE INTERNAL "ranlib tool")
SET(CMAKE_STRIP ${TOOLCHAIN_BIN_DIR}/llvm-strip CACHE INTERNAL "strip tool")

# Set target-specific flags for ARM Cortex-M33
SET(CORTEX_M33_FLAGS "--target=${TARGET_TRIPLET} -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16")

# Pure LLVM embedded flags - no system includes, use compiler-rt
SET(CLANG_COMMON_FLAGS "${CORTEX_M33_FLAGS} -fno-builtin -ffunction-sections -fdata-sections -nostdlib -nostdinc -ffreestanding")

# Add compiler-rt builtin headers
SET(CLANG_BUILTIN_INCLUDES "-isystem ${TOOLCHAIN_DIR}/lib/clang/18/include")

# Add minimal embedded C library headers (we'll use a lightweight implementation)
SET(CLANG_SYSTEM_INCLUDES "-isystem /home/smw016108/Downloads/nxp/llvmproject/mimxrt700evk_xspi_psram_edma_transfer_cm33_core0/embedded_libc/include")

SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${CLANG_COMMON_FLAGS} ${CLANG_BUILTIN_INCLUDES} ${CLANG_SYSTEM_INCLUDES} -O0 -g" CACHE INTERNAL "c compiler flags debug")
SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${CLANG_COMMON_FLAGS} ${CLANG_BUILTIN_INCLUDES} ${CLANG_SYSTEM_INCLUDES} -O0 -g -fno-rtti -fno-exceptions" CACHE INTERNAL "cxx compiler flags debug")
SET(CMAKE_ASM_FLAGS_DEBUG "${CMAKE_ASM_FLAGS_DEBUG} ${CORTEX_M33_FLAGS} -g" CACHE INTERNAL "asm compiler flags debug")

SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${CLANG_COMMON_FLAGS} ${CLANG_BUILTIN_INCLUDES} ${CLANG_SYSTEM_INCLUDES} -O3" CACHE INTERNAL "c compiler flags release")
SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${CLANG_COMMON_FLAGS} ${CLANG_BUILTIN_INCLUDES} ${CLANG_SYSTEM_INCLUDES} -O3 -fno-rtti -fno-exceptions" CACHE INTERNAL "cxx compiler flags release")
SET(CMAKE_ASM_FLAGS_RELEASE "${CMAKE_ASM_FLAGS_RELEASE} ${CORTEX_M33_FLAGS}" CACHE INTERNAL "asm compiler flags release")

# Use LLD linker with embedded-specific flags
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld ${CORTEX_M33_FLAGS} -Wl,--gc-sections -Wl,-static -nostdlib -nodefaultlibs" CACHE INTERNAL "linker flags")
SET(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -g" CACHE INTERNAL "linker flags debug")
SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE}" CACHE INTERNAL "linker flags release")

# Use our custom LLVM runtime library for embedded targets
SET(COMPILER_RT_LIB "${TOOLCHAIN_DIR}/lib/clang/18/lib/baremetal/libclang_rt.builtins-armv8m.main.a")
SET(CMAKE_C_STANDARD_LIBRARIES "${COMPILER_RT_LIB}" CACHE INTERNAL "Standard C libraries")
SET(CMAKE_CXX_STANDARD_LIBRARIES "${COMPILER_RT_LIB}" CACHE INTERNAL "Standard C++ libraries")

# Disable automatic linking of standard libraries
SET(CMAKE_C_IMPLICIT_LINK_LIBRARIES "" CACHE INTERNAL "")
SET(CMAKE_CXX_IMPLICIT_LINK_LIBRARIES "" CACHE INTERNAL "")
SET(CMAKE_C_IMPLICIT_LINK_DIRECTORIES "" CACHE INTERNAL "")
SET(CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES "" CACHE INTERNAL "")

# Override the default system libraries with our runtime library
SET(TARGET_LINK_SYSTEM_LIBRARIES "${COMPILER_RT_LIB}" CACHE INTERNAL "System libraries for embedded target")

SET(CMAKE_FIND_ROOT_PATH ${TOOLCHAIN_DIR} ${EXTRA_FIND_PATH})
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

MESSAGE(STATUS "BUILD_TYPE: " ${CMAKE_BUILD_TYPE})

include(${CMAKE_CURRENT_LIST_DIR}/mcux_config.cmake)

if(DEFINED LIBRARY_TYPE)
    if(DEFINED LANGUAGE)
        set_library(${LIBRARY_TYPE} ${LANGUAGE})
    endif()
    if(DEFINED DEBUG_CONSOLE)
        set_debug_console(${DEBUG_CONSOLE} ${LIBRARY_TYPE})
    endif()
endif()

if(DEFINED FPU_TYPE AND DEFINED FPU_ABI)
    set_floating_point(${FPU_TYPE} ${FPU_ABI})
endif()
